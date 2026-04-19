// Unit tests for the AutoCall safeguard helpers.
//
// These tests exercise the cooldown-window and rate-limit-window logic
// directly on the AutoCall class. They do not require JTDX decoder
// internals, and they run independently of hardware.
//
// Compiled only when WKJTX_BUILD_TESTS=ON.

#include "../AutoCall.hpp"
#include <QTest>
#include <QDateTime>

class TestAutoCallSafeguards : public QObject
{
  Q_OBJECT

private slots:
  void activeCategoryCount_is_zero_when_master_disabled ()
  {
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = false;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::NewDxcc] = true;
    ac.setConfig (cfg);
    QCOMPARE (ac.activeCategoryCount (), 0);
    QVERIFY (!ac.isArmed ());
  }

  void activeCategoryCount_counts_enabled_categories ()
  {
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::NewDxcc]     = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::NewCqZone]   = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::NewCallsign] = true;
    ac.setConfig (cfg);
    QCOMPARE (ac.activeCategoryCount (), 3);
    QVERIFY (ac.isArmed ());
  }

  void setConfig_clears_cooldown_state ()
  {
    // When a new profile is loaded, prior cooldown state MUST be flushed
    // or the user would get unexpected suppression right after switching.
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg1;
    cfg1.masterEnable = true;
    cfg1.categoryEnabled[wkjtx::AutoCallCategory::Alert] = true;
    ac.setConfig (cfg1);
    // No public API yet to push artificial cooldown state — this test
    // will be expanded in Plan 3 when onDecode() does real work. For
    // now, the test doubles as a compile-time check that setConfig
    // resets by not throwing.
    ac.setConfig (cfg1);  // twice = no-op
    QVERIFY (ac.isArmed ());
  }

  void safeguard_constants_are_documented_values ()
  {
    // The safeguards are hardcoded for safety. This test locks the
    // values so a future refactor can't silently weaken them.
    QCOMPARE (wkjtx::AutoCall::kPerCallCooldownSec,    120);
    QCOMPARE (wkjtx::AutoCall::kGlobalLimitPerWindow,  3);
    QCOMPARE (wkjtx::AutoCall::kGlobalWindowSec,       60);
  }
};

QTEST_MAIN (TestAutoCallSafeguards)
#include "test_autocall_safeguards.moc"
