#ifndef WKJTX_ZONE_DETECTOR_HPP
#define WKJTX_ZONE_DETECTOR_HPP

// ZoneDetector — answers "is this decode in a new CQ or ITU zone?"
//
// Maps a DXCC / call / grid to a (CQ zone, ITU zone) tuple using the
// polygon tables in FT8 Card Pro's dxhunter/zones.py (to be ported to
// C++ static arrays in v0.3). Tracks two sets of worked zones.
//
// Skeleton. Implementation in Plan 3 (WKjTX v0.3).

#include <QString>
#include <QSet>

namespace wkjtx {

struct ZoneTuple
{
  int cq {0};     // 0 means unknown
  int itu {0};    // 0 means unknown
};

class ZoneDetector
{
public:
  ZoneDetector ();

  void loadFromAdif (QString const & adifPath);

  void markWorkedCq (int zone);
  void markWorkedItu (int zone);

  bool isNewCqZone (QString const & callsign, QString const & grid) const;
  bool isNewItuZone (QString const & callsign, QString const & grid) const;

  static ZoneTuple zonesFromGrid (QString const & grid);
  static ZoneTuple zonesFromCallsign (QString const & callsign);

private:
  QSet<int> workedCq_;
  QSet<int> workedItu_;
};

} // namespace wkjtx

#endif // WKJTX_ZONE_DETECTOR_HPP
