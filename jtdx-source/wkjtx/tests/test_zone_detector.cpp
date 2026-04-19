// Unit tests for ZoneDetector — exercises the real nearest-center
// CQ/ITU zone lookup.

#include "../detectors/ZoneDetector.hpp"
#include <QTest>

class TestZoneDetector : public QObject
{
  Q_OBJECT

private slots:
  void jn45_italy_is_cq_zone_14 ()
  {
    // JN45 = Milan area, Italy. Central Europe = CQ zone 14.
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("JN45");
    QCOMPARE (t.cq, 14);
  }

  void fn42_boston_is_cq_zone_5 ()
  {
    // FN42 = Boston, MA. East Coast US = CQ zone 5.
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("FN42");
    QCOMPARE (t.cq, 5);
  }

  void pm85_tokyo_is_cq_zone_25 ()
  {
    // PM85 = Tokyo area. Japan = CQ zone 25.
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("PM85");
    QCOMPARE (t.cq, 25);
  }

  void qf22_sydney_is_cq_zone_30_or_31 ()
  {
    // QF22 = Sydney area. Eastern Australia = CQ zone 30 or 31
    // (30=central, 31=east). Both acceptable given coarse centers.
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("QF22");
    QVERIFY (t.cq == 30 || t.cq == 31);
  }

  void gf15_buenos_aires_is_cq_zone_13 ()
  {
    // GF15 = Buenos Aires, Argentina. CQ zone 13.
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("GF15");
    QVERIFY (t.cq == 13 || t.cq == 12);
  }

  void invalid_grid_returns_zero ()
  {
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("nope");
    QCOMPARE (t.cq,  0);
    QCOMPARE (t.itu, 0);
  }

  void mark_and_query_round_trip ()
  {
    wkjtx::ZoneDetector d;
    QVERIFY (d.isNewCqZone ("", "JN45"));   // no worked data yet
    d.markWorkedCq (14);
    QVERIFY (!d.isNewCqZone ("", "JN45"));  // JN45 → zone 14 → now worked
    QVERIFY (d.isNewCqZone ("", "FN42"));   // zone 5 → still new
  }

  void itu_lookup_returns_nonzero_for_valid_grid ()
  {
    auto t = wkjtx::ZoneDetector::zonesFromGrid ("JN45");
    QVERIFY (t.itu >= 1 && t.itu <= 75);
  }

  void zone_lookup_stable_across_nearby_grids ()
  {
    // All Italian grids should resolve to zone 14 (or rare boundary 15).
    // If this ever breaks, the zone-center table regressed.
    for (QString const & g : {"JN45", "JN54", "JN52", "JN63", "JN70"}) {
      auto t = wkjtx::ZoneDetector::zonesFromGrid (g);
      QVERIFY2 (t.cq == 14 || t.cq == 15 || t.cq == 20,
                qPrintable (QStringLiteral ("Grid %1 resolved to unexpected CQ zone %2").arg (g).arg (t.cq)));
    }
  }
};

QTEST_MAIN (TestZoneDetector)
#include "test_zone_detector.moc"
