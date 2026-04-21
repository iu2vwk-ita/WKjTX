#include "QrzUploader.hpp"

// Real QRZ Logbook API uploader.
// Endpoint: POST https://logbook.qrz.com/api
// Body (application/x-www-form-urlencoded):
//     KEY=<api_key>&ACTION=INSERT&ADIF=<adif_record>
// Response (text/plain, key=value pairs separated by '&'):
//     RESULT=OK&LOGID=...&COUNT=1&...
//   or
//     RESULT=FAIL&REASON=<text>
//
// Reference: https://www.qrz.com/docs/logbook/QRZLogbookAPI.html

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace wkjtx {

namespace {

// Extract a call field from an ADIF record for user-facing messages.
// Returns empty if not found.
QString extractCall (QString const & adif)
{
  int const i = adif.indexOf (QLatin1String ("<CALL:"), 0, Qt::CaseInsensitive);
  if (i < 0) return {};
  int const colon = adif.indexOf (':', i + 6);
  if (colon < 0) return {};
  int const gt = adif.indexOf ('>', colon + 1);
  if (gt < 0) return {};
  QString const lenStr = adif.mid (colon + 1, gt - colon - 1);
  bool ok = false;
  int const len = lenStr.toInt (&ok);
  if (!ok || len <= 0 || gt + 1 + len > adif.size ()) return {};
  return adif.mid (gt + 1, len).trimmed ();
}

// Parse qrz.com response text into a RESULT value.
QString parseResult (QString const & body)
{
  QUrlQuery q;
  // qrz.com returns &-delimited key=value pairs, same shape as a query.
  q.setQuery (body);
  return q.queryItemValue (QStringLiteral ("RESULT"));
}

QString parseReason (QString const & body)
{
  QUrlQuery q;
  q.setQuery (body);
  return q.queryItemValue (QStringLiteral ("REASON"));
}

} // namespace

QrzUploader::QrzUploader (QObject * parent)
  : QObject {parent}
  , nam_ {new QNetworkAccessManager {this}}
{}

QrzUploader::~QrzUploader () = default;

void QrzUploader::setApiKey (QString const & key) { apiKey_ = key.trimmed (); }
void QrzUploader::setEnabled (bool on)            { enabled_ = on; }

void QrzUploader::uploadAdif (QString const & adifRecord)
{
  if (!enabled_) return;
  if (apiKey_.isEmpty ()) {
    emit uploadFailed (extractCall (adifRecord),
                       QStringLiteral ("qrz.com: API key non impostata"));
    return;
  }

  QUrl const url {QStringLiteral ("https://logbook.qrz.com/api")};
  QNetworkRequest req {url};
  req.setHeader (QNetworkRequest::ContentTypeHeader,
                 QStringLiteral ("application/x-www-form-urlencoded"));
  req.setHeader (QNetworkRequest::UserAgentHeader,
                 QStringLiteral ("WKjTX/1.0 (+https://github.com/iu2vwk-ita/WKjTX)"));

  QUrlQuery body;
  body.addQueryItem (QStringLiteral ("KEY"),    apiKey_);
  body.addQueryItem (QStringLiteral ("ACTION"), QStringLiteral ("INSERT"));
  body.addQueryItem (QStringLiteral ("ADIF"),   adifRecord);
  QByteArray const data = body.toString (QUrl::FullyEncoded).toUtf8 ();

  QNetworkReply * reply = nam_->post (req, data);
  QString const call = extractCall (adifRecord);

  connect (reply, &QNetworkReply::finished, this, [this, reply, call] {
    reply->deleteLater ();
    if (reply->error () != QNetworkReply::NoError) {
      emit uploadFailed (call,
          QStringLiteral ("rete: %1").arg (reply->errorString ()));
      return;
    }
    QString const body = QString::fromUtf8 (reply->readAll ()).trimmed ();
    QString const result = parseResult (body);
    if (result.compare (QLatin1String ("OK"), Qt::CaseInsensitive) == 0) {
      emit uploaded (call);
    } else {
      QString reason = parseReason (body);
      if (reason.isEmpty ()) reason = body;   // fall back to raw body
      emit uploadFailed (call,
          QStringLiteral ("qrz.com: %1").arg (reason));
    }
  });
}

} // namespace wkjtx
