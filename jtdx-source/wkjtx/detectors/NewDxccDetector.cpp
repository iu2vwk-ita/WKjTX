#include "NewDxccDetector.hpp"

// Skeleton. Real CTY.DAT prefix resolution and ADIF parsing arrive
// in Plan 3 (WKjTX v0.3).

namespace wkjtx {

NewDxccDetector::NewDxccDetector () = default;

void NewDxccDetector::loadFromAdif (QString const & /*adifPath*/)
{
  // TODO v0.3: parse ADIF, fill workedDxcc_ set.
}

void NewDxccDetector::markWorked (QString const & dxccPrefix)
{
  workedDxcc_.insert (dxccPrefix);
}

bool NewDxccDetector::isNewDxcc (QString const & callsign) const
{
  QString const dxcc = dxccFromCallsign (callsign);
  if (dxcc.isEmpty ()) return false;   // don't fire on unknown DXCC
  return !workedDxcc_.contains (dxcc);
}

QString NewDxccDetector::dxccFromCallsign (QString const & /*callsign*/)
{
  // TODO v0.3: CTY.DAT-based resolution. Port logic from
  // jtdx-source/LookupData.cpp (existing JTDX code) or from
  // FT8 Card Pro's dxhunter/detector.py.
  return {};
}

} // namespace wkjtx
