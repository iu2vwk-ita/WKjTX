#ifndef REST_TRANSCEIVER_HPP__
#define REST_TRANSCEIVER_HPP__

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

#include "PollingTransceiver.hpp"
#include "TransceiverFactory.hpp"

class RESTTransceiver
  : public PollingTransceiver
{
  Q_OBJECT;

public:
  RESTTransceiver (QString const& host, quint16 port, int trx_id,
                   int poll_interval, QObject * parent = nullptr);

  static void register_transceivers (TransceiverFactory::Transceivers * registry, unsigned id);

protected:
  int do_start (JTDXDateTime*) override;
  void do_stop () override;
  void do_ptt (bool on) override;
  void do_frequency (Frequency f, MODE m, bool no_ignore) override;
  void do_tx_frequency (Frequency f, MODE m, bool no_ignore) override;
  void do_mode (MODE m) override;
  void do_poll () override;

private:
  QString restUrl (QString const& path) const;
  QString httpGet (QString const& url) const;
  bool httpPost (QString const& url, QByteArray const& body) const;

  QNetworkAccessManager * net_;
  QString host_;
  quint16 port_;
  int trx_id_;
};

#endif
