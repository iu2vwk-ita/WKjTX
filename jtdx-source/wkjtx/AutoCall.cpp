#include "AutoCall.hpp"

// Skeleton implementation — v0.3 (Plan 3) will fill the detection
// logic. The safeguard helpers are already correct so cooldown and
// rate-limit unit tests can be written now against this skeleton.

namespace wkjtx {

AutoCall::AutoCall (QObject * parent)
  : QObject {parent}
{}

AutoCall::~AutoCall () = default;

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

void AutoCall::onDecode (Decode const & /*d*/)
{
  // TODO v0.3: run detector pipeline in priority order; if a category
  // matches AND !cooldown AND !rateLimit → emit callRequested().
  // Update lastCallPerCallsign_ and recentGlobalCalls_ on fire.
  // Emit suppressed() when a match is blocked by safeguards.
}

} // namespace wkjtx
