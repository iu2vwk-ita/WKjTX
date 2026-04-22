#include "UploadQueue.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QtGlobal>

namespace wkjtx {

namespace {

QString serviceToString (UploadService s)
{
    switch (s) {
    case UploadService::Qrz:  return QStringLiteral ("qrz");
    case UploadService::Eqsl: return QStringLiteral ("eqsl");
    }
    return QStringLiteral ("qrz");
}

UploadService serviceFromString (QString const & s)
{
    if (s.compare (QLatin1String ("eqsl"), Qt::CaseInsensitive) == 0)
        return UploadService::Eqsl;
    return UploadService::Qrz;
}

QJsonObject entryToJson (QueuedUpload const & e)
{
    QJsonObject o;
    o.insert ("id",          e.id);
    o.insert ("service",     serviceToString (e.service));
    o.insert ("adif",        e.adifRecord);
    o.insert ("call",        e.callsign);
    o.insert ("band",        e.band);
    o.insert ("mode",        e.mode);
    o.insert ("qsoDate",     e.qsoDate.toString (Qt::ISODate));
    o.insert ("attempts",    e.attempts);
    o.insert ("lastError",   e.lastError);
    o.insert ("lastAttempt", e.lastAttempt.isValid ()
                                 ? e.lastAttempt.toString (Qt::ISODate)
                                 : QString {});
    return o;
}

QueuedUpload entryFromJson (QJsonObject const & o)
{
    QueuedUpload e;
    e.id          = o.value ("id").toInt ();
    e.service     = serviceFromString (o.value ("service").toString ());
    e.adifRecord  = o.value ("adif").toString ();
    e.callsign    = o.value ("call").toString ();
    e.band        = o.value ("band").toString ();
    e.mode        = o.value ("mode").toString ();
    e.qsoDate     = QDateTime::fromString (o.value ("qsoDate").toString (),
                                           Qt::ISODate);
    e.attempts    = o.value ("attempts").toInt ();
    e.lastError   = o.value ("lastError").toString ();
    auto const la = o.value ("lastAttempt").toString ();
    if (!la.isEmpty ())
        e.lastAttempt = QDateTime::fromString (la, Qt::ISODate);
    return e;
}

} // namespace

UploadQueue::UploadQueue (QString const & jsonPath, QObject * parent)
    : QObject {parent}
    , path_ {jsonPath}
{
    load ();
}

UploadQueue::~UploadQueue () = default;

int UploadQueue::enqueue (QueuedUpload entry)
{
    entry.id = next_id_++;
    entries_.append (entry);
    save ();
    emit changed ();
    return entry.id;
}

bool UploadQueue::remove (int id)
{
    for (int i = 0; i < entries_.size (); ++i) {
        if (entries_[i].id == id) {
            entries_.removeAt (i);
            save ();
            emit changed ();
            return true;
        }
    }
    return false;
}

bool UploadQueue::markFailed (int id, QString const & error)
{
    for (auto & e : entries_) {
        if (e.id == id) {
            e.attempts    += 1;
            e.lastError    = error;
            e.lastAttempt  = QDateTime::currentDateTimeUtc ();
            save ();
            emit changed ();
            return true;
        }
    }
    return false;
}

bool UploadQueue::markSuccess (int id)
{
    return remove (id);
}

QVector<QueuedUpload> UploadQueue::forService (UploadService s) const
{
    QVector<QueuedUpload> out;
    for (auto const & e : entries_)
        if (e.service == s) out.append (e);
    return out;
}

void UploadQueue::clear ()
{
    if (entries_.isEmpty ()) return;
    entries_.clear ();
    save ();
    emit changed ();
}

void UploadQueue::load ()
{
    QFileInfo fi {path_};
    QDir {}.mkpath (fi.absolutePath ());

    QFile f {path_};
    if (!f.exists ()) {
        entries_.clear ();
        next_id_ = 1;
        return;
    }
    if (!f.open (QIODevice::ReadOnly)) {
        qWarning ("UploadQueue: cannot open %s for reading: %s",
                  qPrintable (path_), qPrintable (f.errorString ()));
        entries_.clear ();
        return;
    }
    QByteArray const raw = f.readAll ();
    f.close ();

    QJsonParseError err;
    QJsonDocument const doc = QJsonDocument::fromJson (raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject ()) {
        qWarning ("UploadQueue: corrupt JSON in %s — falling back to empty queue (%s)",
                  qPrintable (path_), qPrintable (err.errorString ()));
        entries_.clear ();
        next_id_ = 1;
        return;
    }
    QJsonObject const root = doc.object ();
    next_id_ = root.value ("nextId").toInt (1);
    entries_.clear ();
    for (auto const & v : root.value ("entries").toArray ())
        entries_.append (entryFromJson (v.toObject ()));

    // Be defensive: if the file carried a smaller nextId than the max
    // existing entry id, bump so we never reuse an id.
    for (auto const & e : entries_)
        if (e.id >= next_id_) next_id_ = e.id + 1;
}

void UploadQueue::save () const
{
    QJsonObject root;
    root.insert ("nextId", next_id_);
    QJsonArray arr;
    for (auto const & e : entries_) arr.append (entryToJson (e));
    root.insert ("entries", arr);

    QSaveFile f {path_};
    if (!f.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning ("UploadQueue: cannot write %s: %s",
                  qPrintable (path_), qPrintable (f.errorString ()));
        return;
    }
    f.write (QJsonDocument {root}.toJson (QJsonDocument::Indented));
    f.commit ();
}

} // namespace wkjtx
