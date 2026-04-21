#include "AdifImporter.hpp"

#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace
{
  // Scan a single ADIF record line for a named field.
  // Returns an empty string if absent. Matches both `<CALL:4>W1XT`
  // and `<CALL:4:S>W1XT` (data-type indicator). Case-insensitive on
  // the field name.
  //
  // Kept field-local — ADIF::_extractField in logbook/adif.cpp does
  // the same job but is private; rather than exposing it we copy
  // ~20 lines so this module stays self-contained.
  QString extractField (QString const& line, QString const& fieldName)
  {
    int fieldNameIndex = line.indexOf ('<' + fieldName, 0, Qt::CaseInsensitive);
    if (fieldNameIndex < 0) return QString {};

    int closingBracketIndex = line.indexOf ('>', fieldNameIndex);
    int fieldLengthIndex = line.indexOf (':', fieldNameIndex);
    if (fieldLengthIndex < 0 || fieldLengthIndex >= closingBracketIndex) return QString {};

    int dataTypeIndex = line.indexOf (':', fieldLengthIndex + 1);
    if (dataTypeIndex > closingBracketIndex) dataTypeIndex = -1;

    int fieldLengthCharCount = closingBracketIndex - fieldLengthIndex - 1;
    if (dataTypeIndex >= 0) fieldLengthCharCount -= 2;

    QString fieldLengthString = line.mid (fieldLengthIndex + 1, fieldLengthCharCount);
    bool okNum = false;
    int fieldLength = fieldLengthString.toInt (&okNum);
    if (!okNum || fieldLength <= 0) return QString {};

    return line.mid (closingBracketIndex + 1, fieldLength);
  }

  // Build a canonical dedup key from a record line. Empty if any
  // of the four key fields is missing (caller treats as malformed).
  QString dedupKey (QString const& line)
  {
    QString call = extractField (line, "CALL").trimmed ().toUpper ();
    QString date = extractField (line, "QSO_DATE").trimmed ();
    QString band = extractField (line, "BAND").trimmed ().toLower ();
    QString mode = extractField (line, "MODE").trimmed ().toUpper ();
    if (call.isEmpty () || date.isEmpty () || band.isEmpty () || mode.isEmpty ())
      {
        return QString {};
      }
    return call + QLatin1Char ('|') + date + QLatin1Char ('|') + band + QLatin1Char ('|') + mode;
  }

  // Iterate ADIF records in `fileContents`. ADIF records end with
  // <eor> (case-insensitive). Logs written by JTDX / WSJT-X put one
  // record per line, but the spec allows records to span lines, so
  // we split on <eor> rather than '\n'.
  QStringList splitRecords (QString const& fileContents)
  {
    QStringList out;
    int cursor = 0;
    QRegExp eor {"<eor>", Qt::CaseInsensitive};
    while (cursor < fileContents.size ())
      {
        int next = fileContents.indexOf (eor, cursor);
        if (next < 0) break;
        QString rec = fileContents.mid (cursor, next - cursor);
        out.append (rec);
        cursor = next + 5; // length of "<eor>"
      }
    return out;
  }
}

AdifImporter::Stats
AdifImporter::run (QString const& sourcePath, QString const& destPath)
{
  Stats s;

  QFile src {sourcePath};
  if (!src.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      s.errorMessage = QObject::tr ("Cannot open source file: %1").arg (src.errorString ());
      return s;
    }
  QString srcContents = QString::fromUtf8 (src.readAll ());
  src.close ();

  // Build set of keys already present at destination.
  QSet<QString> existingKeys;
  QFile dest {destPath};
  if (dest.exists ())
    {
      if (!dest.open (QIODevice::ReadOnly | QIODevice::Text))
        {
          s.errorMessage = QObject::tr ("Cannot read existing log: %1").arg (dest.errorString ());
          return s;
        }
      QString destContents = QString::fromUtf8 (dest.readAll ());
      dest.close ();
      for (QString const& rec : splitRecords (destContents))
        {
          QString k = dedupKey (rec);
          if (!k.isEmpty ()) existingKeys.insert (k);
        }
    }

  // Walk source records, append the net-new ones to destination.
  // Open in append mode so we don't rewrite the existing file.
  if (!dest.open (QIODevice::Append | QIODevice::Text))
    {
      s.errorMessage = QObject::tr ("Cannot open destination log for append: %1")
                         .arg (dest.errorString ());
      return s;
    }
  QTextStream out {&dest};

  QStringList srcRecords = splitRecords (srcContents);
  for (QString const& rec : srcRecords)
    {
      ++s.totalRead;
      QString k = dedupKey (rec);
      if (k.isEmpty ())
        {
          ++s.malformed;
          continue;
        }
      if (existingKeys.contains (k))
        {
          ++s.duplicates;
          continue;
        }
      existingKeys.insert (k);                  // de-dup within source itself too
      out << rec.trimmed () << "<eor>\n";       // normalise spacing
      ++s.imported;
    }
  out.flush ();
  dest.close ();

  s.ok = true;
  return s;
}
