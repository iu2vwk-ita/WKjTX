#ifndef WKJTX_LOG_ROUTER_HPP
#define WKJTX_LOG_ROUTER_HPP

// LogRouter — decides whether a logged QSO goes to the shared global
// ADIF log (JTDX default) or to a per-profile ADIF file, based on the
// active profile's "logPath" setting.
//
// Empty logPath → shared log.
// Non-empty → that specific file.
//
// Skeleton. Implementation in Plan 4 (WKjTX v1.0).

#include <QObject>
#include <QString>

namespace wkjtx {

struct QsoRecord
{
  QString callsign;
  QString grid;
  QString mode;
  QString band;
  QString dateUtc;    // YYYYMMDD
  QString timeUtc;    // HHMM
  QString adifLine;   // complete serialized ADIF record, ready to append
};

class LogRouter : public QObject
{
  Q_OBJECT

public:
  explicit LogRouter (QObject * parent = nullptr);

  // Set by MainWindow on profile switch. Empty = use shared log.
  void setProfileLogPath (QString const & path);
  void setSharedLogPath (QString const & path);

  // Append a QSO record to the appropriate destination file.
  // Returns true on success, false on I/O error.
  bool logQso (QsoRecord const & qso);

signals:
  void logFailed (QString path, QString error);

private:
  QString profilePath_;
  QString sharedPath_;

  bool appendToFile (QString const & path, QString const & adifLine);
};

} // namespace wkjtx

#endif // WKJTX_LOG_ROUTER_HPP
