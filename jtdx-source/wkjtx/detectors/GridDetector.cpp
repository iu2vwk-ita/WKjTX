#include "GridDetector.hpp"

#include <QRegularExpression>

namespace wkjtx {

GridDetector::GridDetector () = default;

void GridDetector::loadFromAdif (QString const & /*adifPath*/) {}

void GridDetector::markWorked (QString const & grid4)
{
  if (isValidGrid4 (grid4)) workedGrids_.insert (grid4.toUpper ());
}

bool GridDetector::isNewGrid (QString const & grid4) const
{
  if (!isValidGrid4 (grid4)) return false;
  return !workedGrids_.contains (grid4.toUpper ());
}

bool GridDetector::isValidGrid4 (QString const & s)
{
  static QRegularExpression const re {R"(^[A-R]{2}[0-9]{2}$)",
                                      QRegularExpression::CaseInsensitiveOption};
  return re.match (s).hasMatch ();
}

} // namespace wkjtx
