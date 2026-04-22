#ifndef WKJTX_UPLOAD_QUEUE_HPP
#define WKJTX_UPLOAD_QUEUE_HPP

// UploadQueue — persistent FIFO of pending qrz.com / eQSL uploads.
//
// v1.2.0. Each entry survives app restart via a JSON file at
// %LOCALAPPDATA%/WKjTX/upload_queue.json. The dispatcher pushes new
// entries when a QSO fails to upload (network error, 4xx, etc.) or
// when the user has configured Manual upload mode; the PendingUploads
// dialog drives retry / remove via this class.

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>

namespace wkjtx {

enum class UploadService
{
    Qrz  = 0,
    Eqsl = 1,
};

struct QueuedUpload
{
    int           id {0};
    UploadService service {UploadService::Qrz};
    QString       adifRecord;   // full <EOR>-terminated record
    QString       callsign;
    QString       band;
    QString       mode;
    QDateTime     qsoDate;
    int           attempts {0};
    QString       lastError;
    QDateTime     lastAttempt;  // null if never attempted (manual mode)
};

class UploadQueue : public QObject
{
    Q_OBJECT

public:
    explicit UploadQueue (QString const & jsonPath,
                          QObject * parent = nullptr);
    ~UploadQueue () override;

    // Append a new entry. Returns the assigned id.
    int enqueue (QueuedUpload entry);

    // Remove an entry by id. Returns true if found.
    bool remove (int id);

    // Mark an entry as failed: increments attempts, stamps lastAttempt
    // + lastError. Does NOT remove. Returns true if found.
    bool markFailed (int id, QString const & error);

    // Mark an entry as successfully uploaded: removes it.
    bool markSuccess (int id);

    // Views.
    QVector<QueuedUpload> all () const { return entries_; }
    QVector<QueuedUpload> forService (UploadService s) const;
    int size () const { return entries_.size (); }
    bool isEmpty () const { return entries_.isEmpty (); }

    // Wipe everything (user confirmed "Discard").
    void clear ();

signals:
    // Fired on any mutation so UIs can refresh.
    void changed ();

private:
    void load ();
    void save () const;

    QString  path_;
    QVector<QueuedUpload> entries_;
    int next_id_ {1};
};

} // namespace wkjtx

#endif // WKJTX_UPLOAD_QUEUE_HPP
