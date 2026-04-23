#include "UploadDispatcher.hpp"

#include <QTimer>

#include "../Configuration.hpp"
#include "QrzUploader.hpp"
#include "../eqsl.h"

namespace wkjtx {

namespace {

// Rate-limit pacing between two sequential uploads during flushPending.
constexpr int kFlushStepIntervalMs = 250;

} // namespace

UploadDispatcher::UploadDispatcher (Configuration * cfg,
                                    UploadQueue * queue,
                                    QrzUploader * qrz,
                                    EQSL * eqsl,
                                    QObject * parent)
    : QObject {parent}
    , cfg_    {cfg}
    , queue_  {queue}
    , qrz_    {qrz}
    , eqsl_   {eqsl}
{
    if (qrz_) {
        connect (qrz_, &QrzUploader::uploaded,
                 this, &UploadDispatcher::onQrzOk);
        connect (qrz_, &QrzUploader::uploadFailed,
                 this, &UploadDispatcher::onQrzFail);
    }
    if (eqsl_) {
        connect (eqsl_, &EQSL::uploaded,
                 this,  &UploadDispatcher::onEqslOk);
        connect (eqsl_, &EQSL::uploadFailed,
                 this,  &UploadDispatcher::onEqslFail);
    }
}

UploadDispatcher::~UploadDispatcher () = default;

UploadMode UploadDispatcher::qrzMode () const
{
    if (!cfg_) return UploadMode::Auto;
    // Configuration returns the enum value as int to keep its header
    // free of wkjtx/ dependencies. Cast back at the boundary.
    return static_cast<UploadMode> (cfg_->qrz_upload_mode ());
}

UploadMode UploadDispatcher::eqslMode () const
{
    if (!cfg_) return UploadMode::Auto;
    return static_cast<UploadMode> (cfg_->eqsl_upload_mode ());
}

bool UploadDispatcher::qrzEnabled () const
{
    if (!cfg_) return false;
    return cfg_->send_to_qrz () && !cfg_->qrz_api_key ().isEmpty ();
}

bool UploadDispatcher::eqslEnabled () const
{
    if (!cfg_) return false;
    return cfg_->send_to_eqsl ();
}

void UploadDispatcher::onQsoAccepted (QString const & adifRecord,
                                      QString const & callsign,
                                      QString const & band,
                                      QString const & mode,
                                      QDateTime const & qsoDate)
{
    if (!queue_) return;

    // qrz.com
    if (qrzEnabled ()) {
        QueuedUpload q;
        q.service     = UploadService::Qrz;
        q.adifRecord  = adifRecord;
        q.callsign    = callsign;
        q.band        = band;
        q.mode        = mode;
        q.qsoDate     = qsoDate;
        int const id = queue_->enqueue (q);

        if (qrzMode () == UploadMode::Auto) {
            QueuedUpload live = q; live.id = id;
            uploadViaQrz (live);
        }
    }

    // eQSL — the legacy inline call in acceptQSO2 was removed; routing
    // now goes through the dispatcher for Auto-mode + the queue for
    // Manual mode / failure retry.
    if (eqslEnabled ()) {
        QueuedUpload q;
        q.service     = UploadService::Eqsl;
        q.adifRecord  = adifRecord;
        q.callsign    = callsign;
        q.band        = band;
        q.mode        = mode;
        q.qsoDate     = qsoDate;
        int const id = queue_->enqueue (q);

        if (eqslMode () == UploadMode::Auto) {
            QueuedUpload live = q; live.id = id;
            uploadViaEqsl (live);
        }
    }
}

void UploadDispatcher::retry (int queueId)
{
    if (!queue_) return;
    for (auto const & e : queue_->all ()) {
        if (e.id == queueId) {
            if (e.service == UploadService::Qrz)  uploadViaQrz (e);
            else                                  uploadViaEqsl (e);
            return;
        }
    }
}

void UploadDispatcher::clearPending ()
{
    if (queue_) queue_->clear ();
}

void UploadDispatcher::uploadViaQrz (QueuedUpload const & q)
{
    if (!qrz_ || !cfg_) return;
    qrz_->setApiKey (cfg_->qrz_api_key ());
    qrz_->setEnabled (true);
    qrz_inflight_ = {q.id, true};
    qrz_->uploadAdif (q.adifRecord);
}

void UploadDispatcher::uploadViaEqsl (QueuedUpload const & q)
{
    if (!eqsl_ || !cfg_) return;
    eqsl_inflight_ = {q.id, true};
    // Parameters mirror the old inline call in acceptQSO2; the
    // dispatcher does not capture a full ADIF payload for eQSL because
    // its upload() builds a fresh ADIF from the split fields.
    eqsl_->upload (cfg_->eqsl_username (),
                   cfg_->eqsl_passwd (),
                   cfg_->eqsl_nickname (),
                   q.callsign,
                   q.mode,
                   q.qsoDate,
                   /*rpt_sent*/ QStringLiteral ("0"),
                   q.band,
                   /*eqslcomments*/ QString {});
}

void UploadDispatcher::onQrzOk (QString callsign)
{
    if (!qrz_inflight_.valid || !queue_) return;
    int const id = qrz_inflight_.id;
    qrz_inflight_ = {};
    queue_->markSuccess (id);
    emit serviceSucceeded (UploadService::Qrz, callsign);
    if (flushing_) { ++flush_ok_; stepFlush (); }
}

void UploadDispatcher::onQrzFail (QString callsign, QString error)
{
    if (!qrz_inflight_.valid || !queue_) return;
    int const id = qrz_inflight_.id;
    qrz_inflight_ = {};
    queue_->markFailed (id, error);
    emit serviceFailed (UploadService::Qrz, callsign, error);
    if (flushing_) { ++flush_fail_; stepFlush (); }
}

void UploadDispatcher::onEqslOk (QString callsign)
{
    if (!eqsl_inflight_.valid || !queue_) return;
    int const id = eqsl_inflight_.id;
    eqsl_inflight_ = {};
    queue_->markSuccess (id);
    emit serviceSucceeded (UploadService::Eqsl, callsign);
    if (flushing_) { ++flush_ok_; stepFlush (); }
}

void UploadDispatcher::onEqslFail (QString callsign, QString error)
{
    if (!eqsl_inflight_.valid || !queue_) return;
    int const id = eqsl_inflight_.id;
    eqsl_inflight_ = {};
    queue_->markFailed (id, error);
    emit serviceFailed (UploadService::Eqsl, callsign, error);
    if (flushing_) { ++flush_fail_; stepFlush (); }
}

void UploadDispatcher::flushPending ()
{
    if (flushing_ || !queue_) return;
    flush_ids_.clear ();
    flush_ok_ = 0;
    flush_fail_ = 0;
    for (auto const & e : queue_->all ()) flush_ids_.append (e.id);
    if (flush_ids_.isEmpty ()) {
        emit allFlushed (0, 0);
        return;
    }
    flushing_ = true;
    stepFlush ();
}

void UploadDispatcher::stepFlush ()
{
    if (!flushing_) return;
    if (flush_ids_.isEmpty ()) {
        flushing_ = false;
        emit allFlushed (flush_ok_, flush_fail_);
        return;
    }
    // Pace uploads by kFlushStepIntervalMs to avoid tripping server
    // rate limits when the queue carries dozens of entries.
    QTimer::singleShot (kFlushStepIntervalMs, this, [this] {
        if (flush_ids_.isEmpty ()) {
            flushing_ = false;
            emit allFlushed (flush_ok_, flush_fail_);
            return;
        }
        int const id = flush_ids_.takeFirst ();
        // Entry may have been removed since we snapshotted ids (manual
        // deletion during flush). Look it up fresh; if gone, skip.
        bool found = false;
        for (auto const & e : queue_->all ()) {
            if (e.id == id) {
                if (e.service == UploadService::Qrz)  uploadViaQrz (e);
                else                                  uploadViaEqsl (e);
                found = true;
                break;
            }
        }
        if (!found) stepFlush ();
    });
}

} // namespace wkjtx
