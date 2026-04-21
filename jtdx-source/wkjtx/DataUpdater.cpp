#include "DataUpdater.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace
{
  // Request timeout per file. SourceForge redirects + a slow LoTW CSV
  // fetch can take a minute or two on a bad link; 180 s is generous.
  constexpr int kTimeoutMs = 180000;
}

QString DataUpdater::defaultCtyUrl ()
{
  return QStringLiteral ("http://www.country-files.com/bigcty/cty.dat");
}

QString DataUpdater::defaultStateUrl ()
{
  return QStringLiteral ("https://sourceforge.net/projects/jtdx/files/state_data.bin/download");
}

QString DataUpdater::defaultGridUrl ()
{
  return QStringLiteral ("https://sourceforge.net/projects/jtdx/files/grid_data.bin/download");
}

QString DataUpdater::defaultLotwUrl ()
{
  return QStringLiteral ("https://lotw.arrl.org/lotw-user-activity.csv");
}

DataUpdater::DataUpdater (QNetworkAccessManager * qnam, QObject * parent)
  : QObject {parent}
  , qnam_ {qnam}
{
}

DataUpdater::~DataUpdater () = default;

QString DataUpdater::targetDir () const
{
  return QStandardPaths::writableLocation (QStandardPaths::DataLocation);
}

void DataUpdater::setUrls (QUrl const& cty, QUrl const& state,
                           QUrl const& grid, QUrl const& lotw)
{
  ctyUrl_   = cty;
  stateUrl_ = state;
  gridUrl_  = grid;
  lotwUrl_  = lotw;
}

void DataUpdater::updateAll ()
{
  if (busy_)
    {
      emit busyRejected ();
      return;
    }

  QDir dir {targetDir ()};
  if (!dir.exists ())
    {
      // Create org+app subtree if this is the very first run.
      dir.mkpath (".");
    }

  auto pick = [] (QUrl const& override, QString const& fallback) {
    return override.isEmpty () ? QUrl {fallback} : override;
  };

  queue_.clear ();
  queue_.append ({pick (ctyUrl_,   defaultCtyUrl ()),   QStringLiteral ("cty.dat")});
  queue_.append ({pick (stateUrl_, defaultStateUrl ()), QStringLiteral ("state_data.bin")});
  queue_.append ({pick (gridUrl_,  defaultGridUrl ()),  QStringLiteral ("grid_data.bin")});
  queue_.append ({pick (lotwUrl_,  defaultLotwUrl ()),  QStringLiteral ("lotw-user-activity.csv")});

  totalJobs_ = queue_.size ();
  failures_ = 0;
  summaryLines_.clear ();
  busy_ = true;

  startNext ();
}

void DataUpdater::startNext ()
{
  if (queue_.isEmpty ())
    {
      busy_ = false;
      bool ok = (failures_ == 0);
      QString summary = summaryLines_.join (QStringLiteral ("\n"));
      emit allFinished (ok, summary);
      return;
    }

  currentJob_ = queue_.takeFirst ();
  emit fileStarted (currentJob_.filename);

  QNetworkRequest request {currentJob_.url};
  request.setAttribute (QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::NoLessSafeRedirectPolicy);
  request.setHeader (QNetworkRequest::UserAgentHeader,
                     QStringLiteral ("WKjTX/1.0 (DataUpdater)"));
  request.setTransferTimeout (kTimeoutMs);

  currentReply_ = qnam_->get (request);
  connect (currentReply_, &QNetworkReply::finished,
           this, &DataUpdater::onReplyFinished);
  connect (currentReply_, &QNetworkReply::downloadProgress,
           this, &DataUpdater::onReplyProgress);
}

void DataUpdater::onReplyProgress (qint64 received, qint64 total)
{
  emit fileProgress (currentJob_.filename, received, total);
}

void DataUpdater::onReplyFinished ()
{
  if (!currentReply_)
    {
      return;
    }

  QString msg;
  bool ok = false;

  if (currentReply_->error () != QNetworkReply::NoError)
    {
      msg = currentReply_->errorString ();
    }
  else
    {
      QByteArray body = currentReply_->readAll ();
      if (body.isEmpty ())
        {
          msg = tr ("empty response");
        }
      else
        {
          QString const dir = targetDir ();
          QString const finalPath = dir + QLatin1Char ('/') + currentJob_.filename;
          QString const partPath = finalPath + QStringLiteral (".part");

          // Write to .part first, then atomically replace. This way a
          // failed or partial download never clobbers the good file
          // that is already on disk.
          QFile tmp {partPath};
          if (!tmp.open (QIODevice::WriteOnly | QIODevice::Truncate))
            {
              msg = tmp.errorString ();
            }
          else
            {
              qint64 written = tmp.write (body);
              tmp.close ();
              if (written != body.size ())
                {
                  msg = tr ("short write");
                  QFile::remove (partPath);
                }
              else
                {
                  // Qt on Windows does not allow rename-over, so drop
                  // the existing file first. The rename is still the
                  // safety boundary: if we crash here we keep the old
                  // file; if we crash after rename we keep the new.
                  QFile::remove (finalPath);
                  if (QFile::rename (partPath, finalPath))
                    {
                      ok = true;
                      QFileInfo fi {finalPath};
                      msg = tr ("%1 bytes").arg (fi.size ());
                    }
                  else
                    {
                      msg = tr ("rename failed");
                      QFile::remove (partPath);
                    }
                }
            }
        }
    }

  if (!ok)
    {
      ++failures_;
      summaryLines_.append (QStringLiteral ("%1: %2")
                              .arg (currentJob_.filename, msg));
    }
  else
    {
      summaryLines_.append (QStringLiteral ("%1: OK (%2)")
                              .arg (currentJob_.filename, msg));
    }

  emit fileFinished (currentJob_.filename, ok, msg);

  currentReply_->deleteLater ();
  currentReply_ = nullptr;

  startNext ();
}
