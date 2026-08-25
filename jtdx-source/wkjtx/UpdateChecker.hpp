#ifndef WKJTX_UPDATE_CHECKER_HPP
#define WKJTX_UPDATE_CHECKER_HPP

// UpdateChecker — v1.4.0. Polls the GitHub Releases API for a newer
// WKjTX tag.
//
//   GET https://api.github.com/repos/iu2vwk-ita/WKjTX/releases/latest
//
// The portable builds do not install anything and never phone home on
// their own, so without this check a user who unzipped v1.0 stays on
// v1.0 forever. The check is a single unauthenticated GET; GitHub
// allows 60/hour per IP, which is far above what one desktop app does.
//
// Draft and pre-release tags are skipped by the /latest endpoint
// itself, so anything this class reports is a published release.

#include <QObject>
#include <QString>

class QNetworkAccessManager;

namespace wkjtx {

// Compare two version strings ("1.3.0", "v1.4", "1.4.0-rc2").
// Returns <0 if a is older than b, 0 if equal, >0 if a is newer.
// A release-candidate suffix sorts BEFORE the plain release of the
// same numbers: 1.4.0-rc1 < 1.4.0.
int compareVersions (QString const & a, QString const & b);

class UpdateChecker : public QObject
{
  Q_OBJECT

public:
  // currentVersion is the running build's version string, e.g. "1.3.0";
  // it is injected rather than read from revision_utils so this module
  // stays linkable on its own (tests, standalone build).
  explicit UpdateChecker (QString const & currentVersion,
                          QObject * parent = nullptr);
  ~UpdateChecker () override;

  // Starts a check. userInitiated only travels back out through the
  // signals so the UI can stay silent for the automatic startup check
  // and talk for the menu-driven one.
  void check (bool userInitiated);

  bool busy () const { return inFlight_; }

signals:
  void updateAvailable (QString tag, QString url, QString notes,
                        bool userInitiated);
  void upToDate (QString currentVersion, bool userInitiated);
  void checkFailed (QString error, bool userInitiated);

private:
  QString currentVersion_;
  QNetworkAccessManager * nam_ {nullptr};
  bool inFlight_ {false};
};

} // namespace wkjtx

#endif // WKJTX_UPDATE_CHECKER_HPP
