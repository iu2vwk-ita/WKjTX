// Unit tests for GridDetector — the only detector with usable logic
// in the v0.1 skeleton (grid validation regex). Other detectors are
// thin stubs and will get tests in Plan 3.

#include "../detectors/GridDetector.hpp"
#include <QTest>

class TestGridDetector : public QObject
{
  Q_OBJECT

private slots:
  void valid_grids_accepted ()
  {
    QVERIFY (wkjtx::GridDetector::isValidGrid4 ("JN45"));
    QVERIFY (wkjtx::GridDetector::isValidGrid4 ("FN42"));
    QVERIFY (wkjtx::GridDetector::isValidGrid4 ("aa00"));  // lowercase ok
    QVERIFY (wkjtx::GridDetector::isValidGrid4 ("RR99"));  // boundary
  }

  void invalid_grids_rejected ()
  {
    QVERIFY (!wkjtx::GridDetector::isValidGrid4 (""));
    QVERIFY (!wkjtx::GridDetector::isValidGrid4 ("JN4"));   // too short
    QVERIFY (!wkjtx::GridDetector::isValidGrid4 ("JN456")); // too long
    QVERIFY (!wkjtx::GridDetector::isValidGrid4 ("JNAB"));  // digits required
    QVERIFY (!wkjtx::GridDetector::isValidGrid4 ("XX12"));  // first letter > R
    QVERIFY (!wkjtx::GridDetector::isValidGrid4 ("11JN"));  // wrong order
  }

  void mark_and_query_round_trip ()
  {
    wkjtx::GridDetector d;
    QVERIFY (d.isNewGrid ("JN45"));
    d.markWorked ("JN45");
    QVERIFY (!d.isNewGrid ("JN45"));
    QVERIFY (!d.isNewGrid ("jn45"));   // case insensitive
    QVERIFY (d.isNewGrid ("FN42"));    // unrelated grid still new
  }

  void invalid_grid_is_not_new ()
  {
    // An invalid grid string cannot be "new" because we refuse to
    // trigger on malformed input — defensive posture for auto-call.
    wkjtx::GridDetector d;
    QVERIFY (!d.isNewGrid ("garbage"));
    QVERIFY (!d.isNewGrid (""));
  }
};

QTEST_MAIN (TestGridDetector)
#include "test_grid_detector.moc"
