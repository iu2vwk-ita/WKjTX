#ifndef WKJTX_AUTOCALL_HPP
#define WKJTX_AUTOCALL_HPP

// AutoCall — hands-free Reply for specific decode triggers.
//
// Seven trigger categories, each independently toggleable and OFF by
// default:
//   Alert (user's 5-slot watchlist)
//   NewDxcc
//   NewCqZone
//   NewItuZone
//   NewGrid (4-char)
//   NewPrefix
//   NewCallsign
//
// Hardcoded safeguards (non-negotiable for safety):
//   - Per-callsign cooldown: 120 s
//   - Global rate limit: 3 auto-calls per 60 s rolling window
//
// AutoCall is per-profile: each of the 5 profile slots owns its own
// configuration. When the active profile changes, ProfileManager swaps
// the AutoCall state atomically.
//
// This is a SKELETON header. Implementation is delivered in Plan 3
// (WKjTX v0.3). The design rationale is in the spec doc, section 5,
// and the reference Python implementation lives at
// G:\Claude Local\BACKUP APP\ft8-companion\ui\main_window.py
// (functions _maybe_autocall, _refresh_autocall_badge).

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QDateTime>
#include <deque>

namespace wkjtx {

enum class AutoCallCategory
{
  Alert,
  NewDxcc,
  NewCqZone,
  NewItuZone,
  NewGrid,
  NewPrefix,
  NewCallsign
};

// The minimum info a detector needs from the decoder.
struct Decode
{
  QString callsign;                // dx call
  QString grid4;                   // 4-char Maidenhead (may be empty)
  QString prefix;                  // DXCC/operator prefix, e.g. "IU2"
  QString message;                 // raw decoded text
  double snr {0.0};
  double freqHz {0.0};
  QDateTime utc;
};

// Per-profile configuration snapshot. ProfileManager writes this to
// the slot INI under [AutoCall/*] keys.
struct AutoCallConfig
{
  bool masterEnable {false};
  QHash<AutoCallCategory, bool> categoryEnabled;  // all false by default
  QStringList alertCallsigns;      // up to 5 entries (Alert category)

  AutoCallConfig ()
  {
    categoryEnabled[AutoCallCategory::Alert]       = false;
    categoryEnabled[AutoCallCategory::NewDxcc]     = false;
    categoryEnabled[AutoCallCategory::NewCqZone]   = false;
    categoryEnabled[AutoCallCategory::NewItuZone]  = false;
    categoryEnabled[AutoCallCategory::NewGrid]     = false;
    categoryEnabled[AutoCallCategory::NewPrefix]   = false;
    categoryEnabled[AutoCallCategory::NewCallsign] = false;
  }
};

class AutoCall : public QObject
{
  Q_OBJECT

public:
  // Hardcoded safeguards — never user-configurable.
  static constexpr int kPerCallCooldownSec = 120;
  static constexpr int kGlobalLimitPerWindow = 3;
  static constexpr int kGlobalWindowSec = 60;

  explicit AutoCall (QObject * parent = nullptr);
  ~AutoCall () override;

  // Swap the current config in one atomic step. Called by
  // ProfileManager on profile switch.
  void setConfig (AutoCallConfig const & cfg);

  AutoCallConfig const & config () const { return config_; }

  // Main entry point: the decoder pipeline calls this for every new
  // decode. AutoCall decides whether to trigger and if so, fires
  // callRequested(). MainWindow connects that signal to the "call
  // this decode" action (the same action that double-clicking a
  // decode line performs in JTDX upstream).
  void onDecode (Decode const & d);

  // Number of currently-enabled categories (for badge "AUTO-CALL · N").
  int activeCategoryCount () const;

  // Returns true if any category is on AND masterEnable is on.
  bool isArmed () const;

signals:
  // AutoCall decided to trigger. MainWindow handles the actual Reply.
  void callRequested (Decode decode, AutoCallCategory category);

  // A decode matched a category but was suppressed by cooldown/rate
  // limit. UI can surface this in status bar for user feedback.
  void suppressed (QString callsign, AutoCallCategory category, QString reason);

  // Config mutated (enable flag flipped, alert list edited). UI
  // badge refreshes on this.
  void configChanged ();

private:
  AutoCallConfig config_;
  QHash<QString, QDateTime> lastCallPerCallsign_;
  std::deque<QDateTime> recentGlobalCalls_;  // for rate-limit window

  bool isWithinCooldown (QString const & callsign, QDateTime const & now) const;
  bool isWithinRateLimit (QDateTime const & now);

  Q_DISABLE_COPY (AutoCall)
};

} // namespace wkjtx

#endif // WKJTX_AUTOCALL_HPP
