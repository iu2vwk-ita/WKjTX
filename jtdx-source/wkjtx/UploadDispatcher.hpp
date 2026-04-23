#ifndef WKJTX_UPLOAD_DISPATCHER_HPP
#define WKJTX_UPLOAD_DISPATCHER_HPP

// UploadDispatcher — v1.2.0. Central router for qrz.com / eQSL uploads.
//
// Called from MainWindow::acceptQSO2, reads Configuration to decide
// between:
//   - Automatic mode: upload immediately, queue only on failure.
//   - Manual mode: queue only, never attempt now. User triggers
//     retries from PendingUploadsDialog or on app-close prompt.
//
// Replaces the previous inline eqsl->upload() call in acceptQSO2; the
// equivalent call for qrz.com is added here rather than in mainwindow.

#include <QObject>
#include <QQueue>
#include <QString>
#include <QDateTime>
#include <QHash>

#include "UploadQueue.hpp"

class Configuration;
class EQSL;

namespace wkjtx {

class QrzUploader;

enum class UploadMode
{
    Auto   = 0,
    Manual = 1,
};

class UploadDispatcher : public QObject
{
    Q_OBJECT

public:
    UploadDispatcher (Configuration * cfg,
                      UploadQueue * queue,
                      QrzUploader * qrz,
                      EQSL * eqsl,
                      QObject * parent = nullptr);
    ~UploadDispatcher () override;

    // Main entry point — called from acceptQSO2 with the ADIF record
    // produced by logqso.cpp.
    void onQsoAccepted (QString const & adifRecord,
                        QString const & callsign,
                        QString const & band,
                        QString const & mode,
                        QDateTime const & qsoDate);

    // Upload a single existing queue entry (from PendingUploadsDialog).
    void retry (int queueId);

    // Upload every queued entry, with a short pacing delay between
    // each to avoid tripping qrz.com rate limits. Emits allFlushed()
    // when the last reply has arrived (or on 30 s overall timeout).
    void flushPending ();

    // Wipe the queue (user confirmed at close-time prompt).
    void clearPending ();

    int pendingCount () const { return queue_ ? queue_->size () : 0; }

signals:
    void serviceSucceeded (UploadService s, QString callsign);
    void serviceFailed    (UploadService s, QString callsign, QString error);
    void allFlushed       (int successCount, int failCount);

private slots:
    void onQrzOk   (QString callsign);
    void onQrzFail (QString callsign, QString error);
    void onEqslOk  (QString callsign);
    void onEqslFail(QString callsign, QString error);
    void stepFlush ();              // drives the serialised flushPending

private:
    struct InFlight
    {
        int  id {0};
        bool valid {false};
    };

    UploadMode qrzMode () const;
    UploadMode eqslMode () const;
    bool qrzEnabled () const;
    bool eqslEnabled () const;

    void uploadViaQrz  (QueuedUpload const & q);
    void uploadViaEqsl (QueuedUpload const & q);

    Configuration * cfg_   {nullptr};
    UploadQueue *   queue_ {nullptr};
    QrzUploader *   qrz_   {nullptr};
    EQSL *          eqsl_  {nullptr};

    // Tracks which queue id is currently in flight per service.
    InFlight qrz_inflight_;
    InFlight eqsl_inflight_;

    // State for flushPending().
    bool              flushing_ {false};
    QVector<int>      flush_ids_;
    int               flush_ok_ {0};
    int               flush_fail_ {0};
};

} // namespace wkjtx

#endif // WKJTX_UPLOAD_DISPATCHER_HPP
