#ifndef WKJTX_ADIF_EXPORTER_HPP
#define WKJTX_ADIF_EXPORTER_HPP

#include <QDate>
#include <QString>

// AdifExporter
// ------------
// Copies the internal wsjtx_log.adi into a user-chosen ADIF file,
// optionally filtering QSO records by their QSO_DATE field.
//
// If both fromDate and toDate are invalid QDates, every record in
// the source is copied (equivalent to the pre-v1.1.1 "full log"
// behaviour).
//
// Records whose QSO_DATE field is missing or not parseable as
// YYYYMMDD are counted as `malformedDate` and are **excluded**
// from any date-filtered export (including them without knowing
// when they happened would break the filter contract). In full-log
// mode they are kept.
//
// The ADIF header (everything before the first <EOH>, case-
// insensitive) is copied verbatim; if the source has no <EOH>
// we write a minimal header.
class AdifExporter
{
public:
  struct Stats
  {
    int totalRead = 0;          // records found in source
    int exported = 0;           // records written to destination
    int skippedBelow = 0;       // date < fromDate
    int skippedAbove = 0;       // date > toDate
    int malformedDate = 0;      // QSO_DATE missing / not YYYYMMDD
    bool ok = false;
    QString errorMessage;
  };

  static Stats run (QString const& sourcePath, QString const& destPath,
                    QDate const& fromDate, QDate const& toDate);
};

#endif // WKJTX_ADIF_EXPORTER_HPP
