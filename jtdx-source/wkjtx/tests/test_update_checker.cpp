// Version comparison used by the GitHub release check. The network path
// is not tested here — this locks the ordering rules that decide whether
// a user is told an update exists.

#include <QtTest>

#include "UpdateChecker.hpp"

using wkjtx::compareVersions;

class TestUpdateChecker : public QObject
{
  Q_OBJECT

private slots:
  void newer_patch_is_newer ()
  {
    QVERIFY (compareVersions ("1.3.0", "1.3.1") < 0);
    QVERIFY (compareVersions ("1.3.1", "1.3.0") > 0);
  }

  void newer_minor_and_major ()
  {
    QVERIFY (compareVersions ("1.3.9", "1.4.0") < 0);
    QVERIFY (compareVersions ("1.9.9", "2.0.0") < 0);
  }

  void leading_v_is_ignored ()
  {
    QCOMPARE (compareVersions ("1.3.0", "v1.3.0"), 0);
    QVERIFY (compareVersions ("1.3.0", "v1.4.0") < 0);
  }

  void missing_fields_count_as_zero ()
  {
    QCOMPARE (compareVersions ("1.4", "1.4.0"), 0);
    QVERIFY (compareVersions ("1.4", "1.4.1") < 0);
  }

  void release_candidate_sorts_before_release ()
  {
    QVERIFY (compareVersions ("1.4.0-rc1", "1.4.0") < 0);
    QVERIFY (compareVersions ("1.4.0", "1.4.0-rc1") > 0);
    QVERIFY (compareVersions ("1.4.0-rc1", "1.4.0-rc2") < 0);
  }

  void double_digit_fields_compare_numerically ()
  {
    // The bug this guards: string comparison puts "10" before "9".
    QVERIFY (compareVersions ("1.9.0", "1.10.0") < 0);
    QVERIFY (compareVersions ("1.3.9", "1.3.10") < 0);
  }
};

QTEST_MAIN (TestUpdateChecker)
#include "test_update_checker.moc"
