#ifndef WKJTX_PENDING_UPLOADS_DIALOG_HPP
#define WKJTX_PENDING_UPLOADS_DIALOG_HPP

// PendingUploadsDialog — v1.2.0. Modal view of the upload queue.
// Table of queued uploads + Retry selected / Retry all / Remove /
// Close buttons. Driven by UploadQueue::changed() signal so the
// table refreshes live as the dispatcher flushes entries.

#include <QDialog>

class QTableWidget;
class QLabel;

namespace wkjtx {

class UploadQueue;
class UploadDispatcher;

class PendingUploadsDialog : public QDialog
{
    Q_OBJECT

public:
    PendingUploadsDialog (UploadQueue * queue,
                          UploadDispatcher * dispatcher,
                          QWidget * parent = nullptr);
    ~PendingUploadsDialog () override;

private slots:
    void refresh ();
    void onRetrySelected ();
    void onRetryAll ();
    void onRemoveSelected ();

private:
    UploadQueue *      queue_;
    UploadDispatcher * dispatcher_;
    QTableWidget *     table_;
    QLabel *           status_;
};

} // namespace wkjtx

#endif // WKJTX_PENDING_UPLOADS_DIALOG_HPP
