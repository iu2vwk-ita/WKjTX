#include "ProfileManager.hpp"

// Skeleton implementation — v0.2 (Plan 2) will fill this in.
// The header documents the intended behavior; this TU exists so the
// code compiles and linking succeeds once the module is added to
// CMakeLists.txt.

#include <QDir>
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
  for (int i = 0; i < kMaxSlots; ++i) {
    slots_[i].slotIndex = i + 1;
  }
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
  p.iniPath = path;

  QSettings ini {path, QSettings::IniFormat};
  if (ini.status () != QSettings::NoError) {
    p.valid = false;
    return p;
  }
  p.name = ini.value (QStringLiteral ("Profile/Name")).toString ();
  p.colorTag = ini.value (QStringLiteral ("Profile/ColorTag")).toString ();
  p.hotkey = QKeySequence::fromString (ini.value (QStringLiteral ("Profile/Hotkey")).toString ());
  p.logPath = ini.value (QStringLiteral ("Profile/LogPath")).toString ();
  p.valid = !p.name.isEmpty ();
  return p;
}

bool ProfileManager::writeProfileIni (Profile const & p, QSettings & settings) const
{
  settings.setValue (QStringLiteral ("Profile/Name"), p.name);
  settings.setValue (QStringLiteral ("Profile/ColorTag"), p.colorTag);
  settings.setValue (QStringLiteral ("Profile/Hotkey"), p.hotkey.toString ());
  settings.setValue (QStringLiteral ("Profile/LogPath"), p.logPath);
  settings.sync ();
  return settings.status () == QSettings::NoError;
}

bool ProfileManager::saveSlot (int /*slotIndex*/)
{
  // TODO v0.2: snapshot cfg_ into the slot INI.
  return false;
}

SwitchResult ProfileManager::switchToSlot (int /*slotIndex*/)
{
  // TODO v0.2: implement safe-switch protocol (design section 4.3).
  return SwitchResult::UnknownError;
}

bool ProfileManager::importFromJtdx (QString const & /*jtdxIniPath*/, int /*targetSlot*/)
{
  // TODO v0.2: copy JTDX config INI into a WKjTX slot INI.
  return false;
}

bool ProfileManager::closeCurrentResources ()
{
  // TODO v0.2: close CAT, audio, UDP; wait for graceful shutdown.
  return true;
}

bool ProfileManager::openResourcesForSlot (int /*slotIndex*/)
{
  // TODO v0.2: open hardware with the new profile's settings.
  return true;
}

void ProfileManager::rollback (int /*previousSlot*/)
{
  // TODO v0.2: roll back to previousSlot after a failed switch.
}

} // namespace wkjtx
