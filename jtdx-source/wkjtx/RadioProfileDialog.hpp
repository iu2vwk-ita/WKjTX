#ifndef WKJTX_RADIO_PROFILE_DIALOG_HPP
#define WKJTX_RADIO_PROFILE_DIALOG_HPP

// RadioProfileDialog — compact editor for profile slots 2 and 3.
//
// Slot 1 = main app config (edited through the normal Settings dialog).
// Slot 2/3 = overlays stored in slotN.ini; this dialog edits only the
// radio + audio fields needed to swap rigs. It never touches the main
// JTDX.ini, so mistakes here can't break the base config.

#include <QDialog>
#include <QString>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;

namespace wkjtx {

class RadioProfileDialog : public QDialog
{
  Q_OBJECT

public:
  RadioProfileDialog (int slot, QString const & iniPath,
                      QStringList const & rigList,
                      QWidget * parent = nullptr);

  QString profileName () const;

private slots:
  void on_accept ();

private:
  void loadFromIni ();
  void saveToIni   ();
  void populateAudioInputs ();
  void populateAudioOutputs();

  int     slot_;
  QString iniPath_;

  QLineEdit * nameEdit_       {nullptr};

  // Radio group
  QComboBox * rigCombo_       {nullptr};
  QComboBox * catPortCombo_   {nullptr};
  QComboBox * baudCombo_      {nullptr};
  QComboBox * dataBitsCombo_  {nullptr};
  QComboBox * stopBitsCombo_  {nullptr};
  QComboBox * handshakeCombo_ {nullptr};
  QCheckBox * forceDtrCheck_  {nullptr};
  QComboBox * dtrCombo_       {nullptr};
  QCheckBox * forceRtsCheck_  {nullptr};
  QComboBox * rtsCombo_       {nullptr};
  QSpinBox  * pollSpin_       {nullptr};
  QComboBox * splitCombo_     {nullptr};

  // PTT
  QComboBox * pttMethodCombo_ {nullptr};
  QComboBox * pttPortCombo_   {nullptr};

  // Audio group
  QComboBox * audioInCombo_   {nullptr};
  QComboBox * audioInChCombo_ {nullptr};
  QComboBox * audioOutCombo_  {nullptr};
  QComboBox * audioOutChCombo_{nullptr};
  QComboBox * txAudioCombo_   {nullptr};
};

} // namespace wkjtx

#endif // WKJTX_RADIO_PROFILE_DIALOG_HPP
