#ifndef WKJTX_GRID_DETECTOR_HPP
#define WKJTX_GRID_DETECTOR_HPP

// GridDetector — answers "is this decode's 4-char grid new?"
//
// 4-char Maidenhead (e.g. "JN45"). If the decode doesn't carry a grid,
// returns false (no trigger on unknown grids, to avoid false positives).
//
// Skeleton. Implementation in Plan 3 (WKjTX v0.3).

#include <QString>
#include <QSet>

namespace wkjtx {

class GridDetector
{
public:
  GridDetector ();

  void loadFromAdif (QString const & adifPath);
  void markWorked (QString const & grid4);
  bool isNewGrid (QString const & grid4) const;

  static bool isValidGrid4 (QString const & s);

private:
  QSet<QString> workedGrids_;
};

} // namespace wkjtx

#endif // WKJTX_GRID_DETECTOR_HPP
