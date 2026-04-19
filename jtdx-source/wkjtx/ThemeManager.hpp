#ifndef WKJTX_THEME_MANAGER_HPP
#define WKJTX_THEME_MANAGER_HPP

// ThemeManager — loads QSS stylesheets, applies them globally,
// persists the user's choice across sessions.
//
// Built-in themes:
//   AmberClassic   (default)   — FT8 Card Pro-matched amber on dark
//   AmberNight                 — dimmer amber, night-adapted
//   AmberHiContrast            — max contrast amber+black+white
//   Native                     — no stylesheet, use OS default
//   DarkLegacy                 — JTDX's original QDarkStyleSheet
//
// QSS files live under :/wkjtx/themes/*.qss (Qt resources).
// User choice persisted to QSettings key "theme/current".

#include <QObject>
#include <QString>

class QApplication;

namespace wkjtx {

enum class ThemeId
{
  AmberClassic,
  AmberNight,
  AmberHiContrast,
  Native,
  DarkLegacy
};

class ThemeManager : public QObject
{
  Q_OBJECT

public:
  explicit ThemeManager (QApplication * app, QObject * parent = nullptr);

  // Apply a theme immediately and persist the choice.
  void applyTheme (ThemeId id);

  // The currently-applied theme.
  ThemeId currentTheme () const { return current_; }

  // Human-readable display name (translated where appropriate).
  static QString displayName (ThemeId id);

  // Internal stable string used in QSettings and menu action data.
  static QString idKey (ThemeId id);
  static ThemeId idFromKey (QString const & key);

  // Load the user's previously-chosen theme on startup.
  void loadPersisted ();

signals:
  void themeChanged (ThemeId id);

private:
  QApplication * app_ {nullptr};
  ThemeId current_ {ThemeId::AmberClassic};

  QString readQssResource (QString const & resourcePath) const;
};

} // namespace wkjtx

#endif // WKJTX_THEME_MANAGER_HPP
