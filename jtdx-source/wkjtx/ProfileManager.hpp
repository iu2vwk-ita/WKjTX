#ifndef WKJTX_PROFILE_MANAGER_HPP
#define WKJTX_PROFILE_MANAGER_HPP

// ProfileManager — 5-slot radio profile quick-switch system.
//
// A "profile" is a complete named configuration (CAT, audio, UDP ports,
// log path, macros, auto-call config, etc.) that the user can recall
// with one click or an F-key (F1..F5). Only one profile is active at a
// time (v0.2 scope: single-active). Switching a profile closes hardware
// resources (CAT, audio, UDP), loads the new INI, and reopens with the
// new values. If validation fails mid-switch, roll back to the prior
// profile.
//
// This is a SKELETON header. Implementation is delivered in Plan 2
// (WKjTX v0.2).

#include <QObject>
#include <QString>
#include <QVector>
#include <QKeySequence>

class QSettings;
class Configuration;  // JTDX upstream class

namespace wkjtx {

// A single profile slot. Data owned by the INI file on disk; this is
// just the in-memory view used by ProfileManager.
struct Profile
{
  int slotIndex {0};               // 1..5 (0 = unassigned)
  QString name;                    // user-assigned, e.g. "IC-7300"
  QString colorTag;                // optional hex color #RRGGBB
  QKeySequence hotkey;             // usually F1..F5
  QString iniPath;                 // absolute path to the profile's INI file
  QString logPath;                 // empty = use shared global log;
                                   // non-empty = per-profile ADIF file
  bool valid {false};              // false if INI is missing or malformed

  bool isEmpty () const { return name.isEmpty (); }
};

// The result of a switch attempt. Allows the UI to show a clear toast
// and ProfileManager to roll back if anything mid-switch fails.
enum class SwitchResult
{
  Ok,
  AbortedByUser,           // user clicked "No" on TX-active dialog
  CatOpenFailed,
  AudioOpenFailed,
  UdpPortInUse,
  IniMissing,
  IniMalformed,
  UnknownError
};

class ProfileManager : public QObject
{
  Q_OBJECT

public:
  static constexpr int kMaxSlots = 5;

  explicit ProfileManager (Configuration * cfg, QObject * parent = nullptr);
  ~ProfileManager () override;

  // Load all slot<N>.ini files from %LOCALAPPDATA%/WKjTX/profiles/
  // into in-memory Profile vector. Missing slots are marked isEmpty().
  void loadAll ();

  // Persist a single profile's INI from the live Configuration.
  bool saveSlot (int slotIndex);

  // Switch the live app to profile <slotIndex>. Executes the
  // safe-switch protocol described in the design doc, section 4.3.
  SwitchResult switchToSlot (int slotIndex);

  // The currently-active slot (1..5), or 0 if no profile is active.
  int activeSlot () const { return active_slot_; }

  // Ordered view of all 5 slots (empty or not). UI renders this.
  // Named allSlots() not slots() because "slots" is a Qt macro.
  QVector<Profile> const & allSlots () const { return slots_; }

  // Convenience: import an existing JTDX config INI into a slot.
  // Reads %LOCALAPPDATA%/JTDX/<name>.ini and writes it as
  // %LOCALAPPDATA%/WKjTX/profiles/slot<N>.ini with a WKjTX header.
  bool importFromJtdx (QString const & jtdxIniPath, int targetSlot);

signals:
  // Fired immediately before hardware resources close.
  void aboutToSwitch (int fromSlot, int toSlot);

  // Fired after a successful switch, before MainWindow resumes.
  void switched (int newSlot);

  // Fired when a switch rolls back.
  void switchFailed (int attemptedSlot, SwitchResult reason);

  // Fired when the slots vector changes (slot created, renamed, deleted).
  void slotsChanged ();

private:
  Configuration * cfg_ {nullptr};
  QVector<Profile> slots_;
  int active_slot_ {0};
  QString profilesDir_;            // %LOCALAPPDATA%/WKjTX/profiles/

  void ensureProfilesDirExists ();
  Profile readProfileIni (QString const & path, int slotIndex) const;
  bool writeProfileIni (Profile const & p, QSettings & settings) const;

  // Hardware lifecycle helpers used by switchToSlot().
  bool closeCurrentResources ();
  bool openResourcesForSlot (int slotIndex);
  void rollback (int previousSlot);

  Q_DISABLE_COPY (ProfileManager)
};

} // namespace wkjtx

#endif // WKJTX_PROFILE_MANAGER_HPP
