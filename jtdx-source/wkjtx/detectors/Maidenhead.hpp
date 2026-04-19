#ifndef WKJTX_MAIDENHEAD_HPP
#define WKJTX_MAIDENHEAD_HPP

// Maidenhead (QTH) locator ↔ lat/lon + great-circle distance.
//
// Supports the three common precision levels:
//   4 chars  "JN45"       field + square      ±~56 km
//   6 chars  "JN45ab"     + subsquare         ±~2.3 km
//   8 chars  "JN45ab12"   + extended          ±~230 m
//
// Reference: IARU Region 1 Convention, 1980.
//
// Ported from G:\Claude Local\BACKUP APP\ft8-companion\geo\maidenhead.py.

#include <QString>

namespace wkjtx {

struct LatLon
{
  double lat {0.0};    // -90 .. +90
  double lon {0.0};    // -180 .. +180
  bool valid {false};
};

class Maidenhead
{
public:
  // True iff the string is a well-formed 4/6/8-char locator.
  static bool isValid (QString const & grid);

  // Returns the center of the locator cell in (lat, lon).
  // valid == false if grid is malformed.
  static LatLon toLatLon (QString const & grid);

  // Great-circle distance in km between two lat/lon pairs.
  static double haversineKm (double lat1, double lon1,
                             double lat2, double lon2);
};

} // namespace wkjtx

#endif // WKJTX_MAIDENHEAD_HPP
