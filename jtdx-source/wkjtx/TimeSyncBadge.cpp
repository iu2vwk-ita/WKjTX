#include "TimeSyncBadge.hpp"
#include "TimeSync.hpp"

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QtGlobal>

namespace wkjtx {

TimeSyncBadge::TimeSyncBadge (QWidget * parent)
  : QToolButton {parent}
{
  setObjectName (QStringLiteral ("TimeSyncBadge"));
  setText (QStringLiteral ("NTP: --"));
  setToolTip (tr ("NTP clock offset · click to resync system clock (UAC prompt)"));
  setAutoRaise (false);
  setCursor (Qt::PointingHandCursor);
  setFocusPolicy (Qt::NoFocus);

  // Compact size matching ProfileButton look; wide enough for the
  // worst-case formatted offset text "NTP: +9999 ms" plus padding.
  setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Fixed);
  setMinimumSize (130, 26);

  connect (this, &QToolButton::clicked,
           this, [this] { emit syncRequested (); });

  applyStyle (0, false);
}

void TimeSyncBadge::setOffsetMs (qint64 ms, bool ok, QString const& errorText)
{
  if (ok) {
    QChar sign = ms >= 0 ? QChar ('+') : QChar ('-');
    qint64 absMs = ms < 0 ? -ms : ms;
    if (absMs >= 10000) {
      setText (QStringLiteral ("NTP: %1%2 s").arg (sign).arg (absMs / 1000));
    } else {
      setText (QStringLiteral ("NTP: %1%2 ms").arg (sign).arg (absMs));
    }
    setToolTip (tr ("System clock offset: %1%2 ms vs NTP\nClick to resync (UAC prompt).")
                  .arg (sign).arg (absMs));
    applyStyle (absMs, true);
  } else {
    setText (QStringLiteral ("NTP: ?"));
    setToolTip (tr ("Could not reach NTP: %1\nClick to resync system clock anyway.")
                  .arg (errorText));
    applyStyle (0, false);
  }
}

void TimeSyncBadge::contextMenuEvent (QContextMenuEvent * ev)
{
  QMenu menu (this);
  bool const autoOn = TimeSyncManager::autoSyncEnabled ();

  auto * sync = menu.addAction (tr ("Resync now (UAC)"));
  connect (sync, &QAction::triggered, this, [this] { emit syncRequested (); });
  menu.addSeparator ();

  auto * enable = menu.addAction (
      tr ("Auto-sync system clock every 10 minutes (install, UAC)"));
  enable->setCheckable (true);
  enable->setChecked (autoOn);
  enable->setEnabled (!autoOn);
  connect (enable, &QAction::triggered, this,
           [this] { emit enableAutoSyncRequested (); });

  auto * disable = menu.addAction (
      tr ("Restore Windows default sync (1 week, time.windows.com)"));
  disable->setEnabled (autoOn);
  connect (disable, &QAction::triggered, this,
           [this] { emit disableAutoSyncRequested (); });

  menu.exec (ev->globalPos ());
}

void TimeSyncBadge::applyStyle (qint64 absMs, bool ok)
{
  QString base = QStringLiteral (
      "QToolButton#TimeSyncBadge {"
      " padding: 3px 10px;"
      " border: 1px solid %1;"
      " border-radius: 4px;"
      " background: %2;"
      " color: %3;"
      " font-weight: 600;"
      "}"
      "QToolButton#TimeSyncBadge:hover {"
      " background: %4;"
      "}");

  QString border, bg, fg, bgHover;
  if (!ok) {
    border = QStringLiteral ("#6a1818");
    bg     = QStringLiteral ("#2a0f0f");
    fg     = QStringLiteral ("#ff8080");
    bgHover = QStringLiteral ("#3a1414");
  } else if (absMs < 100) {
    border = QStringLiteral ("#2a8a2a");
    bg     = QStringLiteral ("#0f2010");
    fg     = QStringLiteral ("#80ff80");
    bgHover = QStringLiteral ("#153018");
  } else if (absMs < 500) {
    border = QStringLiteral ("#c46d00");
    bg     = QStringLiteral ("#1a1207");
    fg     = QStringLiteral ("#ffb347");
    bgHover = QStringLiteral ("#261a0a");
  } else {
    border = QStringLiteral ("#8a2a2a");
    bg     = QStringLiteral ("#2a0f0f");
    fg     = QStringLiteral ("#ff6060");
    bgHover = QStringLiteral ("#3a1818");
  }
  setStyleSheet (base.arg (border, bg, fg, bgHover));
}

} // namespace wkjtx
