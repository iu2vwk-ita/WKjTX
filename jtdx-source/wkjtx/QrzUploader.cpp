#include "QrzUploader.hpp"

// Skeleton — v1.0 (Plan 4) wires this into the QSO logging flow.

#include <QNetworkAccessManager>

namespace wkjtx {

QrzUploader::QrzUploader (QObject * parent)
  : QObject {parent}
  , nam_ {new QNetworkAccessManager {this}}
{}

QrzUploader::~QrzUploader () = default;

void QrzUploader::setApiKey (QString const & key) { apiKey_ = key; }
void QrzUploader::setEnabled (bool on)            { enabled_ = on; }

void QrzUploader::uploadAdif (QString const & /*adifRecord*/)
{
  // TODO v1.0: build x-www-form-urlencoded body with KEY, ACTION=INSERT,
  // ADIF=<record>. POST to https://logbook.qrz.com/api. Parse response
  // ("RESULT=OK" or "RESULT=FAIL&REASON=..."). Emit uploaded or
  // uploadFailed. Retry policy: one retry on network error, then give up
  // for this record.
}

} // namespace wkjtx
