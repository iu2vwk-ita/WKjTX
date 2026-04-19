#ifndef WKJTX_AUTOCALL_SETTINGS_DIALOG_HPP
#define WKJTX_AUTOCALL_SETTINGS_DIALOG_HPP

// AutoCallSettingsDialog — standalone configuration dialog for the
// auto-call feature. Accessible from "File → Auto-call..." in the
// WKjTX menu bar.
//
// UI layout (amber-themed via global QSS):
//   - Red warning banner at top: unattended TX risk, amplifier risk.
//   - Master enable checkbox.
//   - 7 category checkboxes with labels.
//   - 5 line-edits for Alert callsigns.
//   - Safeguard info box (non-editable): 120s cooldown + 3/60s limit.
//   - OK / Cancel buttons.
//
// First time the user toggles ANY category ON, a confirmation dialog
// asks them to acknowledge the unattended-TX risk. Declining reverts
// the toggle.

#include <QDialog>
#include <QVector>

class QCheckBox;
class QLineEdit;
class QDialogButtonBox;

namespace wkjtx {

class AutoCall;
struct AutoCallConfig;

class AutoCallSettingsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AutoCallSettingsDialog (AutoCall * ac, QWidget * parent = nullptr);

  // Fills the UI from the current AutoCall config.
  void loadFromAutoCall ();

  // Reads the UI state into an AutoCallConfig and applies to AutoCall.
  void applyToAutoCall ();

public slots:
  void accept () override;

private slots:
  void onCategoryToggled (bool on);

private:
  AutoCall * ac_ {nullptr};

  QCheckBox * masterEnable_ {nullptr};

  QCheckBox * alertCheck_       {nullptr};
  QCheckBox * newDxccCheck_     {nullptr};
  QCheckBox * newCqZoneCheck_   {nullptr};
  QCheckBox * newItuZoneCheck_  {nullptr};
  QCheckBox * newGridCheck_     {nullptr};
  QCheckBox * newPrefixCheck_   {nullptr};
  QCheckBox * newCallsignCheck_ {nullptr};

  QVector<QLineEdit *> alertInputs_;   // 5 slots

  QDialogButtonBox * buttons_ {nullptr};

  bool firstEnableAcknowledged_ {false};

  void buildUi ();
  bool showFirstEnableConfirmation ();
};

} // namespace wkjtx

#endif // WKJTX_AUTOCALL_SETTINGS_DIALOG_HPP
