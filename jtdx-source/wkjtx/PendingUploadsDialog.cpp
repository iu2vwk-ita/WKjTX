#include "PendingUploadsDialog.hpp"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "UploadQueue.hpp"
#include "UploadDispatcher.hpp"

namespace wkjtx {

namespace {

QString serviceLabel (UploadService s)
{
    return s == UploadService::Qrz ? QStringLiteral ("qrz.com")
                                   : QStringLiteral ("eQSL");
}

} // namespace

PendingUploadsDialog::PendingUploadsDialog (UploadQueue * queue,
                                            UploadDispatcher * dispatcher,
                                            QWidget * parent)
    : QDialog {parent}
    , queue_ {queue}
    , dispatcher_ {dispatcher}
    , table_ {new QTableWidget {this}}
    , status_ {new QLabel {this}}
{
    setWindowTitle (tr ("Pending QSO uploads — qrz.com / eQSL"));
    resize (820, 420);

    table_->setColumnCount (7);
    table_->setHorizontalHeaderLabels (QStringList ()
        << tr ("Service")
        << tr ("Callsign")
        << tr ("Band")
        << tr ("Mode")
        << tr ("QSO date")
        << tr ("Attempts")
        << tr ("Last error"));
    table_->horizontalHeader ()->setSectionResizeMode (QHeaderView::Interactive);
    table_->horizontalHeader ()->setStretchLastSection (true);
    table_->setSelectionBehavior (QAbstractItemView::SelectRows);
    table_->setSelectionMode     (QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers      (QAbstractItemView::NoEditTriggers);
    table_->verticalHeader ()->setVisible (false);

    auto * retrySel    = new QPushButton {tr ("&Retry selected"), this};
    auto * retryAll    = new QPushButton {tr ("Retry &all"),      this};
    auto * removeSel   = new QPushButton {tr ("Re&move selected"),this};
    auto * closeBtn    = new QPushButton {tr ("&Close"),          this};

    auto * buttons = new QHBoxLayout;
    buttons->addWidget (retrySel);
    buttons->addWidget (retryAll);
    buttons->addWidget (removeSel);
    buttons->addStretch (1);
    buttons->addWidget (closeBtn);

    auto * layout = new QVBoxLayout {this};
    layout->addWidget (status_);
    layout->addWidget (table_, 1);
    layout->addLayout (buttons);

    connect (retrySel,  &QPushButton::clicked, this, &PendingUploadsDialog::onRetrySelected);
    connect (retryAll,  &QPushButton::clicked, this, &PendingUploadsDialog::onRetryAll);
    connect (removeSel, &QPushButton::clicked, this, &PendingUploadsDialog::onRemoveSelected);
    connect (closeBtn,  &QPushButton::clicked, this, &QDialog::accept);

    if (queue_) {
        connect (queue_, &UploadQueue::changed,
                 this,   &PendingUploadsDialog::refresh);
    }
    if (dispatcher_) {
        connect (dispatcher_, &UploadDispatcher::serviceSucceeded,
                 this,        [this] (UploadService, QString) { refresh (); });
        connect (dispatcher_, &UploadDispatcher::serviceFailed,
                 this,        [this] (UploadService, QString, QString) { refresh (); });
    }

    refresh ();
}

PendingUploadsDialog::~PendingUploadsDialog () = default;

void PendingUploadsDialog::refresh ()
{
    if (!queue_) return;
    auto const entries = queue_->all ();
    table_->setRowCount (entries.size ());
    for (int r = 0; r < entries.size (); ++r) {
        auto const & e = entries[r];
        auto * idItem = new QTableWidgetItem (serviceLabel (e.service));
        // Stash the queue id on the service column so button handlers
        // can recover it from the selected row.
        idItem->setData (Qt::UserRole, e.id);
        table_->setItem (r, 0, idItem);
        table_->setItem (r, 1, new QTableWidgetItem (e.callsign));
        table_->setItem (r, 2, new QTableWidgetItem (e.band));
        table_->setItem (r, 3, new QTableWidgetItem (e.mode));
        table_->setItem (r, 4, new QTableWidgetItem (e.qsoDate.toString (Qt::ISODate)));
        table_->setItem (r, 5, new QTableWidgetItem (QString::number (e.attempts)));
        table_->setItem (r, 6, new QTableWidgetItem (e.lastError));
    }
    status_->setText (tr ("%n QSO in queue.", "", entries.size ()));
}

void PendingUploadsDialog::onRetrySelected ()
{
    if (!dispatcher_) return;
    auto rows = table_->selectionModel ()->selectedRows ();
    for (auto const & idx : rows) {
        auto * item = table_->item (idx.row (), 0);
        if (!item) continue;
        int const id = item->data (Qt::UserRole).toInt ();
        if (id > 0) dispatcher_->retry (id);
    }
}

void PendingUploadsDialog::onRetryAll ()
{
    if (!dispatcher_) return;
    dispatcher_->flushPending ();
}

void PendingUploadsDialog::onRemoveSelected ()
{
    if (!queue_) return;
    auto rows = table_->selectionModel ()->selectedRows ();
    if (rows.isEmpty ()) return;
    if (QMessageBox::question (this,
                               tr ("Remove queued uploads"),
                               tr ("Remove %n selected queued upload(s)?", "", rows.size ()))
        != QMessageBox::Yes) {
        return;
    }
    // Collect ids BEFORE mutating the model (row indices would shift).
    QVector<int> ids;
    for (auto const & idx : rows) {
        auto * item = table_->item (idx.row (), 0);
        if (item) ids.append (item->data (Qt::UserRole).toInt ());
    }
    for (int id : ids) queue_->remove (id);
}

} // namespace wkjtx
