#include "Maidenhead.hpp"

#include <QtGlobal>
#include <cmath>

namespace wkjtx {

namespace {

constexpr double kEarthRadiusKm = 6371.0;

bool is_upper_alpha_AR (QChar c)
{
  // Field chars: A..R (18 fields × 10°/20°)
  return c.toLatin1 () >= 'A' && c.toLatin1 () <= 'R';
}

bool is_upper_alpha_AX (QChar c)
{
  // Subsquare chars: A..X (24 subsquares × 2.5'/5')
  return c.toLatin1 () >= 'A' && c.toLatin1 () <= 'X';
}

bool is_digit (QChar c)
{
  return c.toLatin1 () >= '0' && c.toLatin1 () <= '9';
}

} // namespace

bool Maidenhead::isValid (QString const & grid)
{
  QString const g = grid.trimmed ().toUpper ();
  int const n = g.size ();
  if (n != 4 && n != 6 && n != 8) return false;

  if (!is_upper_alpha_AR (g[0]) || !is_upper_alpha_AR (g[1])) return false;
  if (!is_digit (g[2]) || !is_digit (g[3])) return false;

  if (n >= 6) {
    if (!is_upper_alpha_AX (g[4]) || !is_upper_alpha_AX (g[5])) return false;
  }
  if (n >= 8) {
    if (!is_digit (g[6]) || !is_digit (g[7])) return false;
  }
  return true;
}

LatLon Maidenhead::toLatLon (QString const & grid)
{
  LatLon out;
  if (!isValid (grid)) return out;

  QString const g = grid.trimmed ().toUpper ();

  // Field (20° lon × 10° lat)
  double lon = (g[0].toLatin1 () - 'A') * 20.0 - 180.0;
  double lat = (g[1].toLatin1 () - 'A') * 10.0 -  90.0;

  // Square (2° lon × 1° lat)
  lon += (g[2].toLatin1 () - '0') * 2.0;
  lat += (g[3].toLatin1 () - '0') * 1.0;

  int const n = g.size ();
  if (n >= 6) {
    // Subsquare (5' lon × 2.5' lat)
    lon += (g[4].toLatin1 () - 'A') * (2.0 / 24.0);
    lat += (g[5].toLatin1 () - 'A') * (1.0 / 24.0);
    if (n >= 8) {
      // Extended (30" lon × 15" lat)
      lon += (g[6].toLatin1 () - '0') * (2.0 / 24.0 / 10.0);
      lat += (g[7].toLatin1 () - '0') * (1.0 / 24.0 / 10.0);
      // Center of the extended cell.
      lon += (2.0 / 24.0 / 10.0) / 2.0;
      lat += (1.0 / 24.0 / 10.0) / 2.0;
    } else {
      // Center of subsquare.
      lon += (2.0 / 24.0) / 2.0;
      lat += (1.0 / 24.0) / 2.0;
    }
  } else {
    // Center of square.
    lon += 2.0 / 2.0;
    lat += 1.0 / 2.0;
  }

  out.lat = lat;
  out.lon = lon;
  out.valid = true;
  return out;
}

double Maidenhead::haversineKm (double lat1, double lon1,
                                double lat2, double lon2)
{
  constexpr double kDeg2Rad = 0.017453292519943295; // π/180
  double const r1 = lat1 * kDeg2Rad;
  double const r2 = lat2 * kDeg2Rad;
  double const dLat = (lat2 - lat1) * kDeg2Rad;
  double const dLon = (lon2 - lon1) * kDeg2Rad;
  double const a = std::sin (dLat / 2.0) * std::sin (dLat / 2.0)
                 + std::cos (r1) * std::cos (r2)
                   * std::sin (dLon / 2.0) * std::sin (dLon / 2.0);
  double const c = 2.0 * std::asin (std::min (1.0, std::sqrt (a)));
  return kEarthRadiusKm * c;
}

} // namespace wkjtx
