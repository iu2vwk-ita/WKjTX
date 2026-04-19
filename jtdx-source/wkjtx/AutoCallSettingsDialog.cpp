#include "AutoCallSettingsDialog.hpp"
#include "AutoCall.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace wkjtx {

namespace {

constexpr char kAckKey[] = "autocall/firstEnableAcknowledged";

QString warningBannerHtml ()
{
  return QObject::tr (
    "<div style='padding:12px 14px; background:#3a1500; "
    "border:1px solid #ff6030; border-radius:6px;'>"
    "<b style='color:#ff9050; font-size:13px;'>⚠ Attenzione — uso responsabile</b>"
    "<br><span style='color:#e8d0c0;'>"
    "Con auto-call ON, un decoded call che corrisponde a una categoria "
    "attiva <b>trasmette automaticamente</b> una risposta. È tua "
    "responsabilità sapere cosa va in onda sotto la tua licenza. "
    "<b>Non lasciare mai la stazione incustodita con auto-call ON</b> "
    "se è collegata a un amplificatore o a una ciabatta con timer."
    "</span></div>");
}

QString safeguardInfoHtml ()
{
  return QObject::tr (
    "<div style='padding:8px 10px; background:#14110d; "
    "border:1px solid #2a2218; border-radius:4px; color:#a69070;'>"
    "<b>Safeguard hardcoded</b>: ogni callsign viene chiamato al "
    "massimo una volta ogni <b>120 s</b>. Limite globale: <b>3 "
    "auto-call per minuto</b>. Questi valori non sono modificabili "
    "dall'utente per ragioni di sicurezza."
    "</div>");
}

} // namespace

AutoCallSettingsDialog::AutoCallSettingsDialog (AutoCall * ac, QWidget * parent)
  : QDialog {parent}
  , ac_ {ac}
{
  setWindowTitle (tr ("Auto-call — WKjTX"));
  setModal (true);
  resize (540, 620);

  QSettings settings;
  firstEnableAcknowledged_ = settings.value (kAckKey, false).toBool ();

  buildUi ();
  if (ac_) loadFromAutoCall ();
}

