#include "PrefixDetector.hpp"

namespace wkjtx {

PrefixDetector::PrefixDetector () = default;

void PrefixDetector::loadFromAdif (QString const & /*adifPath*/) {}

void PrefixDetector::markWorked (QString const & prefix)
{
  if (!prefix.isEmpty ()) workedPrefixes_.insert (prefix.toUpper ());
}

bool PrefixDetector::isNewPrefix (QString const & callsign) const
{
  QString const p = prefixFromCallsign (callsign);
  if (p.isEmpty ()) return false;
  return !workedPrefixes_.contains (p);
}

QString PrefixDetector::prefixFromCallsign (QString const & /*callsign*/)
{
  // TODO v0.3: extract leading alphanumeric prefix up to the call-body
  // transition. E.g. "IU2VWK" → "IU2"; "K1ABC" → "K1"; "PY2XYZ" → "PY2".
  return {};
}

} // namespace wkjtx
