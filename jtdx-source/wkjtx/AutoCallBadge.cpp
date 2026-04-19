#include "AutoCallBadge.hpp"
#include "AutoCall.hpp"

#include <QMouseEvent>
#include <QTimer>

namespace wkjtx {

AutoCallBadge::AutoCallBadge (AutoCall * ac, QWidget * parent)
  : QLabel {parent}
  , ac_ {ac}
  , timer_ {new QTimer {this}}
{
  setObjectName (QStringLiteral ("autoCallBadge"));
  setText (QStringLiteral ("AUTO-CALL"));
  setAlignment (Qt::AlignCenter);
  setCursor (Qt::PointingHandCursor);
  setToolTip (tr ("Auto-call attivo. Click per aprire le impostazioni."));
  // Amber-matched warning badge styling; global QSS can override.
  setStyleSheet (QStringLiteral (
    "QLabel#autoCallBadge {"
    "  background: #4a0a0a;"
    "  color: #ff5050;"
    "  border: 1px solid #ff5050;"
    "  border-radius: 4px;"
    "  padding: 2px 8px;"
    "  font-weight: 700;"
    "  letter-spacing: 2px;"
    "  font-size: 10px;"
    "}"
  ));

  timer_->setInterval (500);    // 1 Hz = toggle every 500 ms
  connect (timer_, &QTimer::timeout, this, &AutoCallBadge::onBlink);

  if (ac_) {
    connect (ac_, &AutoCall::configChanged, this, &AutoCallBadge::refresh);
  }
  refresh ();
}

void AutoCallBadge::refresh ()
{
  if (!ac_) {
    setVisible (false);
    timer_->stop ();
    return;
  }
  int const n = ac_->activeCategoryCount ();
  if (n == 0) {
    setVisible (false);
    timer_->stop ();
    return;
  }
  setText (QStringLiteral ("AUTO-CALL · %1").arg (n));
  setVisible (true);
  blinkOn_ = true;
  if (!timer_->isActive ()) timer_->start ();
}

void AutoCallBadge::onBlink ()
{
  blinkOn_ = !blinkOn_;
  // Toggle between the normal "on" state and a dimmed "off" state.
  if (blinkOn_) {
    setStyleSheet (QStringLiteral (
      "QLabel#autoCallBadge {"
      "  background: #4a0a0a;"
      "  color: #ff5050;"
      "  border: 1px solid #ff5050;"
      "  border-radius: 4px;"
      "  padding: 2px 8px;"
      "  font-weight: 700;"
      "  letter-spacing: 2px;"
      "  font-size: 10px;"
      "}"));
  } else {
    setStyleSheet (QStringLiteral (
      "QLabel#autoCallBadge {"
      "  background: #1a0000;"
      "  color: #802020;"
      "  border: 1px solid #802020;"
      "  border-radius: 4px;"
      "  padding: 2px 8px;"
      "  font-weight: 700;"
      "  letter-spacing: 2px;"
      "  font-size: 10px;"
      "}"));
  }
}

void AutoCallBadge::mousePressEvent (QMouseEvent * e)
{
  if (e->button () == Qt::LeftButton) {
    emit clicked ();
    e->accept ();
    return;
  }
  QLabel::mousePressEvent (e);
}

} // namespace wkjtx
