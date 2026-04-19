#include "ZoneDetector.hpp"
#include "Maidenhead.hpp"
#include "ZoneTables.hpp"

// Real implementation — ported from FT8 Card Pro's dxhunter/zones.py.
// Uses nearest-center lookup against constexpr CQ/ITU zone tables.

#include <array>
#include <limits>

namespace wkjtx {

namespace {

template<std::size_t N>
int nearestZone (double lat, double lon, std::array<ZoneCenter, N> const & table)
{
  double bestDist = std::numeric_limits<double>::infinity ();
  int bestZone = 0;
  for (auto const & c : table) {
    double const d = Maidenhead::haversineKm (lat, lon, c.lat, c.lon);
    if (d < bestDist) {
      bestDist = d;
      bestZone = c.zoneNumber;
    }
  }
  return bestZone;
}

} // namespace

ZoneDetector::ZoneDetector () = default;

void ZoneDetector::loadFromAdif (QString const & /*adifPath*/)
{
  // TODO v0.3: parse ADIF, extract CQZ and ITUZ fields per QSO, fill
  // workedCq_ / workedItu_. Defer until JTDX's ADIF parser is wired in.
}

void ZoneDetector::markWorkedCq (int zone)
{
  if (zone > 0) workedCq_.insert (zone);
}

void ZoneDetector::markWorkedItu (int zone)
{
  if (zone > 0) workedItu_.insert (zone);
}

ZoneTuple ZoneDetector::zonesFromGrid (QString const & grid)
{
  ZoneTuple t;
  LatLon const ll = Maidenhead::toLatLon (grid);
  if (!ll.valid) return t;
  t.cq  = nearestZone (ll.lat, ll.lon, kCqZoneCenters);
  t.itu = nearestZone (ll.lat, ll.lon, kItuZoneCenters);
  return t;
}

ZoneTuple ZoneDetector::zonesFromCallsign (QString const & /*callsign*/)
{
  // A callsign alone doesn't carry lat/lon. The Python reference falls
  // back to a DXCC-country → representative-grid lookup, which requires
  // the CTY.DAT dataset. Deferred to v0.3 when we wire JTDX's existing
  // LookupData. Until then, callers should prefer zonesFromGrid().
  return {};
}

bool ZoneDetector::isNewCqZone (QString const & /*callsign*/, QString const & grid) const
{
  ZoneTuple const t = const_cast<ZoneDetector *> (this)->zonesFromGrid (grid);
  if (t.cq <= 0) return false;
  return !workedCq_.contains (t.cq);
}

bool ZoneDetector::isNewItuZone (QString const & /*callsign*/, QString const & grid) const
{
  ZoneTuple const t = const_cast<ZoneDetector *> (this)->zonesFromGrid (grid);
  if (t.itu <= 0) return false;
  return !workedItu_.contains (t.itu);
}

} // namespace wkjtx
