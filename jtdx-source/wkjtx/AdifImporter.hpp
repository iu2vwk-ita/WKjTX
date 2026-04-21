#ifndef WKJTX_ADIF_IMPORTER_HPP
#define WKJTX_ADIF_IMPORTER_HPP

#include <QString>

// AdifImporter
// ------------
// One-shot ADIF log importer. Reads QSO records from a source .adi
// file, dedups against an existing destination .adi, and appends the
// net-new records. Dedup key is CALL + QSO_DATE + BAND + MODE (case
// normalised — ADIF spec says field contents are case-insensitive
// for the dedup-relevant fields).
//
// Stays independent from the JTDX LogBook/ADIF classes on purpose:
// we don't want to disturb the live log view, and this runs from
// the Settings dialog before the main window has a chance to
// reload. After import, next LogBook::load() picks up the appended
// records.
class AdifImporter
{
public:
  struct Stats
  {
    int totalRead = 0;       // records found in source file
    int imported = 0;        // records appended to destination
    int duplicates = 0;      // records skipped as already present
    int malformed = 0;       // records missing one of the 4 key fields
    bool ok = false;         // overall success (both files opened)
    QString errorMessage;    // set when ok == false
  };

  // Run the import. Creates destPath if it does not exist. Returns
  // filled-in stats. Safe to call from the UI thread — reads both
  // files into memory (ADIF logs are small, typically < a few MB).
  static Stats run (QString const& sourcePath, QString const& destPath);
};

#endif // WKJTX_ADIF_IMPORTER_HPP
