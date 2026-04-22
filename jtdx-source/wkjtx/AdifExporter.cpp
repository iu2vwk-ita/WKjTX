#include "AdifExporter.hpp"

#include <QFile>
#include <QRegExp>
#include <QTextStream>

namespace
{
  // Extract the value of an ADIF field from a record string.
  // Matches <NAME:LEN>value and <NAME:LEN:TYPE>value, case-
  // insensitive on the tag name. Returns empty if absent or
  // malformed. Same logic as in AdifImporter.cpp — copied to
  // keep this module self-contained.
  QString extractField (QString const& rec, QString const& fieldName)
  {
    int nameIdx = rec.indexOf ('<' + fieldName, 0, Qt::CaseInsensitive);
    if (nameIdx < 0) return QString {};

    int closeIdx = rec.indexOf ('>', nameIdx);
    int lenIdx = rec.indexOf (':', nameIdx);
    if (lenIdx < 0 || lenIdx >= closeIdx) return QString {};

    int typeIdx = rec.indexOf (':', lenIdx + 1);
    if (typeIdx > closeIdx) typeIdx = -1;

    int lenCharCount = closeIdx - lenIdx - 1;
    if (typeIdx >= 0) lenCharCount -= 2;

    QString lenStr = rec.mid (lenIdx + 1, lenCharCount);
    bool ok = false;
    int len = lenStr.toInt (&ok);
    if (!ok || len <= 0) return QString {};

    return rec.mid (closeIdx + 1, len);
  }

  // Parse an ADIF QSO_DATE string (YYYYMMDD) into QDate. Returns
  // invalid QDate on failure.
  QDate parseAdifDate (QString const& s)
  {
    QString t = s.trimmed ();
    if (t.length () != 8) return QDate {};
    return QDate::fromString (t, QStringLiteral ("yyyyMMdd"));
  }

  // Split source into (header, records). Records are everything
  // after the first <EOH>. If no <EOH> the source is treated as
  // header-less (records start at offset 0).
  void splitHeaderAndBody (QString const& src, QString & header, QString & body)
  {
    QRegExp eoh {"<eoh>", Qt::CaseInsensitive};
    int eohIdx = src.indexOf (eoh);
    if (eohIdx < 0) {
      header = QStringLiteral ("WKjTX filtered ADIF export\n<EOH>\n");
      body = src;
    } else {
      int afterEoh = eohIdx + 5; // length of "<eoh>"
      header = src.left (afterEoh);
      // Skip a newline immediately after <EOH> if present, the header
      // ends cleanly.
      if (afterEoh < src.size () && src.at (afterEoh) == QChar ('\n')) {
        header += QChar ('\n');
        ++afterEoh;
      } else if (afterEoh + 1 < src.size ()
                 && src.at (afterEoh) == QChar ('\r')
                 && src.at (afterEoh + 1) == QChar ('\n')) {
        header += QStringLiteral ("\r\n");
        afterEoh += 2;
      }
      body = src.mid (afterEoh);
    }
  }
}

AdifExporter::Stats
AdifExporter::run (QString const& sourcePath, QString const& destPath,
                   QDate const& fromDate, QDate const& toDate)
{
  Stats s;

  QFile src {sourcePath};
  if (!src.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      s.errorMessage = QObject::tr ("Cannot open source file: %1")
                         .arg (src.errorString ());
      return s;
    }
  QString const srcContents = QString::fromUtf8 (src.readAll ());
  src.close ();

  QFile dest {destPath};
  if (!dest.open (QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
      s.errorMessage = QObject::tr ("Cannot open destination file: %1")
                         .arg (dest.errorString ());
      return s;
    }
  QTextStream out {&dest};

  bool const filtering = fromDate.isValid () || toDate.isValid ();

  QString header, body;
  splitHeaderAndBody (srcContents, header, body);
  out << header;

  // Walk records. ADIF record terminator is <EOR> (case-insensitive).
  QRegExp eor {"<eor>", Qt::CaseInsensitive};
  int cursor = 0;
  while (cursor < body.size ())
    {
      int next = body.indexOf (eor, cursor);
      if (next < 0) break;
      QString rec = body.mid (cursor, next - cursor);
      cursor = next + 5; // length of "<eor>"

      ++s.totalRead;

      if (filtering) {
        QString dateStr = extractField (rec, QStringLiteral ("QSO_DATE"));
        QDate qsoDate = parseAdifDate (dateStr);
        if (!qsoDate.isValid ()) {
          ++s.malformedDate;
          continue;
        }
        if (fromDate.isValid () && qsoDate < fromDate) {
          ++s.skippedBelow;
          continue;
        }
        if (toDate.isValid () && qsoDate > toDate) {
          ++s.skippedAbove;
          continue;
        }
      }

      out << rec.trimmed () << "<EOR>\n";
      ++s.exported;
    }

  out.flush ();
  dest.close ();

  s.ok = true;
  return s;
}
