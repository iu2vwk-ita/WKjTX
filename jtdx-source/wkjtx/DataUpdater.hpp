#ifndef WKJTX_DATA_UPDATER_HPP
#define WKJTX_DATA_UPDATER_HPP

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

// DataUpdater
// -----------
// Sequentially downloads the runtime reference data files that WKjTX
// needs for country/state/grid lookup and LoTW user validation.
//
// Files (saved to QStandardPaths::DataLocation):
//   - cty.dat                  <- http://www.country-files.com/bigcty/cty.dat
//   - state_data.bin           <- https://sourceforge.net/projects/jtdx/files/state_data.bin
//   - grid_data.bin            <- https://sourceforge.net/projects/jtdx/files/grid_data.bin
//   - lotw-user-activity.csv   <- https://lotw.arrl.org/lotw-user-activity.csv
//
// Emits per-file progress and an allFinished() signal when the queue
// drains. Writes to "<name>.part" and renames on success to keep the
// existing file intact if a download fails mid-way.
//
// Single outstanding request; if updateAll() is called while busy it
// is ignored and busyRejected() is emitted.
class DataUpdater : public QObject
{
  Q_OBJECT
public:
  explicit DataUpdater (QNetworkAccessManager * qnam, QObject * parent = nullptr);
  ~DataUpdater () override;

  bool isBusy () const { return busy_; }

  // Well-known upstream defaults. Returned as plain strings so the
  // Settings dialog can pre-fill its line edits without depending on
  // QUrl semantics.
  static QString defaultCtyUrl ();
  static QString defaultStateUrl ();
  static QString defaultGridUrl ();
  static QString defaultLotwUrl ();

  // Override the URLs used by the next updateAll() call. Pass an
  // empty QUrl to revert that slot to its default.
  void setUrls (QUrl const& cty, QUrl const& state,
                QUrl const& grid, QUrl const& lotw);

public slots:
  // Kick off the download queue. Target directory is
  // QStandardPaths::DataLocation; created if missing.
  void updateAll ();

signals:
  // Called once per file as it starts.
  void fileStarted (QString const& filename);
  // Forwarded from the underlying QNetworkReply.
  void fileProgress (QString const& filename, qint64 received, qint64 total);
  // Called once per file once it is saved (success = true) or failed.
  void fileFinished (QString const& filename, bool success, QString const& message);
  // Called when the whole queue has drained.
  void allFinished (bool overallSuccess, QString const& summary);
  // Emitted if updateAll() is invoked while a previous run is still running.
  void busyRejected ();

private slots:
  void onReplyFinished ();
  void onReplyProgress (qint64 received, qint64 total);

private:
  struct Job {
    QUrl url;
    QString filename;   // target basename in DataLocation
  };

  void startNext ();
  QString targetDir () const;

  QNetworkAccessManager * qnam_;
  QVector<Job> queue_;
  int totalJobs_ = 0;
  int failures_ = 0;
  QStringList summaryLines_;
  QNetworkReply * currentReply_ = nullptr;
  Job currentJob_;
  bool busy_ = false;

  // URL overrides (empty => use default). The settings dialog writes
  // these via setUrls() right before calling updateAll().
  QUrl ctyUrl_;
  QUrl stateUrl_;
  QUrl gridUrl_;
  QUrl lotwUrl_;
};

#endif // WKJTX_DATA_UPDATER_HPP
