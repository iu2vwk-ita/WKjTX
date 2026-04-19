// End-to-end unit tests for the AutoCall pipeline including real
// detector integration (GridDetector, PrefixDetector, ZoneDetector).

#include "../AutoCall.hpp"
#include "../detectors/GridDetector.hpp"
#include "../detectors/PrefixDetector.hpp"
#include "../detectors/ZoneDetector.hpp"
#include "../detectors/NewDxccDetector.hpp"
#include "../detectors/CallsignDetector.hpp"

#include <QTest>
#include <QSignalSpy>
#include <QDateTime>

class TestAutoCallPipeline : public QObject
{
  Q_OBJECT

private slots:

  void not_armed_means_no_fire ()
  {
    wkjtx::AutoCall ac;
    wkjtx::Decode d;
    d.callsign = "K1ABC";
    d.grid4 = "FN42";
    d.utc = QDateTime::currentDateTimeUtc ();
    QSignalSpy spy {&ac, &wkjtx::AutoCall::callRequested};
    ac.onDecode (d);
    QCOMPARE (spy.count (), 0);
  }

  void alert_category_fires_on_matching_callsign ()
  {
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::Alert] = true;
    cfg.alertCallsigns = {"K1ABC", "DL1XYZ"};
    ac.setConfig (cfg);

    wkjtx::Decode d;
    d.callsign = "K1ABC";
    d.utc = QDateTime::currentDateTimeUtc ();
    QSignalSpy spy {&ac, &wkjtx::AutoCall::callRequested};
    ac.onDecode (d);
    QCOMPARE (spy.count (), 1);
  }

  void alert_does_not_fire_on_non_alert_call ()
  {
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::Alert] = true;
    cfg.alertCallsigns = {"K1ABC"};
    ac.setConfig (cfg);

    wkjtx::Decode d;
    d.callsign = "W2XYZ";
    d.utc = QDateTime::currentDateTimeUtc ();
    QSignalSpy spy {&ac, &wkjtx::AutoCall::callRequested};
    ac.onDecode (d);
    QCOMPARE (spy.count (), 0);
  }

  void new_grid_fires_then_cooldown_suppresses_repeat ()
  {
    wkjtx::AutoCall ac;
    wkjtx::GridDetector gd;
    ac.setDetectors (nullptr, nullptr, &gd, nullptr, nullptr);

    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::NewGrid] = true;
    ac.setConfig (cfg);

    wkjtx::Decode d;
    d.callsign = "K1ABC";
    d.grid4 = "JN45";
    d.utc = QDateTime::currentDateTimeUtc ();

    QSignalSpy fire {&ac, &wkjtx::AutoCall::callRequested};
    QSignalSpy sup  {&ac, &wkjtx::AutoCall::suppressed};

    // First decode: should fire.
    ac.onDecode (d);
    QCOMPARE (fire.count (), 1);
    QCOMPARE (sup.count (),  0);

    // Same callsign, within 120s → suppressed by per-call cooldown.
    ac.onDecode (d);
    QCOMPARE (fire.count (), 1);
    QCOMPARE (sup.count (),  1);
  }

  void rate_limit_suppresses_4th_in_60_seconds ()
  {
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::Alert] = true;
    cfg.alertCallsigns = {"AA1A", "BB2B", "CC3C", "DD4D"};
    ac.setConfig (cfg);

    QSignalSpy fire {&ac, &wkjtx::AutoCall::callRequested};
    QSignalSpy sup  {&ac, &wkjtx::AutoCall::suppressed};

    QDateTime const t0 = QDateTime::currentDateTimeUtc ();
    for (QString const & c : QStringList {"AA1A", "BB2B", "CC3C", "DD4D"}) {
      wkjtx::Decode d;
      d.callsign = c;
      d.utc = t0;
      ac.onDecode (d);
    }

    QCOMPARE (fire.count (), 3);  // first 3 fire
    QCOMPARE (sup.count (),  1);  // 4th suppressed by global rate limit
  }

  void priority_alert_beats_new_grid_when_both_match ()
  {
    wkjtx::AutoCall ac;
    wkjtx::GridDetector gd;
    ac.setDetectors (nullptr, nullptr, &gd, nullptr, nullptr);

    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::Alert]   = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::NewGrid] = true;
    cfg.alertCallsigns = {"K1ABC"};
    ac.setConfig (cfg);

    wkjtx::Decode d;
    d.callsign = "K1ABC";
    d.grid4 = "JN45";    // new grid too
    d.utc = QDateTime::currentDateTimeUtc ();

    wkjtx::AutoCallCategory receivedCat = wkjtx::AutoCallCategory::NewCallsign;
    connect (&ac, &wkjtx::AutoCall::callRequested,
             this, [&receivedCat](wkjtx::Decode, wkjtx::AutoCallCategory c)
             { receivedCat = c; });

    ac.onDecode (d);
    // Alert has priority #1; even though the grid is also "new",
    // the pipeline fires the Alert category.
    QCOMPARE (receivedCat, wkjtx::AutoCallCategory::Alert);
  }

  void setConfig_clears_cooldown_on_profile_switch ()
  {
    wkjtx::AutoCall ac;
    wkjtx::AutoCallConfig cfg;
    cfg.masterEnable = true;
    cfg.categoryEnabled[wkjtx::AutoCallCategory::Alert] = true;
    cfg.alertCallsigns = {"K1ABC"};
    ac.setConfig (cfg);

    wkjtx::Decode d;
    d.callsign = "K1ABC";
    d.utc = QDateTime::currentDateTimeUtc ();
    QSignalSpy fire {&ac, &wkjtx::AutoCall::callRequested};

    ac.onDecode (d);
    QCOMPARE (fire.count (), 1);

    ac.onDecode (d);
    QCOMPARE (fire.count (), 1);  // suppressed by cooldown

    // Profile switch: new config, cooldown history cleared.
    ac.setConfig (cfg);

    ac.onDecode (d);
    QCOMPARE (fire.count (), 2);  // cooldown was cleared
  }
};

QTEST_MAIN (TestAutoCallPipeline)
#include "test_autocall_pipeline.moc"
