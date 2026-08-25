#ifndef WKJTX_ADIF_UTILS_HPP
#define WKJTX_ADIF_UTILS_HPP

// Tiny ADIF helpers shared by the uploaders. Header-only: one function,
// no state, not worth a translation unit.

#include <QLatin1String>
#include <QString>

namespace wkjtx {

// Read one field out of an ADIF record: "<CALL:6>IU2VWK" -> "IU2VWK".
// Returns an empty string when the field is absent or malformed.
// fieldName is matched case-insensitively and without the angle
// brackets, e.g. adifField (record, "CALL").
inline QString adifField (QString const & adif, QString const & fieldName)
{
  QString const tag = QLatin1Char ('<') + fieldName + QLatin1Char (':');
  int const i = adif.indexOf (tag, 0, Qt::CaseInsensitive);
  if (i < 0) return {};
  int const colon = i + tag.size () - 1;
  int const gt = adif.indexOf ('>', colon + 1);
  if (gt < 0) return {};
  // The length field may carry an ADIF type suffix: <CALL:6:S>.
  QString lenStr = adif.mid (colon + 1, gt - colon - 1);
  int const typeSep = lenStr.indexOf (':');
  if (typeSep >= 0) lenStr = lenStr.left (typeSep);
  bool ok = false;
  int const len = lenStr.toInt (&ok);
  if (!ok || len <= 0 || gt + 1 + len > adif.size ()) return {};
  return adif.mid (gt + 1, len).trimmed ();
}

} // namespace wkjtx

#endif // WKJTX_ADIF_UTILS_HPP
