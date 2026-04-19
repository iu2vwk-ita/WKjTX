#include "AutoCall.hpp"

#include "detectors/NewDxccDetector.hpp"
#include "detectors/ZoneDetector.hpp"
#include "detectors/GridDetector.hpp"
#include "detectors/PrefixDetector.hpp"
#include "detectors/CallsignDetector.hpp"

// AutoCall pipeline.
//
// Category priority (first match wins, per design doc section 5.3):
//   1. Alert (user's 5 explicit callsigns)
//   2. NewDxcc
//   3. NewCqZone
//   4. NewItuZone
//   5. NewGrid
//   6. NewPrefix
//   7. NewCallsign
//
// Order-independent safeguards apply to every fire:
//   - 120 s per-callsign cooldown (lastCallPerCallsign_)
//   - 3 auto-calls per 60 s global rolling window (recentGlobalCalls_)
//
// Notes:
//   * v0.3 wires this into MainWindow at mainwindow.cpp:3857 (decode
//     display). MainWindow supplies the populated detectors after the
//     startup ADIF scan.
//   * A decode with no callsign never fires.
//   * "New" detectors that depend on data we don't have (e.g. zones
//     without a grid, prefix without a digit in the call) return false
//     rather than fire a false positive — defensive posture for safety.

namespace wkjtx {

AutoCall::AutoCall (QObject * parent)
  : QObject {parent}
{}

AutoCall::~AutoCall () = default;

void AutoCall::setDetectors (NewDxccDetector * dxcc,
                             ZoneDetector * zone,
                             GridDetector * grid,
                             PrefixDetector * prefix,
                             CallsignDetector * callsign)
{
  dxcc_     = dxcc;
  zone_     = zone;
  grid_     = grid;
  prefix_   = prefix;
  callsign_ = callsign;
}

void AutoCall::setConfig (AutoCallConfig const & cfg)
{
  config_ = cfg;
  lastCallPerCallsign_.clear ();
  recentGlobalCalls_.clear ();
  emit configChanged ();
}

int AutoCall::activeCategoryCount () const
{
  if (!config_.masterEnable) return 0;
  int n = 0;
  for (auto it = config_.categoryEnabled.cbegin (); it != config_.categoryEnabled.cend (); ++it) {
    if (it.value ()) ++n;
  }
  return n;
}

bool AutoCall::isArmed () const
{
  return config_.masterEnable && activeCategoryCount () > 0;
}

bool AutoCall::isCategoryOn (AutoCallCategory c) const
{
  if (!config_.masterEnable) return false;
  return config_.categoryEnabled.value (c, false);
}

bool AutoCall::matchesAlert (Decode const & d) const
{
  QString const up = d.callsign.toUpper ();
  for (auto const & alert : config_.alertCallsigns) {
    if (alert.trimmed ().toUpper () == up) return true;
  }
  return false;
}

bool AutoCall::isWithinCooldown (QString const & callsign, QDateTime const & now) const
{
  auto it = lastCallPerCallsign_.constFind (callsign);
  if (it == lastCallPerCallsign_.cend ()) return false;
  return it.value ().secsTo (now) < kPerCallCooldownSec;
}

bool AutoCall::isWithinRateLimit (QDateTime const & now)
{
  // Drop entries outside the 60 s window.
  while (!recentGlobalCalls_.empty ()
         && recentGlobalCalls_.front ().secsTo (now) > kGlobalWindowSec) {
    recentGlobalCalls_.pop_front ();
  }
  return static_cast<int> (recentGlobalCalls_.size ()) >= kGlobalLimitPerWindow;
}

void AutoCall::onDecode (Decode const & d)
{
  if (!isArmed ()) return;
  if (d.callsign.isEmpty ()) return;

  QDateTime const now = d.utc.isValid () ? d.utc : QDateTime::currentDateTimeUtc ();
  QString const call = d.callsign.toUpper ();

  // Run priority pipeline. First match wins; the loop emits the
  // category, then the caller will either fire or suppress.
  AutoCallCategory matched = AutoCallCategory::Alert;
  bool anyMatch = false;

  if (isCategoryOn (AutoCallCategory::Alert) && matchesAlert (d)) {
    matched = AutoCallCategory::Alert;
    anyMatch = true;
  } else if (isCategoryOn (AutoCallCategory::NewDxcc) && dxcc_
             && dxcc_->isNewDxcc (call)) {
    matched = AutoCallCategory::NewDxcc;
    anyMatch = true;
  } else if (isCategoryOn (AutoCallCategory::NewCqZone) && zone_
             && zone_->isNewCqZone (call, d.grid4)) {
    matched = AutoCallCategory::NewCqZone;
    anyMatch = true;
  } else if (isCategoryOn (AutoCallCategory::NewItuZone) && zone_
             && zone_->isNewItuZone (call, d.grid4)) {
    matched = AutoCallCategory::NewItuZone;
    anyMatch = true;
  } else if (isCategoryOn (AutoCallCategory::NewGrid) && grid_
             && grid_->isNewGrid (d.grid4)) {
    matched = AutoCallCategory::NewGrid;
    anyMatch = true;
  } else if (isCategoryOn (AutoCallCategory::NewPrefix) && prefix_
             && prefix_->isNewPrefix (call)) {
    matched = AutoCallCategory::NewPrefix;
    anyMatch = true;
  } else if (isCategoryOn (AutoCallCategory::NewCallsign) && callsign_
             && callsign_->isNewCallsign (call)) {
    matched = AutoCallCategory::NewCallsign;
    anyMatch = true;
  }

  if (!anyMatch) return;

  // Safeguards.
  if (isWithinCooldown (call, now)) {
    emit suppressed (call, matched, QStringLiteral ("cooldown 120s"));
    return;
  }
  if (isWithinRateLimit (now)) {
    emit suppressed (call, matched, QStringLiteral ("rate limit 3/60s"));
    return;
  }

  // Fire.
  lastCallPerCallsign_[call] = now;
  recentGlobalCalls_.push_back (now);
  emit callRequested (d, matched);
}

} // namespace wkjtx
