#include "ThemeManager.hpp"

#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTextStream>

namespace wkjtx {

ThemeManager::ThemeManager (QApplication * app, QObject * parent)
  : QObject {parent}
  , app_ {app}
{}

QString ThemeManager::displayName (ThemeId id)
{
  switch (id) {
  case ThemeId::AmberClassic:    return QObject::tr ("Amber Classic");
  case ThemeId::AmberNight:      return QObject::tr ("Amber Night");
  case ThemeId::AmberHiContrast: return QObject::tr ("Amber High Contrast");
  case ThemeId::Native:          return QObject::tr ("Native (OS default)");
  case ThemeId::DarkLegacy:      return QObject::tr ("Dark (legacy JTDX)");
  }
  return QObject::tr ("Unknown");
}

QString ThemeManager::idKey (ThemeId id)
{
  switch (id) {
  case ThemeId::AmberClassic:    return QStringLiteral ("amber-classic");
  case ThemeId::AmberNight:      return QStringLiteral ("amber-night");
  case ThemeId::AmberHiContrast: return QStringLiteral ("amber-high-contrast");
  case ThemeId::Native:          return QStringLiteral ("native");
  case ThemeId::DarkLegacy:      return QStringLiteral ("dark-legacy");
  }
  return QStringLiteral ("amber-classic");
}

ThemeId ThemeManager::idFromKey (QString const & key)
{
  if (key == QLatin1String ("amber-classic"))       return ThemeId::AmberClassic;
  if (key == QLatin1String ("amber-night"))         return ThemeId::AmberNight;
  if (key == QLatin1String ("amber-high-contrast")) return ThemeId::AmberHiContrast;
  if (key == QLatin1String ("native"))              return ThemeId::Native;
  if (key == QLatin1String ("dark-legacy"))         return ThemeId::DarkLegacy;
  return ThemeId::AmberClassic;
}

QString ThemeManager::readQssResource (QString const & resourcePath) const
{
  QFile f {resourcePath};
  if (!f.open (QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning ("ThemeManager: cannot open QSS resource %s",
              qPrintable (resourcePath));
    return QString ();
  }
  QTextStream ts {&f};
  return ts.readAll ();
}

void ThemeManager::applyTheme (ThemeId id)
{
  if (!app_) return;

  QString qss;
  switch (id) {
  case ThemeId::AmberClassic:
    qss = readQssResource (QStringLiteral (":/wkjtx/themes/amber-classic.qss"));
    break;
  case ThemeId::AmberNight:
    qss = readQssResource (QStringLiteral (":/wkjtx/themes/amber-night.qss"));
    break;
  case ThemeId::AmberHiContrast:
    qss = readQssResource (QStringLiteral (":/wkjtx/themes/amber-high-contrast.qss"));
    break;
  case ThemeId::Native:
    qss.clear ();  // empty stylesheet = OS default
    break;
  case ThemeId::DarkLegacy:
    // Re-apply the original JTDX QDarkStyleSheet. The resource path is
    // compiled in by the contrib/ submodule; qss lives at :/qdarkstyle/style.qss.
    qss = readQssResource (QStringLiteral (":/qdarkstyle/style.qss"));
    break;
  }

  app_->setStyleSheet (qss);
  current_ = id;

  QSettings settings;
  settings.setValue (QStringLiteral ("theme/current"), idKey (id));
  // Keep the legacy JTDX "UseDarkStyle" flag in sync so
  // Configuration::set_application_font's legacy branch stays consistent
  // with the ThemeManager selection. Only DarkLegacy means "apply the old
  // QDarkStyleSheet"; every other WKjTX theme must clear the flag.
  settings.setValue (QStringLiteral ("UseDarkStyle"), id == ThemeId::DarkLegacy);
  settings.sync ();

  emit themeChanged (id);
}

void ThemeManager::loadPersisted ()
{
  QSettings settings;
  QString const key = settings.value (
      QStringLiteral ("theme/current"),
      QStringLiteral ("amber-classic")).toString ();
  applyTheme (idFromKey (key));
}

} // namespace wkjtx
