// This source code file was last time modified by Arvo ES1JA on 20181229
// v1.2.0 (WKjTX): added signal-based success/failure reporting so the
// UploadDispatcher can queue retries on network error or server reject.

#include "wsprnet.h"

#include <cmath>

#include <QTimer>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

#include "moc_eqsl.cpp"

namespace
{
  char const * const EqslUrl = "http://www.eqsl.cc/qslcard/importADIF.cfm";
  // char const * const wsprNetUrl = "http://127.0.0.1/post?";
};

EQSL::EQSL(QNetworkAccessManager * manager, QObject *parent)
  : QObject{parent}
  , networkManager {manager}
  , uploadTimer {new QTimer {this}}
  , m_in_progress {false}
{
  uploadTimer->setSingleShot(true);
  connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReply(QNetworkReply*)));
  connect( uploadTimer, SIGNAL(timeout()), this, SLOT(work()));
}

void EQSL::upload(QString const& eqsl_username, QString const& eqsl_passwd, QString const& eqsl_nickname
                  , QString const& call, QString const& mode
                  , QDateTime const& QSO_date_on
                  , QString const& rpt_sent, QString const& band
                  , QString const& eqslcomments)
{

    if (m_in_progress) {
        reply->abort();
        m_in_progress = false;
    }
    m_pending_call = call;
    myadif="<ADIF_VER:5>2.1.9";
    myadif+="<EQSL_USER:" + QString::number(eqsl_username.length()) + ">" + eqsl_username;
    myadif+="<EQSL_PSWD:" + QString::number(eqsl_passwd.length()) + ">" + eqsl_passwd;
    myadif+="<PROGRAMID:5>WKjTX<EOH><APP_EQSL_QTH_NICKNAME:" + QString::number(eqsl_nickname.length()) + ">" + eqsl_nickname;
    myadif+="<CALL:" + QString::number(call.length()) + ">" + call;
    myadif+="<MODE:"  + QString::number(mode.length()) + ">" + mode;
    myadif+="<QSO_DATE:8>" + QSO_date_on.date().toString("yyyyMMdd");
    myadif+="<TIME_ON:4>" + QSO_date_on.time().toString("hhmm");
    myadif+="<RST_SENT:" + QString::number(rpt_sent.length()) + ">" + rpt_sent;
    myadif+="<BAND:" + QString::number(band.length()) + ">" + band;
    if(eqslcomments!="") myadif+="<QSLMSG:" + QString::number(eqslcomments.length()) + ">" + eqslcomments;
//    myadif+="<QSLMSG:19>TNX For QSO TU 73!.";
    myadif+="<EOR>";

    uploadTimer->start(1);
}

void EQSL::networkReply(QNetworkReply *incoming)
{
    // The QNetworkAccessManager is shared with other components
    // (QrzUploader, DataUpdater, etc.). Filter to the reply we're
    // actually tracking, otherwise we'd wipe m_in_progress on somebody
    // else's request.
    if (incoming != this->reply) {
        return;
    }

    QString const call = m_pending_call;
    bool const had_error = (QNetworkReply::NoError != incoming->error ());
    QString body;
    if (!had_error) {
        body = QString::fromUtf8 (incoming->readAll ());
    }

    if (had_error) {
      printf ("eqsl upload error:%d\n", incoming->error ());
      emit uploadFailed (call,
          QStringLiteral ("rete: %1").arg (incoming->errorString ()));
    }
    else {
      // eQSL returns HTML. Success indicators the server puts in the
      // body: "Result: 1 records added" or "QSL Record(s) uploaded"
      // or a line starting with "Result:" followed by a success
      // phrase. Failure surfaces as "Error:" / "Warning:" /
      // "Result: 0 records added" plus a human-readable reason.
      QString const lower = body.toLower ();
      bool const ok =
          (lower.contains (QLatin1String ("result:"))
              && lower.contains (QLatin1String ("records added"))
              && !lower.contains (QLatin1String ("result: 0 records added")))
       || lower.contains (QLatin1String ("record(s) uploaded"))
       || lower.contains (QLatin1String ("qsl record(s)"));
      if (ok) {
        emit uploaded (call);
      } else {
        // Try to extract a short reason ("Error: ...", up to end of line).
        QString reason;
        int const errIdx = lower.indexOf (QLatin1String ("error:"));
        if (errIdx >= 0) {
            int const eol = body.indexOf (QLatin1Char ('\n'), errIdx);
            reason = body.mid (errIdx,
                               eol > 0 ? eol - errIdx : 200).trimmed ();
        } else {
            reason = body.left (160).simplified ();
        }
        if (reason.isEmpty ()) reason = QStringLiteral ("risposta eQSL non riconosciuta");
        emit uploadFailed (call,
            QStringLiteral ("eqsl.cc: %1").arg (reason));
      }
    }

    // delete request object instance on return to the event loop otherwise it is leaked
    incoming->deleteLater ();
    m_in_progress = false;
    this->reply = nullptr;
    m_pending_call.clear ();
}

void EQSL::work()
{
#if QT_VERSION < QT_VERSION_CHECK (5, 15, 0)
      if (QNetworkAccessManager::Accessible != networkManager->networkAccessible ()) {
        // try and recover network access for QNAM
        networkManager->setNetworkAccessible (QNetworkAccessManager::Accessible);
      }
#endif
    m_in_progress = true;
    QUrl url(EqslUrl);
    QUrlQuery query;
    query.addQueryItem("ADIFdata", myadif);
    url.setQuery(query.query());
    m_in_progress = true;
//    printf ("eqsl upload request:%s\n",myadif.toStdString().c_str());
    QNetworkRequest request(url);
    reply = networkManager->get(request);
//    printf ("sent\n");
}

