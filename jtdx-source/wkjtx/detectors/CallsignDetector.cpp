#include "CallsignDetector.hpp"

namespace wkjtx {

CallsignDetector::CallsignDetector () = default;

void CallsignDetector::loadFromAdif (QString const & /*adifPath*/) {}

void CallsignDetector::markWorked (QString const & callsign)
{
  if (!callsign.isEmpty ()) workedCalls_.insert (callsign.toUpper ());
}

bool CallsignDetector::isNewCallsign (QString const & callsign) const
{
  if (callsign.isEmpty ()) return false;
  return !workedCalls_.contains (callsign.toUpper ());
}

} // namespace wkjtx
