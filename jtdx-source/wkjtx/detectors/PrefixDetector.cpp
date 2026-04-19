#include "PrefixDetector.hpp"

// Real implementation — ported from FT8 Card Pro's
// dxhunter/zones.py prefix_from_callsign().

#include <QSet>

namespace wkjtx {

namespace {

// Known DX prefixes that remain the "operating prefix" when a call is
// written as DX_PREFIX/BASE_CALL, e.g. "KH6/W1AW" → KH6.
// List is conservative — covers only the common cases.
QSet<QString> const & dxPrefixes ()
{
  static QSet<QString> const set = {
    "KH6", "KH8", "KH0", "KL7", "KP4",
    "VP9", "VP2", "VP8",
    "FO", "FR", "TF", "OY",
    "VK9"
  };
  return set;
}

} // namespace

PrefixDetector::PrefixDetector () = default;

void PrefixDetector::loadFromAdif (QString const & /*adifPath*/)
{
  // TODO v0.3: parse ADIF, compute prefix per QSO, fill workedPrefixes_.
}

void PrefixDetector::markWorked (QString const & prefix)
{
  if (!prefix.isEmpty ()) workedPrefixes_.insert (prefix.toUpper ());
}

bool PrefixDetector::isNewPrefix (QString const & callsign) const
{
  QString const p = prefixFromCallsign (callsign);
  if (p.isEmpty ()) return false;
  return !workedPrefixes_.contains (p);
}

QString PrefixDetector::prefixFromCallsign (QString const & callsign)
{
  if (callsign.isEmpty ()) return {};
  QString c = callsign.toUpper ().trimmed ();

  // Handle slash: DX prefix is on the left for known DX calls, otherwise
  // short right-side qualifiers (/P /M /MM /AM /QRP) mean "use the left";
  // anything else means "use the right" (a DX op roaming with their
  // home call as prefix, like "JA1ABC/VE3").
  int const slash = c.indexOf ('/');
  if (slash >= 0) {
    QString const left  = c.left (slash);
    QString const right = c.mid (slash + 1);
    if (dxPrefixes ().contains (left)) {
      c = left;
    } else if (right.size () <= 3) {
      // Short right side = qualifier (/P /M /MM /QRP etc.), keep left.
      c = left;
    } else {
      c = right;
    }
  }

  // Prefix = everything up to and INCLUDING the first digit.
  for (int i = 0; i < c.size (); ++i) {
    QChar const ch = c.at (i);
    if (ch.isDigit ()) {
      return c.left (i + 1);
    }
  }

  // No digit found (exotic special-event call) — fall back to first 3.
  return c.left (3);
}

} // namespace wkjtx
