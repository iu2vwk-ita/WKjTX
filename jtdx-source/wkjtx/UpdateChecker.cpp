#include "UpdateChecker.hpp"

#include <QJsonDocument>
#include <QTimer>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>

namespace wkjtx {

namespace {

constexpr char kLatestReleaseUrl[] =
    "https://api.github.com/repos/iu2vwk-ita/WKjTX/releases/latest";

// Overall timeout. Qt5's QNetworkRequest has no per-request deadline
// before 5.15, so the reply is aborted by hand.
constexpr int kTimeoutMs = 15000;

struct ParsedVersion
{
  QVector<int> numbers;
  QString      preRelease;   // empty for a plain release
};

ParsedVersion parseVersion (QString const & raw)
{
  ParsedVersion v;
  QString s = raw.trimmed ();
  if (s.startsWith ('v') || s.startsWith ('V')) s = s.mid (1);

  int const dash = s.indexOf ('-');
  if (dash >= 0) {
    v.preRelease = s.mid (dash + 1);
    s = s.left (dash);
  }

  for (QString const & part : s.split ('.')) {
    bool ok = false;
    int const n = part.toInt (&ok);
    v.numbers.append (ok ? n : 0);
  }
  return v;
}

} // namespace

int compareVersions (QString const & a, QString const & b)
{
  ParsedVersion const va = parseVersion (a);
  ParsedVersion const vb = parseVersion (b);

  int const n = qMax (va.numbers.size (), vb.numbers.size ());
  for (int i = 0; i < n; ++i) {
    // A missing field counts as 0 so "1.4" == "1.4.0".
    int const x = i < va.numbers.size () ? va.numbers[i] : 0;
    int const y = i < vb.numbers.size () ? vb.numbers[i] : 0;
    if (x != y) return x < y ? -1 : 1;
  }

  // Same numbers: a release beats its own release candidates.
  bool const preA = !va.preRelease.isEmpty ();
  bool const preB = !vb.preRelease.isEmpty ();
  if (preA != preB) return preA ? -1 : 1;
  if (preA && preB) return QString::compare (va.preRelease, vb.preRelease,
                                             Qt::CaseInsensitive);
  return 0;
}

UpdateChecker::UpdateChecker (QString const & currentVersion, QObject * parent)
  : QObject {parent}
  , currentVersion_ {currentVersion}
  , nam_ {new QNetworkAccessManager {this}}
{}

UpdateChecker::~UpdateChecker () = default;

void UpdateChecker::check (bool userInitiated)
{
  if (inFlight_) return;

  QNetworkRequest req {QUrl {QString::fromLatin1 (kLatestReleaseUrl)}};
  req.setRawHeader ("Accept", "application/vnd.github+json");
  req.setHeader (QNetworkRequest::UserAgentHeader,
                 QStringLiteral ("WKjTX (+https://github.com/iu2vwk-ita/WKjTX)"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  req.setAttribute (QNetworkRequest::FollowRedirectsAttribute, true);
#endif

  inFlight_ = true;
  QNetworkReply * reply = nam_->get (req);

  // Hard deadline — abort() makes finished() fire with OperationCanceledError.
  QTimer * timeout = new QTimer {reply};
  timeout->setSingleShot (true);
  connect (timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
  timeout->start (kTimeoutMs);

  connect (reply, &QNetworkReply::finished, this,
           [this, reply, userInitiated] {
    reply->deleteLater ();
    inFlight_ = false;

    if (reply->error () != QNetworkReply::NoError) {
      emit checkFailed (reply->errorString (), userInitiated);
      return;
    }

    QJsonParseError err {};
    QJsonDocument const doc =
        QJsonDocument::fromJson (reply->readAll (), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject ()) {
      emit checkFailed (tr ("unreadable reply from GitHub"), userInitiated);
      return;
    }

    QJsonObject const o = doc.object ();
    QString const tag = o.value (QStringLiteral ("tag_name")).toString ();
    if (tag.isEmpty ()) {
      emit checkFailed (tr ("no release tag in reply"), userInitiated);
      return;
    }

    QString url = o.value (QStringLiteral ("html_url")).toString ();
    if (url.isEmpty ()) {
      url = QStringLiteral ("https://github.com/iu2vwk-ita/WKjTX/releases");
    }
    QString const notes = o.value (QStringLiteral ("body")).toString ();

    if (compareVersions (currentVersion_, tag) < 0) {
      emit updateAvailable (tag, url, notes, userInitiated);
    } else {
      emit upToDate (currentVersion_, userInitiated);
    }
  });
}

} // namespace wkjtx
