#ifndef WKJTX_CLUBLOG_UPLOADER_HPP
#define WKJTX_CLUBLOG_UPLOADER_HPP

// ClubLogUploader — v1.4.0. Real-time single-QSO upload to Club Log.
//
// POST https://clublog.org/realtime.php, form-urlencoded:
//     email=<account e-mail>&password=<application password>
//    &callsign=<log callsign>&api=<api key>&adif=<one ADIF record>
//
// Success is signalled by HTTP 200; any other status carries an
// operator-readable message in the body.
//
// Club Log's own API notes are explicit about two things and this
// class honours both:
//   - realtime.php is for QSOs made at operator pace, never for batch
//     uploads (the queue flush therefore stays paced by the
//     dispatcher, same as qrz.com).
//   - on HTTP 403 the client must stop sending with those credentials
//     or the IP gets firewalled, so an auth rejection latches this
//     uploader off until the credentials are set again.
//
// Reference:
//   https://clublog.freshdesk.com/support/solutions/articles/54906
//   -how-to-upload-qsos-in-real-time

#include <QObject>
#include <QString>

class QNetworkAccessManager;

namespace wkjtx {

class ClubLogUploader : public QObject
{
  Q_OBJECT

public:
  explicit ClubLogUploader (QObject * parent = nullptr);
  ~ClubLogUploader () override;

  // Any change here clears a previous auth-rejection latch.
  void setCredentials (QString const & email,
                       QString const & password,
                       QString const & callsign,
                       QString const & apiKey);
  void setEnabled (bool on);

  // True once Club Log answered 403 — no further request is sent until
  // setCredentials() is called again.
  bool authRejected () const { return authRejected_; }

  void uploadAdif (QString const & adifRecord);

signals:
  void uploaded (QString callsign);
  void uploadFailed (QString callsign, QString error);

private:
  QString email_;
  QString password_;
  QString logCallsign_;
  QString apiKey_;
  bool enabled_ {false};
  bool authRejected_ {false};
  QNetworkAccessManager * nam_ {nullptr};
};

} // namespace wkjtx

#endif // WKJTX_CLUBLOG_UPLOADER_HPP
