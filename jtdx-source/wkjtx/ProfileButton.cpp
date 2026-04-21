#include "ProfileButton.hpp"

#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>

namespace wkjtx {

ProfileButton::ProfileButton (int slot, QWidget * parent)
  : QPushButton {parent}
  , slot_ {slot}
{
  setFixedWidth (90);
  setFixedHeight (22);
  setFlat (true);
  setFocusPolicy (Qt::NoFocus);
  applyStyle ();
}

void ProfileButton::setProfileName (QString const & name)
{
  name_ = name;
  setText (name_.isEmpty () ? QStringLiteral ("+") : name_);
  applyStyle ();
}

void ProfileButton::setActive (bool active)
{
  active_ = active;
  applyStyle ();
}

void ProfileButton::setSlotVisible (bool visible)
{
  setVisible (visible);
}

void ProfileButton::applyStyle ()
{
  QString const border = active_
    ? QStringLiteral ("1px solid #ff9500")
    : QStringLiteral ("1px solid #444");
  QString const color = isEmpty ()
    ? QStringLiteral ("#555")
    : (active_ ? QStringLiteral ("#ff9500") : QStringLiteral ("#ccc"));
  QString const bg = active_
    ? QStringLiteral ("#2a1a05")
    : QStringLiteral ("#1a1a1a");

  setStyleSheet (QStringLiteral (
    "QPushButton {"
    "  background: %1;"
    "  border: %2;"
    "  border-radius: 3px;"
    "  color: %3;"
    "  font-size: 11px;"
    "  padding: 0 4px;"
    "}"
    "QPushButton:hover { background: #252525; border-color: #666; }"
  ).arg (bg, border, color));
}

void ProfileButton::mousePressEvent (QMouseEvent * e)
{
  if (e->button () == Qt::LeftButton) {
    if (!isEmpty ())
      emit switchRequested (slot_);
    else
      emit configureRequested (slot_);
  } else {
    QPushButton::mousePressEvent (e);
  }
}

void ProfileButton::contextMenuEvent (QContextMenuEvent * e)
{
  QMenu menu (this);
  QAction * configure = menu.addAction (isEmpty ()
    ? QStringLiteral ("Set up profile %1...").arg (slot_)
    : QStringLiteral ("Configure..."));
  QAction * rename = menu.addAction (QStringLiteral ("Rename"));
  QAction * hide   = menu.addAction (QStringLiteral ("Hide"));
  menu.addSeparator ();
  QAction * clear  = menu.addAction (QStringLiteral ("Clear"));

  rename->setEnabled (!isEmpty ());
  clear->setEnabled  (!isEmpty ());

  QAction * chosen = menu.exec (e->globalPos ());
  if      (chosen == configure) emit configureRequested (slot_);
  else if (chosen == rename)    emit renameRequested    (slot_);
  else if (chosen == hide)      emit hideRequested      (slot_);
  else if (chosen == clear)     emit clearRequested     (slot_);
}

} // namespace wkjtx
