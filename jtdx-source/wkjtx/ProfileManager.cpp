#include "ProfileManager.hpp"
#include "../Configuration.hpp"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

// Slot 1 is special: it represents the base app configuration stored in
// the main JTDX.ini. Profile-button actions never overwrite it.
// Slots 2 and 3 are overlays stored in %LOCALAPPDATA%/WKjTX/profiles/slotN.ini
// and carry only the radio+audio fields produced by
// Configuration::snapshotRadioToSettings().

namespace wkjtx {

ProfileManager::ProfileManager (Configuration * cfg, QObject * parent)
  : QObject {parent}
  , cfg_ {cfg}
{
  profilesDir_ = QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation)
               + QStringLiteral ("/profiles");
  slots_.resize (kMaxSlots);
  for (int i = 0; i < kMaxSlots; ++i)
    slots_[i].slotIndex = i + 1;

  // Refresh the slot 1 baseline whenever the user edits + saves the
  // main JTDX.ini config (Settings dialog → OK while slot 1 is active).
  // Without this, a later switch slot 2 → slot 1 would re-apply a stale
  // baseline that predates the user's edits.
  connect (cfg_, &Configuration::base_rig_settings_persisted,
           this, &ProfileManager::refreshSlot1Baseline);
}

ProfileManager::~ProfileManager () = default;

void ProfileManager::ensureProfilesDirExists ()
{
  QDir {}.mkpath (profilesDir_);
}

void ProfileManager::loadAll ()
{
  ensureProfilesDirExists ();
  for (int i = 0; i < kMaxSlots; ++i) {
    QString const path = QStringLiteral ("%1/slot%2.ini").arg (profilesDir_).arg (i + 1);
    slots_[i] = readProfileIni (path, i + 1);
  }
  // Slot 1 is always valid — it maps to the live main config. If no name
  // saved yet, default to "Radio 1".
  if (slots_[0].name.isEmpty ())
    slots_[0].name = QStringLiteral ("Radio 1");
  slots_[0].valid   = true;
  if (slots_[0].iniPath.isEmpty ())
    slots_[0].iniPath = QStringLiteral ("%1/slot1.ini").arg (profilesDir_);

  // Capture the live (just-loaded-from-JTDX.ini) radio settings as the
  // slot 1 baseline. Held in memory rather than serialised to INI so
  // enum fields cannot be lost in a QVariant round-trip. We still
  // mirror the snapshot into slot1.ini for backwards compatibility
  // and so something on disk reflects "what slot 1 looks like".
  baseline_rig_params_ = cfg_->currentRigParams ();
  baseline_captured_   = true;
  {
    QSettings ini {slots_[0].iniPath, QSettings::IniFormat};
    ini.setValue (QStringLiteral ("Profile/Name"),    slots_[0].name);
    ini.setValue (QStringLiteral ("Profile/Visible"), slots_[0].visible);
    cfg_->snapshotRadioToSettings (ini);
  }

  active_slot_ = 1;
  cfg_->setActiveProfileSlot (1);
  emit slotsChanged ();
}

Profile ProfileManager::readProfileIni (QString const & path, int slotIndex) const
{
  Profile p;
  p.slotIndex = slotIndex;
  p.iniPath   = path;
  QSettings ini {path, QSettings::IniFormat};
  p.name    = ini.value (QStringLiteral ("Profile/Name")).toString ();
  p.visible = ini.value (QStringLiteral ("Profile/Visible"), true).toBool ();
  p.valid   = !p.name.isEmpty ();
  return p;
}

bool ProfileManager::writeProfileIni (Profile const & p, QSettings & settings) const
{
  settings.setValue (QStringLiteral ("Profile/Name"),    p.name);
  settings.setValue (QStringLiteral ("Profile/Visible"), p.visible);
  settings.sync ();
  return settings.status () == QSettings::NoError;
}

bool ProfileManager::saveSlot (int slotIndex)
{
  if (slotIndex < 1 || slotIndex > kMaxSlots) return false;
  ensureProfilesDirExists ();
  Profile & p = slots_[slotIndex - 1];
  if (p.iniPath.isEmpty ())
    p.iniPath = QStringLiteral ("%1/slot%2.ini").arg (profilesDir_).arg (slotIndex);

  // Slot 1 lives in the main JTDX.ini. We only keep its *name* in a tiny
  // side-file so the button label persists. No radio settings go to disk
  // here — those stay in the main app config where they belong.
  QSettings ini {p.iniPath, QSettings::IniFormat};
  writeProfileIni (p, ini);
  if (slotIndex != 1) {
    cfg_->snapshotRadioToSettings (ini);
  }
  return ini.status () == QSettings::NoError;
}

