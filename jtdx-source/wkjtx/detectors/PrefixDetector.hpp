#ifndef WKJTX_PREFIX_DETECTOR_HPP
#define WKJTX_PREFIX_DETECTOR_HPP

// PrefixDetector — answers "is this decode's callsign prefix one I
// haven't worked?"
//
// "Prefix" here means the operator prefix in the amateur radio sense
// (e.g. IU2, K1, PY2, not the DXCC entity). One DXCC maps to multiple
// prefixes.
//
// Skeleton. Implementation in Plan 3 (WKjTX v0.3).

#include <QString>
#include <QSet>

namespace wkjtx {

class PrefixDetector
{
public:
  PrefixDetector ();

  void loadFromAdif (QString const & adifPath);
  void markWorked (QString const & prefix);
  bool isNewPrefix (QString const & callsign) const;

  static QString prefixFromCallsign (QString const & callsign);

private:
  QSet<QString> workedPrefixes_;
};

} // namespace wkjtx

#endif // WKJTX_PREFIX_DETECTOR_HPP
