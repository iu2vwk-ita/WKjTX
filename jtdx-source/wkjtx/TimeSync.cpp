#include "TimeSync.hpp"

#include <QByteArray>
#include <QDateTime>
#include <QHostInfo>
#include <QSettings>
#include <QTimer>
#include <QUdpSocket>

#ifdef Q_OS_WIN
# include <windows.h>
# include <shellapi.h>
#endif

namespace wkjtx {

namespace {

quint64 unixMsToNtp64 (qint64 unixMs)
{
  qint64 secs = unixMs / 1000;
  qint64 ms   = unixMs % 1000;
  if (ms < 0) { ms += 1000; --secs; }
  quint32 ntpSecs = static_cast<quint32> (secs + SntpClient::NTP_EPOCH_OFFSET_SEC);
  quint32 ntpFrac = static_cast<quint32> ((static_cast<quint64> (ms) << 32) / 1000ULL);
  return (static_cast<quint64> (ntpSecs) << 32) | ntpFrac;
}

qint64 ntp64ToUnixMs (quint64 ntp)
{
  quint32 ntpSecs = static_cast<quint32> (ntp >> 32);
  quint32 ntpFrac = static_cast<quint32> (ntp & 0xFFFFFFFFu);
  qint64 unixSec = static_cast<qint64> (ntpSecs) - SntpClient::NTP_EPOCH_OFFSET_SEC;
  qint64 ms      = (static_cast<qint64> (ntpFrac) * 1000LL) >> 32;
  return unixSec * 1000LL + ms;
}

void writeBigEndian64 (QByteArray & pkt, int offset, quint64 v)
{
  for (int i = 0; i < 8; ++i) {
    pkt[offset + i] = static_cast<char> ((v >> (56 - 8 * i)) & 0xFFu);
  }
}

quint64 readBigEndian64 (QByteArray const& pkt, int offset)
{
  quint64 v = 0;
  for (int i = 0; i < 8; ++i) {
    v = (v << 8) | static_cast<quint8> (pkt.at (offset + i));
  }
  return v;
}

} // anon

// ── SntpClient ─────────────────────────────────────────────────────────────

constexpr int SntpClient::TIMEOUT_MS;
constexpr qint64 SntpClient::NTP_EPOCH_OFFSET_SEC;

SntpClient::SntpClient (QObject * parent)
  : QObject {parent}
{}

void SntpClient::query (QString const& host, quint16 port)
{
  if (in_flight_) return;
  in_flight_ = true;
  port_      = port;

  // Async DNS lookup; Qt invokes our lambda on resolution.
  QHostInfo::lookupHost (host, this, [this] (QHostInfo const& info) {
    if (info.error () != QHostInfo::NoError || info.addresses ().isEmpty ()) {
      emit finished (0, false, tr ("DNS lookup failed: %1").arg (info.errorString ()));
      in_flight_ = false;
      return;
    }
    // Prefer IPv4 if available — some NTP pool members don't respond on v6.
    QHostAddress picked = info.addresses ().first ();
    for (QHostAddress const& a : info.addresses ()) {
      if (a.protocol () == QAbstractSocket::IPv4Protocol) {
        picked = a;
        break;
      }
    }
    server_ = picked;
    sendPacket ();
  });
}

void SntpClient::onLookup (int)
{
  // unused placeholder retained from earlier signal/slot attempt.
}

void SntpClient::cleanup ()
{
  if (socket_) {
    socket_->close ();
    socket_->deleteLater ();
    socket_ = nullptr;
  }
  if (timeout_) {
    timeout_->stop ();
    timeout_->deleteLater ();
    timeout_ = nullptr;
  }
  in_flight_ = false;
}

void SntpClient::sendPacket ()
{
  socket_ = new QUdpSocket (this);
  connect (socket_, &QUdpSocket::readyRead, this, &SntpClient::onReadyRead);

  timeout_ = new QTimer (this);
  timeout_->setSingleShot (true);
  connect (timeout_, &QTimer::timeout, this, &SntpClient::onTimeout);
  timeout_->start (TIMEOUT_MS);

  QByteArray pkt (48, '\0');
  pkt[0] = static_cast<char> (0x1B);   // LI=0, VN=3, Mode=3 (client)

  t1_unix_ms_ = QDateTime::currentMSecsSinceEpoch ();
  writeBigEndian64 (pkt, 40, unixMsToNtp64 (t1_unix_ms_));

  qint64 written = socket_->writeDatagram (pkt, server_, port_);
  if (written != pkt.size ()) {
    emit finished (0, false, tr ("Failed to send NTP request."));
    cleanup ();
    return;
  }
}

void SntpClient::onReadyRead ()
{
  if (!socket_) return;
  while (socket_->hasPendingDatagrams ()) {
    QByteArray pkt;
    pkt.resize (static_cast<int> (socket_->pendingDatagramSize ()));
    socket_->readDatagram (pkt.data (), pkt.size ());
    if (pkt.size () < 48) continue;

    qint64 t4 = QDateTime::currentMSecsSinceEpoch ();
    qint64 t2 = ntp64ToUnixMs (readBigEndian64 (pkt, 32));  // server receive
    qint64 t3 = ntp64ToUnixMs (readBigEndian64 (pkt, 40));  // server transmit

    quint8 liVnMode = static_cast<quint8> (pkt.at (0));
    int mode = liVnMode & 0x07;
    if (mode != 4 || t2 == 0 || t3 == 0) {
      emit finished (0, false, tr ("Malformed NTP response."));
      cleanup ();
      return;
    }

    qint64 offsetMs = ((t2 - t1_unix_ms_) + (t3 - t4)) / 2;
    emit finished (offsetMs, true, QString ());
    cleanup ();
    return;
  }
}

void SntpClient::onTimeout ()
{
  emit finished (0, false, tr ("NTP request timed out."));
  cleanup ();
}


// ── TimeSyncManager ────────────────────────────────────────────────────────

TimeSyncManager::TimeSyncManager (QObject * parent)
  : QObject {parent}
  , client_ {new SntpClient (this)}
  , refresh_ {new QTimer (this)}
{
  connect (client_, &SntpClient::finished, this, &TimeSyncManager::onFinished);
  connect (refresh_, &QTimer::timeout, this, &TimeSyncManager::queryNow);
}

void TimeSyncManager::startAutoRefresh (int intervalMs)
{
  if (intervalMs < 30000) intervalMs = 30000;
  refresh_->start (intervalMs);
  queryNow ();
}

void TimeSyncManager::queryNow ()
{
  client_->query (QStringLiteral ("pool.ntp.org"));
}

void TimeSyncManager::onFinished (qint64 offsetMs, bool ok, QString const& err)
{
  last_offset_ms_ = ok ? offsetMs : 0;
  last_ok_        = ok;
  last_error_     = err;
  emit offsetUpdated (last_offset_ms_, last_ok_, last_error_);
}

namespace {

#ifdef Q_OS_WIN
// Launch PowerShell elevated with a single -Command string. Returns
// true if ShellExecute handed off to the OS (UAC consent path); false
// on UAC refusal or launch failure.
bool runElevatedPowerShell (QString const& psCommand)
{
  QString const args = QStringLiteral (
      "-NoProfile -ExecutionPolicy Bypass -Command \"%1\"").arg (psCommand);
  std::wstring const wArgs = args.toStdWString ();
  HINSTANCE rc = ShellExecuteW (nullptr,
                                L"runas",
                                L"powershell.exe",
                                wArgs.c_str (),
                                nullptr,
                                SW_HIDE);
  return reinterpret_cast<INT_PTR> (rc) > 32;
}
#endif

QString const AUTO_SYNC_KEY = QStringLiteral ("timesync/auto_enabled");

} // anon

bool TimeSyncManager::elevatedSyncSystemClock (qint64 offsetMs)
{
#ifdef Q_OS_WIN
  // PowerShell Set-Date steps the system clock directly by the offset
  // we measured over SNTP. This does NOT depend on Windows Time
  // service being configured or running — it bypasses w32tm entirely
  // by calling the Win32 SetSystemTime API from inside the elevated
  // PowerShell process.
  QString const psCmd = QStringLiteral (
      "Set-Date (Get-Date).AddMilliseconds(%1)").arg (offsetMs);
  return runElevatedPowerShell (psCmd);
#else
  Q_UNUSED (offsetMs);
  return false;
#endif
}

bool TimeSyncManager::enableAutoSync10Min ()
{
#ifdef Q_OS_WIN
  // Reconfigure Windows Time Service to sync against pool.ntp.org
  // every 600 seconds. SpecialPollInterval is the registry key
  // w32time reads for the "0x8" special-interval flag on the peer.
  //
  // Commands are chained with ';' so one UAC prompt covers the lot.
  QString const psCmd = QStringLiteral (
      "Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\w32time\\TimeProviders\\NtpClient' -Name SpecialPollInterval -Value 600 -Type DWord;"
      " w32tm /config /manualpeerlist:'pool.ntp.org,0x8' /syncfromflags:manual /reliable:yes /update;"
      " Start-Service w32time -ErrorAction SilentlyContinue;"
      " Restart-Service w32time;"
      " w32tm /resync /force"
  );
  bool launched = runElevatedPowerShell (psCmd);
  if (launched) {
    QSettings s;
    s.setValue (AUTO_SYNC_KEY, true);
    s.sync ();
  }
  return launched;
#else
  return false;
#endif
}

bool TimeSyncManager::restoreDefaultSync ()
{
#ifdef Q_OS_WIN
  // Put w32time back to the Windows-default 7-day poll against
  // time.windows.com. Non-domain-joined machines see this as the
  // out-of-box configuration.
  QString const psCmd = QStringLiteral (
      "Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Services\\w32time\\TimeProviders\\NtpClient' -Name SpecialPollInterval -Value 604800 -Type DWord;"
      " w32tm /config /manualpeerlist:'time.windows.com,0x9' /syncfromflags:manual /update;"
      " Restart-Service w32time"
  );
  bool launched = runElevatedPowerShell (psCmd);
  if (launched) {
    QSettings s;
    s.setValue (AUTO_SYNC_KEY, false);
    s.sync ();
  }
  return launched;
#else
  return false;
#endif
}

bool TimeSyncManager::autoSyncEnabled ()
{
  QSettings s;
  return s.value (AUTO_SYNC_KEY, false).toBool ();
}

} // namespace wkjtx
