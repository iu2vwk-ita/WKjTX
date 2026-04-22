#ifndef WKJTX_TIME_SYNC_BADGE_HPP
#define WKJTX_TIME_SYNC_BADGE_HPP

#include <QToolButton>

namespace wkjtx {

// TimeSyncBadge
// -------------
// Small menubar-corner button that shows the current NTP offset of
// the system clock and triggers a one-shot elevated resync on click.
//
// Colour logic:
//   green   < 100 ms
//   amber   < 500 ms
//   red     >= 500 ms  (or unreachable NTP)
class TimeSyncBadge : public QToolButton
{
  Q_OBJECT

public:
  explicit TimeSyncBadge (QWidget * parent = nullptr);

public slots:
  void setOffsetMs (qint64 ms, bool ok, QString const& errorText);

signals:
  void syncRequested ();
  void enableAutoSyncRequested ();
  void disableAutoSyncRequested ();

protected:
  void contextMenuEvent (QContextMenuEvent * ev) override;

private:
  void applyStyle (qint64 absMs, bool ok);
};

} // namespace wkjtx

#endif // WKJTX_TIME_SYNC_BADGE_HPP
