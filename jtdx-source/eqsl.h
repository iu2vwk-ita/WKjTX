// This source code file was last time modified by Arvo ES1JA on 20181229
// v1.2.0 (WKjTX): signal-based success/failure reporting so the
// UploadDispatcher can queue retries.

#ifndef EQSL_H
#define EQSL_H

#include <QObject>
#include <QString>
#include "Radio.hpp"


class QNetworkAccessManager;
class QTimer;
class QNetworkReply;

class EQSL : public QObject
{
  Q_OBJECT;

public:
  explicit EQSL(QNetworkAccessManager *, QObject *parent = nullptr);
    void upload(QString const& eqsl_username, QString const& eqsl_passwd, QString const& eqsl_nickname
                  , QString const& call, QString const& mode
                  , QDateTime const& QSO_date_on
                  , QString const& rpt_sent, QString const& band
                  , QString const& eqslcomments);

signals:
    void uploaded     (QString callsign);
    void uploadFailed (QString callsign, QString error);

public slots:
    void networkReply(QNetworkReply *);
    void work();

private:
    QNetworkAccessManager *networkManager;
    QString myadif;
    QTimer *uploadTimer;
    QNetworkReply *reply;
    bool m_in_progress;
    // v1.2.0: remember the callsign whose upload is in flight so we can
    // route the success / failure signal to the right entry in the
    // UploadDispatcher.
    QString m_pending_call;
};

#endif // EQSL_H
