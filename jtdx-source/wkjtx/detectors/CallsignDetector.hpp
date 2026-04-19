#ifndef WKJTX_CALLSIGN_DETECTOR_HPP
#define WKJTX_CALLSIGN_DETECTOR_HPP

// CallsignDetector — answers "have I ever worked this exact callsign?"
//
// Skeleton. Implementation in Plan 3 (WKjTX v0.3).

#include <QString>
#include <QSet>

namespace wkjtx {

class CallsignDetector
{
public:
  CallsignDetector ();

  void loadFromAdif (QString const & adifPath);
  void markWorked (QString const & callsign);
  bool isNewCallsign (QString const & callsign) const;

private:
  QSet<QString> workedCalls_;
};

} // namespace wkjtx

#endif // WKJTX_CALLSIGN_DETECTOR_HPP
