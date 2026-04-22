#ifndef WKJTX_ADIF_EXPORT_DIALOG_HPP
#define WKJTX_ADIF_EXPORT_DIALOG_HPP

#include <QDate>
#include <QDialog>

class QRadioButton;
class QDateEdit;
class QLabel;

// AdifExportDialog
// ----------------
// Lets the user pick what slice of the log to export:
//   - Full log              (no filter)
//   - Since last export     (date of previous Export action)
//   - Last 7 days
//   - Last 30 days
//   - Custom date range     (from ... to ...)
//
// After accept(), `fromDate()` and `toDate()` return the effective
// range (either may be invalid == unbounded). Call
// `rememberExport(QDate::currentDate())` after a successful export
// to update the "Since last export" anchor.
class AdifExportDialog : public QDialog
{
  Q_OBJECT

public:
  enum class Mode
  {
    FullLog,
    SinceLastExport,
    Last7Days,
    Last30Days,
    CustomRange
  };

  explicit AdifExportDialog (QWidget * parent = nullptr);

  Mode selectedMode () const;
  QDate fromDate () const;
  QDate toDate () const;

  // Persisted anchor for the "Since last export" option. Reads /
  // writes QSettings key "export/last_adif_date" as ISO yyyy-MM-dd.
  static QDate lastExportDate ();
  static void  rememberExport (QDate const& date);

private slots:
  void onModeChanged ();

private:
  QRadioButton * rb_full_;
  QRadioButton * rb_since_last_;
  QRadioButton * rb_7days_;
  QRadioButton * rb_30days_;
  QRadioButton * rb_range_;

  QDateEdit * de_from_;
  QDateEdit * de_to_;
  QLabel   * lbl_last_export_;
};

#endif // WKJTX_ADIF_EXPORT_DIALOG_HPP
