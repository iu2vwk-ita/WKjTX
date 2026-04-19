// Unit tests for Maidenhead locator utility.

#include "../detectors/Maidenhead.hpp"
#include <QTest>
#include <cmath>

class TestMaidenhead : public QObject
{
  Q_OBJECT

private slots:
  void isValid_accepts_4_6_8_char_grids ()
  {
    QVERIFY (wkjtx::Maidenhead::isValid ("JN45"));
    QVERIFY (wkjtx::Maidenhead::isValid ("JN45ab"));
    QVERIFY (wkjtx::Maidenhead::isValid ("JN45ab12"));
    QVERIFY (wkjtx::Maidenhead::isValid ("aa00"));     // min field chars
    QVERIFY (wkjtx::Maidenhead::isValid ("RR99"));     // max field chars
    QVERIFY (wkjtx::Maidenhead::isValid ("AR99xx99")); // mixed precision
  }

  void isValid_rejects_malformed ()
  {
    QVERIFY (!wkjtx::Maidenhead::isValid (""));
    QVERIFY (!wkjtx::Maidenhead::isValid ("J"));
    QVERIFY (!wkjtx::Maidenhead::isValid ("JN4"));
    QVERIFY (!wkjtx::Maidenhead::isValid ("JN45a"));    // 5 = invalid length
    QVERIFY (!wkjtx::Maidenhead::isValid ("JN45ab1"));  // 7 = invalid length
    QVERIFY (!wkjtx::Maidenhead::isValid ("XX12"));     // first char > R
    QVERIFY (!wkjtx::Maidenhead::isValid ("45JN"));     // wrong order
    QVERIFY (!wkjtx::Maidenhead::isValid ("JNAB"));     // digits required
  }

  void isValid_rejects_subsquare_out_of_range ()
  {
    QVERIFY (!wkjtx::Maidenhead::isValid ("JN45yy"));  // y > x, invalid
    QVERIFY (!wkjtx::Maidenhead::isValid ("JN45a9"));  // digit in subsquare pos
  }

  void toLatLon_reference_points ()
  {
    // JN45: Italy, Milan area. Expected approx lat=45.5, lon=9.0
    auto ll = wkjtx::Maidenhead::toLatLon ("JN45");
    QVERIFY (ll.valid);
    QVERIFY (std::abs (ll.lat - 45.5) < 0.6);
    QVERIFY (std::abs (ll.lon -  9.0) < 1.1);
  }

  void toLatLon_fn42_boston ()
  {
    // FN42: Boston, MA, USA. Expected approx lat=42.5, lon=-71.0
    auto ll = wkjtx::Maidenhead::toLatLon ("FN42");
    QVERIFY (ll.valid);
    QVERIFY (std::abs (ll.lat - 42.5) < 0.6);
    QVERIFY (std::abs (ll.lon - (-71.0)) < 1.1);
  }

  void toLatLon_aa00_south_pole_pacific ()
  {
    // AA00: (-90..-89, -180..-178) intersection — deep south pacific
    auto ll = wkjtx::Maidenhead::toLatLon ("AA00");
    QVERIFY (ll.valid);
    QVERIFY (ll.lat < -85.0);
    QVERIFY (ll.lon < -175.0);
  }

  void toLatLon_precision_6_narrows_8_narrows_further ()
  {
    auto ll4 = wkjtx::Maidenhead::toLatLon ("JN45");
    auto ll6 = wkjtx::Maidenhead::toLatLon ("JN45ab");
    auto ll8 = wkjtx::Maidenhead::toLatLon ("JN45ab12");
    QVERIFY (ll4.valid && ll6.valid && ll8.valid);
    // All three should land inside the JN45 2x1 deg cell centered ~45.5,9.0
    for (auto const & ll : {ll4, ll6, ll8}) {
      QVERIFY (ll.lat >= 45.0 && ll.lat <= 46.0);
      QVERIFY (ll.lon >=  8.0 && ll.lon <= 10.0);
    }
  }

  void toLatLon_invalid_returns_invalid_struct ()
  {
    auto ll = wkjtx::Maidenhead::toLatLon ("garbage");
    QVERIFY (!ll.valid);
  }

  void haversine_zero_for_identical ()
  {
    double d = wkjtx::Maidenhead::haversineKm (45.0, 9.0, 45.0, 9.0);
    QVERIFY (d < 0.001);
  }

  void haversine_milano_nyc_roughly_6400km ()
  {
    // Milano (45.46, 9.18) to NYC (40.71, -74.01). Expected ~6490 km.
    double d = wkjtx::Maidenhead::haversineKm (45.46, 9.18, 40.71, -74.01);
    QVERIFY (d > 6400.0 && d < 6600.0);
  }

  void haversine_antipodes_half_earth_circumference ()
  {
    // London (51.5, 0) to its antipode (-51.5, 180) should be ~20015 km
    // (half circumference). Haversine max is π * R = 20015.
    double d = wkjtx::Maidenhead::haversineKm (51.5, 0.0, -51.5, 180.0);
    QVERIFY (d > 19900.0 && d < 20100.0);
  }
};

QTEST_MAIN (TestMaidenhead)
#include "test_maidenhead.moc"
