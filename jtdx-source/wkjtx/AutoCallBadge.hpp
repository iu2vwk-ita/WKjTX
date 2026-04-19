#ifndef WKJTX_AUTOCALL_BADGE_HPP
#define WKJTX_AUTOCALL_BADGE_HPP

// AutoCallBadge — flashing red "AUTO-CALL · N" indicator shown in the
// status bar when at least one auto-call category is active. Blinks at
// ~1 Hz via QTimer to attract operator attention.
//
// N = number of currently-active categories (0 = hidden, N>0 = visible).
//
// Clicking the badge opens AutoCallSettingsDialog so the operator can
// review or disable immediately. Not the primary kill-switch (that's
// F12 / Halt TX) but a convenient escalation.

#include <QLabel>

class QTimer;

namespace wkjtx {

class AutoCall;

class AutoCallBadge : public QLabel
{
  Q_OBJECT

public:
  explicit AutoCallBadge (AutoCall * ac, QWidget * parent = nullptr);

  // Refreshes visibility and text based on AutoCall state. Called
  // automatically via AutoCall::configChanged signal.
  void refresh ();

signals:
  // Emitted when the user clicks the badge (for opening the settings
  // dialog from MainWindow).
  void clicked ();

protected:
  void mousePressEvent (QMouseEvent * e) override;

private slots:
  void onBlink ();

private:
  AutoCall * ac_  {nullptr};
  QTimer *   timer_ {nullptr};
  bool       blinkOn_ {true};
};

} // namespace wkjtx

#endif // WKJTX_AUTOCALL_BADGE_HPP
