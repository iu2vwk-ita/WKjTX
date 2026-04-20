// Unit tests for LogRouter — verifies the shared-vs-per-profile
// routing decision and append-only semantics.

#include "../LogRouter.hpp"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>

class TestLogRouter : public QObject
{
  Q_OBJECT

private:
  QTemporaryDir tmp_;

  QString makePath (char const * name) const
  {
    return tmp_.path () + "/" + QString::fromUtf8 (name);
  }

private slots:
  void initTestCase ()
  {
    QVERIFY (tmp_.isValid ());
  }

  void routes_to_profile_when_nonempty ()
  {
    QString const shared  = makePath ("shared.adi");
    QString const profile = makePath ("profile.adi");
    wkjtx::LogRouter r;
    r.setSharedLogPath (shared);
    r.setProfileLogPath (profile);
    wkjtx::QsoRecord q;
    q.adifLine = "<CALL:5>K1ABC <EOR>";
    QVERIFY (r.logQso (q));
    QVERIFY (QFile::exists (profile));
    QVERIFY (!QFile::exists (shared));
  }

  void routes_to_shared_when_profile_empty ()
  {
    QString const shared = makePath ("shared2.adi");
    wkjtx::LogRouter r;
    r.setSharedLogPath (shared);
    r.setProfileLogPath ("");
    wkjtx::QsoRecord q;
    q.adifLine = "<CALL:5>W1ABC <EOR>";
    QVERIFY (r.logQso (q));
    QVERIFY (QFile::exists (shared));
  }

  void appends_without_truncating ()
  {
    QString const shared = makePath ("shared3.adi");
    wkjtx::LogRouter r;
    r.setSharedLogPath (shared);
    wkjtx::QsoRecord q1; q1.adifLine = "<CALL:5>AA1AA <EOR>";
    wkjtx::QsoRecord q2; q2.adifLine = "<CALL:5>BB2BB <EOR>";
    QVERIFY (r.logQso (q1));
    QVERIFY (r.logQso (q2));
    QFile f {shared};
    QVERIFY (f.open (QIODevice::ReadOnly | QIODevice::Text));
    QString const content = f.readAll ();
    QVERIFY (content.contains ("AA1AA"));
    QVERIFY (content.contains ("BB2BB"));
  }

  void returns_false_when_both_paths_empty ()
  {
    wkjtx::LogRouter r;
    r.setSharedLogPath ("");
    r.setProfileLogPath ("");
    wkjtx::QsoRecord q; q.adifLine = "x";
    QVERIFY (!r.logQso (q));
  }

  void emits_logFailed_on_bad_path ()
  {
    wkjtx::LogRouter r;
    r.setProfileLogPath ("/nonexistent_dir_xyz/cant_write_here.adi");
    QSignalSpy spy {&r, &wkjtx::LogRouter::logFailed};
    wkjtx::QsoRecord q; q.adifLine = "x";
    QVERIFY (!r.logQso (q));
    QCOMPARE (spy.count (), 1);
  }
};

QTEST_MAIN (TestLogRouter)
#include "test_log_router.moc"