void AutoCallSettingsDialog::buildUi ()
{
  auto * mainLayout = new QVBoxLayout (this);
  mainLayout->setSpacing (10);

  // Warning banner.
  auto * warning = new QLabel (warningBannerHtml (), this);
  warning->setWordWrap (true);
  warning->setTextFormat (Qt::RichText);
  mainLayout->addWidget (warning);

  // Master enable.
  masterEnable_ = new QCheckBox (tr ("Abilita auto-call (master switch)"), this);
  QFont f = masterEnable_->font ();
  f.setBold (true);
  masterEnable_->setFont (f);
  mainLayout->addWidget (masterEnable_);

  // Categories group.
  auto * catGroup = new QGroupBox (tr ("Categorie trigger"), this);
  auto * catLayout = new QGridLayout (catGroup);

  struct CatRow {
    QCheckBox ** target;
    char const * label;
    char const * desc;
  };
  CatRow const rows[] = {
    {&alertCheck_,      QT_TR_NOOP ("Alert callsigns (vedi sotto 5 slot)"),
                        QT_TR_NOOP ("I 5 nominativi che hai scelto esplicitamente")},
    {&newDxccCheck_,    QT_TR_NOOP ("NEW DXCC entity"),
                        QT_TR_NOOP ("Paese DXCC mai lavorato")},
    {&newCqZoneCheck_,  QT_TR_NOOP ("NEW CQ zone"),
                        QT_TR_NOOP ("Zona CQ mai lavorata (dal grid 4-char)")},
    {&newItuZoneCheck_, QT_TR_NOOP ("NEW ITU zone"),
                        QT_TR_NOOP ("Zona ITU mai lavorata")},
    {&newGridCheck_,    QT_TR_NOOP ("NEW grid (4-char)"),
                        QT_TR_NOOP ("Locator 4-char mai lavorato")},
    {&newPrefixCheck_,  QT_TR_NOOP ("NEW prefix"),
                        QT_TR_NOOP ("Prefisso operatore mai lavorato")},
    {&newCallsignCheck_,QT_TR_NOOP ("NEW callsign"),
                        QT_TR_NOOP ("Callsign esatto mai lavorato")},
  };

  int row = 0;
  for (auto const & r : rows) {
    *r.target = new QCheckBox (tr (r.label), catGroup);
    auto * desc = new QLabel (tr (r.desc), catGroup);
    desc->setStyleSheet (QStringLiteral ("color: #a69070; font-size: 10px;"));
    catLayout->addWidget (*r.target, row, 0);
    catLayout->addWidget (desc,       row, 1);
    connect (*r.target, &QCheckBox::toggled,
             this, &AutoCallSettingsDialog::onCategoryToggled);
    ++row;
  }
  catLayout->setColumnStretch (1, 1);
  mainLayout->addWidget (catGroup);

  // Alert callsign slots.
  auto * alertGroup = new QGroupBox (tr ("Alert callsigns (5 slot)"), this);
  auto * alertLayout = new QGridLayout (alertGroup);
  for (int i = 0; i < 5; ++i) {
    auto * lbl = new QLabel (tr ("Slot %1:").arg (i + 1), alertGroup);
    auto * edit = new QLineEdit (alertGroup);
    edit->setPlaceholderText (tr ("es. K1ABC, DL1XYZ, ..."));
    edit->setMaxLength (16);
    alertInputs_.append (edit);
    alertLayout->addWidget (lbl,  i, 0);
    alertLayout->addWidget (edit, i, 1);
  }
  alertLayout->setColumnStretch (1, 1);
  mainLayout->addWidget (alertGroup);

  // Safeguard info.
  auto * safe = new QLabel (safeguardInfoHtml (), this);
  safe->setWordWrap (true);
  safe->setTextFormat (Qt::RichText);
  mainLayout->addWidget (safe);

  mainLayout->addStretch ();

  // Dialog buttons.
  buttons_ = new QDialogButtonBox (
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect (buttons_, &QDialogButtonBox::accepted,
           this, &AutoCallSettingsDialog::accept);
  connect (buttons_, &QDialogButtonBox::rejected,
           this, &AutoCallSettingsDialog::reject);
  mainLayout->addWidget (buttons_);
}

void AutoCallSettingsDialog::loadFromAutoCall ()
{
  if (!ac_) return;
  auto const & cfg = ac_->config ();
  masterEnable_->setChecked (cfg.masterEnable);

  struct Bind { QCheckBox * box; AutoCallCategory cat; };
  Bind const binds[] = {
    {alertCheck_,       AutoCallCategory::Alert},
    {newDxccCheck_,     AutoCallCategory::NewDxcc},
    {newCqZoneCheck_,   AutoCallCategory::NewCqZone},
    {newItuZoneCheck_,  AutoCallCategory::NewItuZone},
    {newGridCheck_,     AutoCallCategory::NewGrid},
    {newPrefixCheck_,   AutoCallCategory::NewPrefix},
    {newCallsignCheck_, AutoCallCategory::NewCallsign},
  };
  for (auto const & b : binds) {
    b.box->blockSignals (true);
    b.box->setChecked (cfg.categoryEnabled.value (b.cat, false));
    b.box->blockSignals (false);
  }
  for (int i = 0; i < alertInputs_.size () && i < cfg.alertCallsigns.size (); ++i) {
    alertInputs_[i]->setText (cfg.alertCallsigns[i]);
  }
}

void AutoCallSettingsDialog::applyToAutoCall ()
{
  if (!ac_) return;
  AutoCallConfig cfg;
  cfg.masterEnable = masterEnable_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::Alert]       = alertCheck_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::NewDxcc]     = newDxccCheck_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::NewCqZone]   = newCqZoneCheck_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::NewItuZone]  = newItuZoneCheck_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::NewGrid]     = newGridCheck_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::NewPrefix]   = newPrefixCheck_->isChecked ();
  cfg.categoryEnabled[AutoCallCategory::NewCallsign] = newCallsignCheck_->isChecked ();
  cfg.alertCallsigns.clear ();
  for (auto * edit : alertInputs_) {
    QString const s = edit->text ().trimmed ().toUpper ();
    if (!s.isEmpty ()) cfg.alertCallsigns.append (s);
  }
  ac_->setConfig (cfg);
}

void AutoCallSettingsDialog::onCategoryToggled (bool on)
{
  if (!on) return;                    // only guard on enabling
  if (firstEnableAcknowledged_) return;
  auto * box = qobject_cast<QCheckBox *> (sender ());
  if (!box) return;
  if (!showFirstEnableConfirmation ()) {
    box->blockSignals (true);
    box->setChecked (false);
    box->blockSignals (false);
  } else {
    firstEnableAcknowledged_ = true;
    QSettings {}.setValue (kAckKey, true);
  }
}

bool AutoCallSettingsDialog::showFirstEnableConfirmation ()
{
  QMessageBox box {this};
  box.setIcon (QMessageBox::Warning);
  box.setWindowTitle (tr ("Auto-call — conferma richiesta"));
  box.setText (tr ("<b>Abilitare auto-call?</b>"));
  box.setInformativeText (tr (
      "Auto-call trasmette <b>automaticamente</b> quando un decode matcha "
      "una categoria attiva. È tua responsabilità sapere cosa va in onda.\n\n"
      "Safeguard integrati: cooldown 120 s per callsign + 3 auto-call al "
      "minuto globali.\n\n"
      "<b>Non lasciare mai la stazione incustodita con auto-call ON.</b>"));
  box.setStandardButtons (QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton (QMessageBox::No);
  return box.exec () == QMessageBox::Yes;
}

void AutoCallSettingsDialog::accept ()
{
  applyToAutoCall ();
  QDialog::accept ();
}

} // namespace wkjtx
