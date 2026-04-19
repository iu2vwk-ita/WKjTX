#ifndef WKJTX_QRZ_UPLOADER_HPP
#define WKJTX_QRZ_UPLOADER_HPP

// QrzUploader — HTTPS ADIF upload to qrz.com Logbook API.
//
// POST https://logbook.qrz.com/api with form-urlencoded body:
//     KEY=<api_key>&ACTION=INSERT&ADIF=<adif_record>
//
// qrz.com Logbook API reference:
//   https://www.qrz.com/docs/logbook/QRZLogbookAPI.html
//
// Skeleton. Implementation in Plan 4 (WKjTX v1.0). Before implementing,
// check if JTDX 2.2.159 already has a qrz.com upload somewhere; if yes,
// reuse it and just expose the credentials per-profile.

#include <QObject>
#include <QString>

class QNetworkAccessManager;

namespace wkjtx {

class QrzUploader : public QObject
{
  Q_OBJECT

public:
  explicit QrzUploader (QObject * parent = nullptr);
  ~QrzUploader () override;

  void setApiKey (QString const & key);
  void setEnabled (bool on);

  // Upload a single ADIF record. Non-blocking; result arrives via
  // one of the signals below.
  void uploadAdif (QString const & adifRecord);

signals:
  void uploaded (QString callsign);     // success
  void uploadFailed (QString callsign, QString error);

private:
  QString apiKey_;
  bool enabled_ {false};
  QNetworkAccessManager * nam_ {nullptr};
};

} // namespace wkjtx

#endif // WKJTX_QRZ_UPLOADER_HPP
