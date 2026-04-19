#include "ZoneDetector.hpp"

// Skeleton — v0.3 will port the polygon tables and lookup logic.

namespace wkjtx {

ZoneDetector::ZoneDetector () = default;

void ZoneDetector::loadFromAdif (QString const & /*adifPath*/) {}

void ZoneDetector::markWorkedCq (int zone)
{
  if (zone > 0) workedCq_.insert (zone);
}

void ZoneDetector::markWorkedItu (int zone)
{
  if (zone > 0) workedItu_.insert (zone);
}

bool ZoneDetector::isNewCqZone (QString const & /*callsign*/, QString const & /*grid*/) const
{
  return false;  // v0.3
}

bool ZoneDetector::isNewItuZone (QString const & /*callsign*/, QString const & /*grid*/) const
{
  return false;  // v0.3
}

ZoneTuple ZoneDetector::zonesFromGrid (QString const & /*grid*/)
{
  return {};  // v0.3
}

ZoneTuple ZoneDetector::zonesFromCallsign (QString const & /*callsign*/)
{
  return {};  // v0.3
}

} // namespace wkjtx
