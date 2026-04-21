#include "ProfileManager.hpp"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

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
  QSettings ini {p.iniPath, QSettings::IniFormat};
  writeProfileIni (p, ini);
  cfg_->snapshotRadioToSettings (ini);
  return ini.status () == QSettings::NoError;
}

SwitchResult ProfileManager::switchToSlot (int slotIndex)
{
  if (slotIndex < 1 || slotIndex > kMaxSlots) return SwitchResult::UnknownError;
  Profile const & target = slots_[slotIndex - 1];
  if (!target.valid) return SwitchResult::IniMissing;

  if (active_slot_ >= 1 && active_slot_ <= kMaxSlots)
    saveSlot (active_slot_);

  emit aboutToSwitch (active_slot_, slotIndex);

  QSettings ini {target.iniPath, QSettings::IniFormat};
  if (ini.status () != QSettings::NoError) return SwitchResult::IniMissing;

  cfg_->applyRadioFromSettings (ini);

  if (!cfg_->transceiver_online ()) {
    if (active_slot_ >= 1 && active_slot_ <= kMaxSlots) {
      QSettings prev {slots_[active_slot_ - 1].iniPath, QSettings::IniFormat};
      cfg_->applyRadioFromSettings (prev);
      cfg_->transceiver_online ();
    }
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
