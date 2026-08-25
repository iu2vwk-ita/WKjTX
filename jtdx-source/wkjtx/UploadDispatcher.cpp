#include "UploadDispatcher.hpp"

#include <QTimer>

#include "../Configuration.hpp"
#include "QrzUploader.hpp"
#include "ClubLogUploader.hpp"
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
                                    ClubLogUploader * clublog,
                                    QObject * parent)
    : QObject   {parent}
    , cfg_      {cfg}
    , queue_    {queue}
    , qrz_      {qrz}
    , eqsl_     {eqsl}
    , clublog_  {clublog}
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
    if (clublog_) {
        connect (clublog_, &ClubLogUploader::uploaded,
                 this,     &UploadDispatcher::onClubLogOk);
        connect (clublog_, &ClubLogUploader::uploadFailed,
                 this,     &UploadDispatcher::onClubLogFail);
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

UploadMode UploadDispatcher::clublogMode () const
{
    if (!cfg_) return UploadMode::Auto;
    return static_cast<UploadMode> (cfg_->clublog_upload_mode ());
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

bool UploadDispatcher::clublogEnabled () const
{
    if (!cfg_ || !clublog_) return false;
    return cfg_->send_to_clublog ()
        && !cfg_->clublog_email ().isEmpty ()
        && !cfg_->clublog_api_key ().isEmpty ();
}

void UploadDispatcher::onQsoAccepted (QString const & adifRecord,
                                      QString const & callsign,
                                      QString const & band,
                                      QString const & mode,
                                      QDateTime const & qsoDate)
{
    if (!queue_) return;

    if (qrzEnabled ()) {
        enqueueFor (UploadService::Qrz, qrzMode (),
                    adifRecord, callsign, band, mode, qsoDate);
    }

    // eQSL — the legacy inline call in acceptQSO2 was removed; routing
    // now goes through the dispatcher for Auto-mode + the queue for
    // Manual mode / failure retry.
    if (eqslEnabled ()) {
        enqueueFor (UploadService::Eqsl, eqslMode (),
                    adifRecord, callsign, band, mode, qsoDate);
    }

    // Club Log (v1.4.0). realtime.php is rated for operator-pace QSOs,
    // which is exactly one call per logged contact.
    if (clublogEnabled ()) {
        enqueueFor (UploadService::ClubLog, clublogMode (),
                    adifRecord, callsign, band, mode, qsoDate);
    }
}

void UploadDispatcher::enqueueFor (UploadService service,
                                   UploadMode mode,
                                   QString const & adifRecord,
                                   QString const & callsign,
                                   QString const & band,
                                   QString const & mode_name,
                                   QDateTime const & qsoDate)
{
    QueuedUpload q;
    q.service    = service;
    q.adifRecord = adifRecord;
    q.callsign   = callsign;
    q.band       = band;
    q.mode       = mode_name;
    q.qsoDate    = qsoDate;
    int const id = queue_->enqueue (q);

    if (mode == UploadMode::Auto) {
        QueuedUpload live = q;
        live.id = id;
        uploadEntry (live);
    }
}

void UploadDispatcher::uploadEntry (QueuedUpload const & q)
{
    switch (q.service) {
    case UploadService::Qrz:     uploadViaQrz (q);     break;
    case UploadService::Eqsl:    uploadViaEqsl (q);    break;
    case UploadService::ClubLog: uploadViaClubLog (q); break;
    }
}

void UploadDispatcher::retry (int queueId)
{
    if (!queue_) return;
    for (auto const & e : queue_->all ()) {
        if (e.id == queueId) {
            uploadEntry (e);
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

void UploadDispatcher::uploadViaClubLog (QueuedUpload const & q)
{
    if (!clublog_ || !cfg_) return;
    clublog_->setCredentials (cfg_->clublog_email (),
                              cfg_->clublog_password (),
                              cfg_->clublog_callsign ().isEmpty ()
                                  ? cfg_->my_callsign ()
                                  : cfg_->clublog_callsign (),
                              cfg_->clublog_api_key ());
    clublog_->setEnabled (true);
    clublog_inflight_ = {q.id, true};
    clublog_->uploadAdif (q.adifRecord);
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

void UploadDispatcher::onClubLogOk (QString callsign)
{
    if (!clublog_inflight_.valid || !queue_) return;
    int const id = clublog_inflight_.id;
    clublog_inflight_ = {};
    queue_->markSuccess (id);
    emit serviceSucceeded (UploadService::ClubLog, callsign);
    if (flushing_) { ++flush_ok_; stepFlush (); }
}

void UploadDispatcher::onClubLogFail (QString callsign, QString error)
{
    if (!clublog_inflight_.valid || !queue_) return;
    int const id = clublog_inflight_.id;
    clublog_inflight_ = {};
    queue_->markFailed (id, error);
    emit serviceFailed (UploadService::ClubLog, callsign, error);
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
                uploadEntry (e);
                found = true;
                break;
            }
        }
        if (!found) stepFlush ();
    });
}

} // namespace wkjtx
