#include "ClubLogUploader.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include "AdifUtils.hpp"

namespace wkjtx {

namespace {

constexpr char kRealtimeUrl[] = "https://clublog.org/realtime.php";

} // namespace

ClubLogUploader::ClubLogUploader (QObject * parent)
  : QObject {parent}
  , nam_ {new QNetworkAccessManager {this}}
{}

ClubLogUploader::~ClubLogUploader () = default;

void ClubLogUploader::setCredentials (QString const & email,
                                      QString const & password,
                                      QString const & callsign,
                                      QString const & apiKey)
{
  bool const changed = email    != email_
                    || password != password_
                    || callsign != logCallsign_
                    || apiKey   != apiKey_;
  email_       = email.trimmed ();
  password_    = password;
  logCallsign_ = callsign.trimmed ().toUpper ();
  apiKey_      = apiKey.trimmed ();
  // New credentials deserve a fresh attempt.
  if (changed) authRejected_ = false;
}

void ClubLogUploader::setEnabled (bool on) { enabled_ = on; }

void ClubLogUploader::uploadAdif (QString const & adifRecord)
{
  if (!enabled_) return;

  QString const call = adifField (adifRecord, QStringLiteral ("CALL"));

  if (authRejected_) {
    emit uploadFailed (call,
        tr ("Club Log: credentials rejected earlier — fix them in "
            "Settings → Reporting before retrying"));
    return;
  }
  if (email_.isEmpty () || password_.isEmpty ()
      || logCallsign_.isEmpty () || apiKey_.isEmpty ()) {
    emit uploadFailed (call,
        tr ("Club Log: e-mail, password, callsign and API key are all "
            "required"));
    return;
  }

  QNetworkRequest req {QUrl {QString::fromLatin1 (kRealtimeUrl)}};
  req.setHeader (QNetworkRequest::ContentTypeHeader,
                 QStringLiteral ("application/x-www-form-urlencoded"));
  req.setHeader (QNetworkRequest::UserAgentHeader,
                 QStringLiteral ("WKjTX (+https://github.com/iu2vwk-ita/WKjTX)"));

  QUrlQuery body;
  body.addQueryItem (QStringLiteral ("email"),    email_);
  body.addQueryItem (QStringLiteral ("password"), password_);
  body.addQueryItem (QStringLiteral ("callsign"), logCallsign_);
  body.addQueryItem (QStringLiteral ("api"),      apiKey_);
  body.addQueryItem (QStringLiteral ("adif"),     adifRecord);
  QByteArray const data = body.toString (QUrl::FullyEncoded).toUtf8 ();

  QNetworkReply * reply = nam_->post (req, data);

  connect (reply, &QNetworkReply::finished, this, [this, reply, call] {
    reply->deleteLater ();

    int const status = reply
        ->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
    QString const text = QString::fromUtf8 (reply->readAll ()).trimmed ();

    if (status == 200) {
      emit uploaded (call);
      return;
    }

    // 403 means bad credentials. Club Log firewalls IPs that keep
    // hammering with them, so stop here until the operator edits the
    // settings.
    if (status == 403) {
      authRejected_ = true;
      emit uploadFailed (call,
          tr ("Club Log: rejected credentials (403) — uploads disabled "
              "until you correct them. %1").arg (text));
      return;
    }

    if (status) {
      emit uploadFailed (call,
          QStringLiteral ("Club Log: HTTP %1 %2").arg (status).arg (text));
    } else {
      emit uploadFailed (call,
          tr ("network: %1").arg (reply->errorString ()));
    }
  });
}

} // namespace wkjtx