SwitchResult ProfileManager::switchToSlot (int slotIndex)
{
  if (slotIndex < 1 || slotIndex > kMaxSlots) return SwitchResult::UnknownError;
  Profile const & target = slots_[slotIndex - 1];
  if (!target.valid) return SwitchResult::IniMissing;
  if (target.iniPath.isEmpty ()) return SwitchResult::IniMissing;

  emit aboutToSwitch (active_slot_, slotIndex);

  // The persistence-gate must be flipped BEFORE applyRadioFromSettings,
  // because that call already mutates rig_params_ and any side-effect
  // write (e.g. a Settings dialog accept fired by Qt event reentrancy)
  // must see the correct active slot.
  cfg_->setActiveProfileSlot (slotIndex);

  // If we're about to LEAVE slot 1, refresh slot1.ini with the
  // currently-live config first. This guarantees that the baseline on
  // disk reflects what was active just before the user pressed an
  // overlay button, regardless of whether write_settings() has been
  // called since the last edit. Without this, an INI snapshot taken
  // way back at app start can drift from what the user currently
  // expects "Radio 1" to mean.
  if (active_slot_ == 1 && slotIndex != 1) {
    QSettings baseIni {slots_[0].iniPath, QSettings::IniFormat};
    baseIni.setValue (QStringLiteral ("Profile/Name"),    slots_[0].name);
    baseIni.setValue (QStringLiteral ("Profile/Visible"), slots_[0].visible);
    cfg_->snapshotRadioToSettings (baseIni);
    baseline_rig_params_ = cfg_->currentRigParams ();
    baseline_captured_   = true;
  }

  // Apply the target slot's INI to the live Configuration. Slot 1's
  // INI is the baseline that was just refreshed above (or captured at
  // loadAll); slot 2/3's INI is the user-edited overlay from
  // RadioProfileDialog. Either way the apply path is identical and
  // covers both rig + audio fields.
  QSettings ini {target.iniPath, QSettings::IniFormat};
  if (ini.status () != QSettings::NoError) {
    cfg_->setActiveProfileSlot (active_slot_);  // unwind flag
    return SwitchResult::IniMissing;
  }
  cfg_->applyRadioFromSettings (ini);

  // Use the rig_params_-driven reopen path rather than the generic
  // transceiver_online() — the latter routes through gather_rig_data()
  // which scrapes the Settings dialog widgets, and any field that fails
  // to round-trip through that scrape (e.g. a baud combo that doesn't
  // contain the slot's exact value, or a serial port not present in the
  // freshly re-enumerated list) silently turns into the wrong rig.
  if (!cfg_->reopenRigWithCurrentParams ()) {
    int const previous = active_slot_;
    QString const prevPath = slots_[previous - 1].iniPath;
    if (!prevPath.isEmpty ()) {
      QSettings prevIni {prevPath, QSettings::IniFormat};
      if (prevIni.status () == QSettings::NoError)
        cfg_->applyRadioFromSettings (prevIni);
    }
    cfg_->setActiveProfileSlot (previous);
    cfg_->reopenRigWithCurrentParams ();
    return SwitchResult::CatOpenFailed;
  }

  active_slot_ = slotIndex;
  emit switched (slotIndex);
  return SwitchResult::Ok;
}

void ProfileManager::setSlotVisible (int slotIndex, bool visible)
{
  if (slotIndex < 1 || slotIndex > kMaxSlots) return;
  slots_[slotIndex - 1].visible = visible;
  if (!slots_[slotIndex - 1].iniPath.isEmpty ()) {
    QSettings ini {slots_[slotIndex - 1].iniPath, QSettings::IniFormat};
    ini.setValue (QStringLiteral ("Profile/Visible"), visible);
  }
  emit slotsChanged ();
}

bool ProfileManager::isSlotVisible (int slotIndex) const
{
  if (slotIndex < 1 || slotIndex > kMaxSlots) return false;
  return slots_[slotIndex - 1].visible;
}

void ProfileManager::showAllSlots ()
{
  for (int i = 1; i <= kMaxSlots; ++i)
    setSlotVisible (i, true);
}

bool ProfileManager::closeCurrentResources ()  { return true; }
bool ProfileManager::openResourcesForSlot (int) { return true; }
void ProfileManager::rollback (int)             {}

void ProfileManager::refreshSlot1Baseline ()
{
  if (slots_.isEmpty ()) return;
  if (slots_[0].iniPath.isEmpty ())
    slots_[0].iniPath = QStringLiteral ("%1/slot1.ini").arg (profilesDir_);
  ensureProfilesDirExists ();
  QSettings ini {slots_[0].iniPath, QSettings::IniFormat};
  ini.setValue (QStringLiteral ("Profile/Name"),    slots_[0].name);
  ini.setValue (QStringLiteral ("Profile/Visible"), slots_[0].visible);
  cfg_->snapshotRadioToSettings (ini);
}

} // namespace wkjtx
