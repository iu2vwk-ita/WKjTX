#ifndef WKJTX_PROFILE_MANAGER_HPP
#define WKJTX_PROFILE_MANAGER_HPP

// ProfileManager — 3-slot radio profile quick-switch system.
//
// Each slot stores a complete CAT+audio configuration in a separate INI
// file under %LOCALAPPDATA%/WKjTX/profiles/slot<N>.ini. Switching is
// immediate: save current slot, load new slot into Configuration,
// reconnect transceiver. Rolls back on CAT open failure.

#include <QObject>
#include <QString>
#include <QVector>

class QSettings;
class Configuration;

namespace wkjtx {

struct Profile
{
  int     slotIndex {0};
  QString name;
  QString iniPath;
  bool    visible {true};
  bool    valid   {false};

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
  static constexpr int kMaxSlots = 3;

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

  void setSlotVisible (int slotIndex, bool visible);
  bool isSlotVisible  (int slotIndex) const;
  void showAllSlots   ();

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
