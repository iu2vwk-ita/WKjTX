#include "RESTTransceiver.hpp"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QUrl>

#include "moc_RESTTransceiver.cpp"

namespace
{
  auto constexpr api_base = "/api/v1";
}

RESTTransceiver::RESTTransceiver (QString const& host, quint16 port, int trx_id,
                                  int poll_interval, QObject * parent)
  : PollingTransceiver {poll_interval, parent}
  , net_ {new QNetworkAccessManager {this}}
  , host_ {host}
  , port_ {port}
  , trx_id_ {trx_id}
{
}

void RESTTransceiver::register_transceivers (TransceiverFactory::Transceivers * registry, unsigned id)
{
  (*registry)["Hamlib REST API"] = TransceiverFactory::Capabilities {
    id
    , TransceiverFactory::Capabilities::none
    , false
    , false
    , false
    , true
  };
}

QString RESTTransceiver::restUrl (QString const& path) const
{
  return QStringLiteral ("http://%1:%2%3/rigs/%4%5")
      .arg (host_).arg (port_).arg (api_base).arg (trx_id_).arg (path);
}

QString RESTTransceiver::httpGet (QString const& url) const
{
  QNetworkRequest req {QUrl {url}};
  QNetworkReply * reply = net_->get (req);
  QEventLoop loop;
  QObject::connect (reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec (QEventLoop::ExcludeUserInputEvents);
  QString body = QString::fromUtf8 (reply->readAll ());
  reply->deleteLater ();
  return body;
}

bool RESTTransceiver::httpPost (QString const& url, QByteArray const& data) const
{
  QNetworkRequest req {QUrl {url}};
  req.setHeader (QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkReply * reply = net_->post (req, data);
  QEventLoop loop;
  QObject::connect (reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec (QEventLoop::ExcludeUserInputEvents);
  auto status = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute);
  reply->deleteLater ();
  return status.isValid () && status.toInt () >= 200 && status.toInt () < 300;
}

int RESTTransceiver::do_start (JTDXDateTime *)
{
  return 0;
}

void RESTTransceiver::do_stop ()
{
}

void RESTTransceiver::do_ptt (bool on)
{
  QJsonObject body;
  body["newValue"] = on ? QStringLiteral ("1") : QStringLiteral ("0");
  httpPost (restUrl (QStringLiteral ("/ptt")),
            QJsonDocument {body}.toJson (QJsonDocument::Compact));
  update_PTT (on);
}

void RESTTransceiver::do_frequency (Frequency f, MODE m, bool)
{
  QJsonObject body;
  body["newValue"] = QString::number (f);
  if (!httpPost (restUrl (QStringLiteral ("/frequency")),
                 QJsonDocument {body}.toJson (QJsonDocument::Compact)))
    {
      return;
    }
  update_rx_frequency (f);

  if (UNK != m)
    {
      QJsonObject modeBody;
      modeBody["mode"]     = "USB";
      modeBody["passband"] = "2500";
      httpPost (restUrl (QStringLiteral ("/mode")),
                QJsonDocument {modeBody}.toJson (QJsonDocument::Compact));
      update_mode (m);
    }
}

void RESTTransceiver::do_mode (MODE m)
{
  // Mode is set via do_frequency when a non-UNK mode is passed.
  // For standalone mode changes, set via REST API.
  QJsonObject body;
  body["mode"]     = "USB";
  body["passband"] = "2500";
  httpPost (restUrl (QStringLiteral ("/mode")),
            QJsonDocument {body}.toJson (QJsonDocument::Compact));
  update_mode (m);
}

void RESTTransceiver::do_tx_frequency (Frequency f, MODE, bool)
{
  if (f)
    {
      QJsonObject body;
      body["newValue"] = QString::number (f);
      if (httpPost (restUrl (QStringLiteral ("/split_frequency")),
                    QJsonDocument {body}.toJson (QJsonDocument::Compact)))
        {
          update_split (true);
        }
    }
  else
    {
      QJsonObject body;
      body["newValue"] = QStringLiteral ("0");
      httpPost (restUrl (QStringLiteral ("/split_mode")),
                QJsonDocument {body}.toJson (QJsonDocument::Compact));
      update_split (false);
    }
}

void RESTTransceiver::do_poll ()
{
  QString freqResp = httpGet (restUrl (QStringLiteral ("/frequency")));
  if (!freqResp.isEmpty ())
    {
      QJsonDocument doc = QJsonDocument::fromJson (freqResp.toUtf8 ());
      if (doc.isObject () && doc.object ().contains ("freq"))
        {
          Frequency f = doc.object ()["freq"].toString ().toLongLong ();
          if (f > 0) update_rx_frequency (f);
        }
    }

  QString pttResp = httpGet (restUrl (QStringLiteral ("/ptt")));
  if (!pttResp.isEmpty ())
    {
      QJsonDocument doc = QJsonDocument::fromJson (pttResp.toUtf8 ());
      if (doc.isObject () && doc.object ().contains ("ptt"))
        {
          bool on = doc.object ()["ptt"].toString () == QStringLiteral ("1");
          update_PTT (on);
        }
    }
}
