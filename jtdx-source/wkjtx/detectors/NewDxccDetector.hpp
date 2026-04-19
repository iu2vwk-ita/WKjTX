#ifndef WKJTX_NEW_DXCC_DETECTOR_HPP
#define WKJTX_NEW_DXCC_DETECTOR_HPP

// NewDxccDetector — answers "is this decode's callsign a DXCC entity
// I haven't worked yet?"
//
// Works off an in-memory worked-before index populated at startup by
// scanning the active ADIF log (shared or per-profile). The index
// grows by one when a QSO is logged.
//
// Skeleton. Implementation in Plan 3 (WKjTX v0.3). Reference Python
// logic at G:\Claude Local\BACKUP APP\ft8-companion\dxhunter\detector.py.

#include <QString>
#include <QSet>

namespace wkjtx {

class NewDxccDetector
{
public:
  NewDxccDetector ();

  // Load worked DXCC prefixes from an ADIF file path. Safe to call
  // repeatedly; replaces the in-memory set.
  void loadFromAdif (QString const & adifPath);

  // Mark a DXCC as "now worked" after a QSO is logged.
  void markWorked (QString const & dxccPrefix);

  // Query: has the callsign's DXCC been worked? Returns true if
  // UN-worked (= "NEW" = trigger candidate).
  bool isNewDxcc (QString const & callsign) const;

  // Extract DXCC prefix from a callsign using CTY.DAT-style rules.
  // Returns empty QString if the callsign can't be mapped.
  static QString dxccFromCallsign (QString const & callsign);

private:
  QSet<QString> workedDxcc_;
};

} // namespace wkjtx

#endif // WKJTX_NEW_DXCC_DETECTOR_HPP
