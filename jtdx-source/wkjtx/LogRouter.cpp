#include "LogRouter.hpp"

// Skeleton — v1.0 (Plan 4) wires this into the QSO logging flow.

#include <QFile>
#include <QTextStream>

namespace wkjtx {

LogRouter::LogRouter (QObject * parent)
  : QObject {parent}
{}

void LogRouter::setProfileLogPath (QString const & path) { profilePath_ = path; }
void LogRouter::setSharedLogPath (QString const & path)  { sharedPath_ = path; }

bool LogRouter::logQso (QsoRecord const & qso)
{
  QString const & target = profilePath_.isEmpty () ? sharedPath_ : profilePath_;
  return appendToFile (target, qso.adifLine);
}

bool LogRouter::appendToFile (QString const & path, QString const & adifLine)
{
  if (path.isEmpty ()) return false;
  QFile f {path};
  if (!f.open (QIODevice::Append | QIODevice::Text)) {
    emit logFailed (path, f.errorString ());
    return false;
  }
  QTextStream ts {&f};
  ts << adifLine << '\n';
  return true;
}

} // namespace wkjtx
