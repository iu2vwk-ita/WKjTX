// Unit tests for PrefixDetector — real prefix extraction logic.

#include "../detectors/PrefixDetector.hpp"
#include <QTest>

class TestPrefixDetector : public QObject
{
  Q_OBJECT

private slots:
  void standard_italian_call ()
  {
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("IU2VWK"),
              QString ("IU2"));
  }

  void standard_us_call ()
  {
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("K1ABC"), QString ("K1"));
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("W1AW"),  QString ("W1"));
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("N0CALL"),QString ("N0"));
  }

  void brazil_and_multi_letter_prefix ()
  {
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("PY2XYZ"), QString ("PY2"));
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("PY7DF"),  QString ("PY7"));
  }

  void dx_prefix_on_left_of_slash ()
  {
    // KH6/W1AW — operating from Hawaii. DX prefix KH6 wins.
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("KH6/W1AW"),
              QString ("KH6"));
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("VP9/K1ABC"),
              QString ("VP9"));
  }

  void short_qualifier_on_right_of_slash ()
  {
    // IU2VWK/P, IU2VWK/M — "/P" and "/M" are qualifiers (portable,
    // mobile). The prefix is still IU2.
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("IU2VWK/P"),
              QString ("IU2"));
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("IU2VWK/M"),
              QString ("IU2"));
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("IU2VWK/MM"),
              QString ("IU2"));
  }

  void long_right_side_is_a_dx_visit ()
  {
    // JA1ABC/VE3 = Japanese ham operating in Canada (VE3). The active
    // prefix is VE3.
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("JA1ABC/VE3"),
              QString ("VE3"));
  }

  void empty_input_returns_empty ()
  {
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign (""), QString (""));
  }

  void lowercase_input_normalized ()
  {
    QCOMPARE (wkjtx::PrefixDetector::prefixFromCallsign ("iu2vwk"),
              QString ("IU2"));
  }

  void mark_and_query_round_trip ()
  {
    wkjtx::PrefixDetector d;
    QVERIFY (d.isNewPrefix ("IU2VWK"));
    d.markWorked ("IU2");
    QVERIFY (!d.isNewPrefix ("IU2VWK"));
    QVERIFY (!d.isNewPrefix ("IU2ABC"));   // same prefix
    QVERIFY (d.isNewPrefix ("K1ABC"));      // different prefix
  }
};

QTEST_MAIN (TestPrefixDetector)
#include "test_prefix_detector.moc"
