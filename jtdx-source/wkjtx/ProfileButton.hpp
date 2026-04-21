#ifndef WKJTX_PROFILE_BUTTON_HPP
#define WKJTX_PROFILE_BUTTON_HPP

#include <QPushButton>
#include <QString>

namespace wkjtx {

class ProfileButton : public QPushButton
{
  Q_OBJECT

public:
  explicit ProfileButton (int slot, QWidget * parent = nullptr);

  void setProfileName (QString const & name);
  void setActive      (bool active);
  void setSlotVisible (bool visible);

  int  slotIndex () const { return slot_; }
  bool isEmpty ()   const { return name_.isEmpty (); }

signals:
  void switchRequested    (int slot);
  void configureRequested (int slot);
  void renameRequested    (int slot);
  void hideRequested      (int slot);
  void clearRequested     (int slot);

protected:
  void mousePressEvent    (QMouseEvent       * e) override;
  void contextMenuEvent   (QContextMenuEvent * e) override;

private:
  void applyStyle ();

  int     slot_;
  QString name_;
  bool    active_ {false};
};

} // namespace wkjtx
#endif
