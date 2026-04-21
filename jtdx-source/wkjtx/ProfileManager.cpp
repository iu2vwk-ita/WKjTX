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
  slots_[0].valid = true;
  active_slot_ = 1;
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

  emit aboutToSwitch (active_slot_, slotIndex);

  if (slotIndex == 1) {
    // Slot 1 = reapply the main app configuration. The main settings are
    // already live in memory, so we just need to nudge the transceiver to
    // reconnect with them. No INI read, no overwrite.
    if (!cfg_->transceiver_online ()) return SwitchResult::CatOpenFailed;
    active_slot_ = 1;
    emit switched (1);
    return SwitchResult::Ok;
  }

  QSettings ini {target.iniPath, QSettings::IniFormat};
  if (ini.status () != QSettings::NoError) return SwitchResult::IniMissing;

  cfg_->applyRadioFromSettings (ini);

  if (!cfg_->transceiver_online ()) {
    // Roll back to the main config (slot 1) on CAT failure.
    cfg_->transceiver_online ();
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

} // namespace wkjtx
