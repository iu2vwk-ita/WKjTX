#include "AdifExportDialog.hpp"

#include <QDateEdit>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>

namespace
{
  QString const LAST_EXPORT_KEY = QStringLiteral ("export/last_adif_date");
}

QDate AdifExportDialog::lastExportDate ()
{
  QSettings s;
  QString const iso = s.value (LAST_EXPORT_KEY).toString ();
  if (iso.isEmpty ()) return QDate {};
  return QDate::fromString (iso, Qt::ISODate);
}

void AdifExportDialog::rememberExport (QDate const& date)
{
  if (!date.isValid ()) return;
  QSettings s;
  s.setValue (LAST_EXPORT_KEY, date.toString (Qt::ISODate));
  s.sync ();
}

AdifExportDialog::AdifExportDialog (QWidget * parent)
  : QDialog {parent}
{
  setWindowTitle (tr ("Export ADIF log"));
  setModal (true);

  auto * root = new QVBoxLayout {this};

  auto * group = new QGroupBox {tr ("What to export"), this};
  auto * grid = new QGridLayout {group};

  rb_full_       = new QRadioButton {tr ("Full log (all QSOs)"), group};
  rb_since_last_ = new QRadioButton {tr ("Since last export"),   group};
  rb_7days_      = new QRadioButton {tr ("Last 7 days"),         group};
  rb_30days_     = new QRadioButton {tr ("Last 30 days"),        group};
  rb_range_      = new QRadioButton {tr ("Custom date range"),   group};

  QDate const lastExp = lastExportDate ();
  QString const lastStr = lastExp.isValid ()
      ? lastExp.toString (Qt::ISODate)
      : tr ("never");
  lbl_last_export_ = new QLabel (
      tr ("Previous export: %1").arg (lastStr), group);
  lbl_last_export_->setStyleSheet (QStringLiteral ("color: gray;"));
  if (!lastExp.isValid ()) {
    rb_since_last_->setEnabled (false);
  }

  de_from_ = new QDateEdit {group};
  de_from_->setCalendarPopup (true);
  de_from_->setDisplayFormat (QStringLiteral ("yyyy-MM-dd"));
  de_from_->setDate (QDate::currentDate ().addDays (-7));

  de_to_ = new QDateEdit {group};
  de_to_->setCalendarPopup (true);
  de_to_->setDisplayFormat (QStringLiteral ("yyyy-MM-dd"));
  de_to_->setDate (QDate::currentDate ());

  auto * lbl_from = new QLabel {tr ("From:"), group};
  auto * lbl_to   = new QLabel {tr ("To:"),   group};

  int row = 0;
  grid->addWidget (rb_full_,        row++, 0, 1, 4);
  grid->addWidget (rb_since_last_,  row,   0, 1, 3);
  grid->addWidget (lbl_last_export_, row++, 3, 1, 1);
  grid->addWidget (rb_7days_,       row++, 0, 1, 4);
  grid->addWidget (rb_30days_,      row++, 0, 1, 4);
  grid->addWidget (rb_range_,       row++, 0, 1, 4);
  grid->addWidget (lbl_from, row,   1);
  grid->addWidget (de_from_, row++, 2, 1, 2);
  grid->addWidget (lbl_to,   row,   1);
  grid->addWidget (de_to_,   row++, 2, 1, 2);

  root->addWidget (group);

  auto * bb = new QDialogButtonBox (
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  bb->button (QDialogButtonBox::Ok)->setText (tr ("Export..."));
  connect (bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect (bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget (bb);

  connect (rb_full_,       &QRadioButton::toggled, this, &AdifExportDialog::onModeChanged);
  connect (rb_since_last_, &QRadioButton::toggled, this, &AdifExportDialog::onModeChanged);
  connect (rb_7days_,      &QRadioButton::toggled, this, &AdifExportDialog::onModeChanged);
  connect (rb_30days_,     &QRadioButton::toggled, this, &AdifExportDialog::onModeChanged);
  connect (rb_range_,      &QRadioButton::toggled, this, &AdifExportDialog::onModeChanged);

  // Default: pick the most useful mode available.
  if (lastExp.isValid ()) rb_since_last_->setChecked (true);
  else rb_7days_->setChecked (true);
  onModeChanged ();
}

void AdifExportDialog::onModeChanged ()
{
  bool const customActive = rb_range_->isChecked ();
  de_from_->setEnabled (customActive);
  de_to_  ->setEnabled (customActive);
}

AdifExportDialog::Mode AdifExportDialog::selectedMode () const
{
  if (rb_full_->isChecked ())       return Mode::FullLog;
  if (rb_since_last_->isChecked ()) return Mode::SinceLastExport;
  if (rb_7days_->isChecked ())      return Mode::Last7Days;
  if (rb_30days_->isChecked ())     return Mode::Last30Days;
  return Mode::CustomRange;
}

QDate AdifExportDialog::fromDate () const
{
  QDate const today = QDate::currentDate ();
  switch (selectedMode ()) {
  case Mode::FullLog:          return QDate {};   // unbounded
  case Mode::SinceLastExport:  return lastExportDate ();
  case Mode::Last7Days:        return today.addDays (-6);
  case Mode::Last30Days:       return today.addDays (-29);
  case Mode::CustomRange:      return de_from_->date ();
  }
  return QDate {};
}

QDate AdifExportDialog::toDate () const
{
  switch (selectedMode ()) {
  case Mode::FullLog:          return QDate {};   // unbounded
  case Mode::SinceLastExport:
  case Mode::Last7Days:
  case Mode::Last30Days:       return QDate::currentDate ();
  case Mode::CustomRange:      return de_to_->date ();
  }
  return QDate {};
}
