#ifndef WKJTX_TIME_SYNC_HPP
#define WKJTX_TIME_SYNC_HPP

#include <QObject>
#include <QString>
#include <QHostAddress>

class QUdpSocket;
class QTimer;

namespace wkjtx {

// SntpClient
// ----------
// Single-shot SNTP client (RFC 4330). Resolves a host, sends a 48-byte
// NTP packet on UDP/123, reads the response, computes the offset of
// the system clock against the server clock, and emits `finished()`.
//
// Offset is positive when the server is ahead of us.
//
// Non-blocking: the whole exchange runs on the Qt event loop. A
// hard timeout aborts after ~3 s so the UI never stalls on an
// unreachable server.
class SntpClient : public QObject
{
  Q_OBJECT

public:
  explicit SntpClient (QObject * parent = nullptr);

  // Kick off a query. Only one query may be in flight per instance.
  void query (QString const& host = QStringLiteral ("pool.ntp.org"),
              quint16 port = 123);

signals:
  // offsetMs: (server - client) in milliseconds when ok == true.
  // When ok == false the offset is 0 and errorText describes why.
  void finished (qint64 offsetMs, bool ok, QString const& errorText);

private slots:
  void onLookup (int id);
  void onReadyRead ();
  void onTimeout ();

private:
  void sendPacket ();
  void cleanup ();

  QUdpSocket * socket_ {nullptr};
  QTimer     * timeout_ {nullptr};
  QHostAddress server_;
  quint16      port_   {123};
  qint64       t1_unix_ms_ {0};
  int          lookup_id_  {-1};
  bool         in_flight_  {false};

public:
  static constexpr int TIMEOUT_MS  = 3000;
  static constexpr qint64 NTP_EPOCH_OFFSET_SEC = 2208988800LL;  // 1900-01-01 -> 1970-01-01
};


// TimeSyncManager
// ---------------
// Runs an SntpClient on a timer, exposes last offset for UI,
// and provides a helper to trigger the elevated Windows clock
// resync via `w32tm /resync`.
class TimeSyncManager : public QObject
{
  Q_OBJECT

public:
  explicit TimeSyncManager (QObject * parent = nullptr);

  // Fire an immediate query and arm the auto-refresh timer.
  void startAutoRefresh (int intervalMs = 5 * 60 * 1000);
  void queryNow ();

  qint64  lastOffsetMs () const { return last_offset_ms_; }
  bool    lastOk       () const { return last_ok_; }
  QString lastError    () const { return last_error_; }

  // Set the system clock by applying offsetMs via an elevated
  // PowerShell Set-Date call. This bypasses Windows Time service
  // configuration (which is often broken / unconfigured on user
  // machines) and steps the clock directly by the measured NTP
  // offset.
  //
  // Returns true on successful elevated launch; UAC refusal returns
  // false. The actual clock change happens synchronously inside the
  // elevated child process.
  static bool elevatedSyncSystemClock (qint64 offsetMs);

  // One-time elevated reconfiguration of Windows Time Service:
  // point it at pool.ntp.org, set SpecialPollInterval to 600 s
  // (10 minutes), restart the service. One UAC prompt; after that
  // Windows keeps the clock in sync silently as SYSTEM.
  // Persists the user preference under QSettings "timesync/auto_enabled".
  static bool enableAutoSync10Min ();

  // Revert Windows Time Service to a default time.windows.com peer
  // with a 1-week poll interval (approximate Windows default).
  // Also clears QSettings "timesync/auto_enabled".
  static bool restoreDefaultSync ();

  // Read the persisted preference flag (does NOT probe the registry —
  // if the user edited the time service outside of WKjTX the flag may
  // be stale; we trust the user in that case).
  static bool autoSyncEnabled ();

signals:
  void offsetUpdated (qint64 offsetMs, bool ok, QString const& errorText);

private slots:
  void onFinished (qint64 offsetMs, bool ok, QString const& errorText);

private:
  SntpClient * client_  {nullptr};
  QTimer     * refresh_ {nullptr};
  qint64 last_offset_ms_ {0};
  bool   last_ok_        {false};
  QString last_error_;
};

} // namespace wkjtx

#endif // WKJTX_TIME_SYNC_HPP
