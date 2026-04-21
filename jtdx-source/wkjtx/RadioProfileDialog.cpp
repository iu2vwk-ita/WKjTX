#include "RadioProfileDialog.hpp"
#include "../TransceiverFactory.hpp"

#include <QCheckBox>
#include <QtSerialPort/QSerialPortInfo>
#include <QStringList>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtMultimedia/QAudioDeviceInfo>

namespace wkjtx {

// Minimal editor for slot 2/3. Writes only to the slot's own INI file —
// nothing here touches the main JTDX.ini.

RadioProfileDialog::RadioProfileDialog (int slot, QString const & iniPath,
                                        QStringList const & rigList,
                                        QWidget * parent)
  : QDialog {parent}
  , slot_   {slot}
  , iniPath_{iniPath}
{
  setWindowTitle (tr ("Configure Radio profile %1").arg (slot));
  setModal (true);

  auto * main = new QVBoxLayout (this);

  // ── Name ─────────────────────────────────────────────────────────────
  auto * nameRow = new QFormLayout;
  nameEdit_ = new QLineEdit (this);
  nameEdit_->setPlaceholderText (tr ("e.g. IC-705, FT-891"));
  nameRow->addRow (tr ("Profile name:"), nameEdit_);
  main->addLayout (nameRow);

  // ── Radio (General) ──────────────────────────────────────────────────
  auto * rigBox   = new QGroupBox (tr ("General"), this);
  auto * rigForm  = new QFormLayout (rigBox);
  rigCombo_ = new QComboBox (this);
  rigCombo_->addItems (rigList);
  rigForm->addRow (tr ("Rig:"), rigCombo_);
  catPortCombo_ = new QComboBox (this);
  catPortCombo_->addItem (QString {});
  for (auto const & p : QSerialPortInfo::availablePorts ())
    catPortCombo_->addItem (p.portName ());
  rigForm->addRow (tr ("CAT Serial Port:"), catPortCombo_);
  baudCombo_ = new QComboBox (this);
  for (int b : {1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200})
    baudCombo_->addItem (QString::number (b), b);
  baudCombo_->setCurrentText ("38400");
  rigForm->addRow (tr ("Baud Rate:"), baudCombo_);
  main->addWidget (rigBox);

  // ── Radio handshake ──────────────────────────────────────────────────
  auto * catBox  = new QGroupBox (tr ("Radio"), this);
  auto * catForm = new QFormLayout (catBox);
  dataBitsCombo_ = new QComboBox (this);
  dataBitsCombo_->addItem ("7 (Seven)",  TransceiverFactory::seven_data_bits);
  dataBitsCombo_->addItem ("8 (Eight)",  TransceiverFactory::eight_data_bits);
  dataBitsCombo_->setCurrentIndex (1);
  catForm->addRow (tr ("Data Bits:"), dataBitsCombo_);
  stopBitsCombo_ = new QComboBox (this);
  stopBitsCombo_->addItem ("1 (One)",  TransceiverFactory::one_stop_bit);
  stopBitsCombo_->addItem ("2 (Two)",  TransceiverFactory::two_stop_bits);
  catForm->addRow (tr ("Stop Bits:"), stopBitsCombo_);
  handshakeCombo_ = new QComboBox (this);
  handshakeCombo_->addItem ("None",       TransceiverFactory::handshake_default);
  handshakeCombo_->addItem ("XON/XOFF",   TransceiverFactory::handshake_XonXoff);
  handshakeCombo_->addItem ("Hardware",   TransceiverFactory::handshake_hardware);
  catForm->addRow (tr ("Handshake:"), handshakeCombo_);
  forceDtrCheck_ = new QCheckBox (tr ("Force Control Lines"), this);
  catForm->addRow (QString {}, forceDtrCheck_);
  dtrCombo_ = new QComboBox (this);
  dtrCombo_->addItem ("DTR High", true);
  dtrCombo_->addItem ("DTR Low",  false);
  catForm->addRow (tr ("DTR:"), dtrCombo_);
  forceRtsCheck_ = new QCheckBox (tr ("Force RTS"), this);
  catForm->addRow (QString {}, forceRtsCheck_);
  rtsCombo_ = new QComboBox (this);
  rtsCombo_->addItem ("RTS High", true);
  rtsCombo_->addItem ("RTS Low",  false);
  catForm->addRow (tr ("RTS:"), rtsCombo_);
  pollSpin_ = new QSpinBox (this);
  pollSpin_->setRange (0, 3600);
  pollSpin_->setSuffix (" s");
  pollSpin_->setValue (0);
  catForm->addRow (tr ("Poll Interval:"), pollSpin_);
  splitCombo_ = new QComboBox (this);
  splitCombo_->addItem (tr ("None"),       TransceiverFactory::split_mode_none);
  splitCombo_->addItem (tr ("Rig"),        TransceiverFactory::split_mode_rig);
  splitCombo_->addItem (tr ("Fake It"),    TransceiverFactory::split_mode_emulate);
  catForm->addRow (tr ("Split Operation:"), splitCombo_);
  pttMethodCombo_ = new QComboBox (this);
  pttMethodCombo_->addItem ("VOX", TransceiverFactory::PTT_method_VOX);
  pttMethodCombo_->addItem ("CAT", TransceiverFactory::PTT_method_CAT);
  pttMethodCombo_->addItem ("DTR", TransceiverFactory::PTT_method_DTR);
  pttMethodCombo_->addItem ("RTS", TransceiverFactory::PTT_method_RTS);
  catForm->addRow (tr ("PTT Method:"), pttMethodCombo_);
  pttPortCombo_ = new QComboBox (this);
  pttPortCombo_->addItem (QString {});
  for (auto const & p : QSerialPortInfo::availablePorts ())
    pttPortCombo_->addItem (p.portName ());
  catForm->addRow (tr ("PTT Port:"), pttPortCombo_);
  main->addWidget (catBox);

  // ── Audio ────────────────────────────────────────────────────────────
  auto * audioBox  = new QGroupBox (tr ("Audio"), this);
  auto * audioForm = new QFormLayout (audioBox);
  audioInCombo_ = new QComboBox (this);
  populateAudioInputs ();
  audioForm->addRow (tr ("Input Device:"), audioInCombo_);
  audioInChCombo_ = new QComboBox (this);
  audioInChCombo_->addItems ({"Mono", "Left", "Right"});
  audioForm->addRow (tr ("Input Channel:"), audioInChCombo_);
  audioOutCombo_ = new QComboBox (this);
  populateAudioOutputs ();
  audioForm->addRow (tr ("Output Device:"), audioOutCombo_);
  audioOutChCombo_ = new QComboBox (this);
  audioOutChCombo_->addItems ({"Mono", "Left", "Right", "Both"});
  audioForm->addRow (tr ("Output Channel:"), audioOutChCombo_);
  txAudioCombo_ = new QComboBox (this);
  txAudioCombo_->addItem (tr ("Rear/Data"), TransceiverFactory::TX_audio_source_rear);
  txAudioCombo_->addItem (tr ("Front/Mic"), TransceiverFactory::TX_audio_source_front);
  audioForm->addRow (tr ("TX Audio Source:"), txAudioCombo_);
  main->addWidget (audioBox);

  // ── Buttons ──────────────────────────────────────────────────────────
  auto * buttons = new QDialogButtonBox (
    QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  connect (buttons, &QDialogButtonBox::accepted, this, &RadioProfileDialog::on_accept);
  connect (buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  main->addWidget (buttons);

  loadFromIni ();
}

QString RadioProfileDialog::profileName () const
{
  return nameEdit_->text ().trimmed ();
}

void RadioProfileDialog::populateAudioInputs ()
{
  for (auto const & d : QAudioDeviceInfo::availableDevices (QAudio::AudioInput))
    audioInCombo_->addItem (d.deviceName ());
}

void RadioProfileDialog::populateAudioOutputs ()
{
  for (auto const & d : QAudioDeviceInfo::availableDevices (QAudio::AudioOutput))
    audioOutCombo_->addItem (d.deviceName ());
}

void RadioProfileDialog::loadFromIni ()
{
  QSettings ini {iniPath_, QSettings::IniFormat};
  nameEdit_->setText (ini.value ("Profile/Name", QStringLiteral ("Radio %1").arg (slot_)).toString ());

  QString rig = ini.value ("Rig").toString ();
  if (!rig.isEmpty ()) {
    int i = rigCombo_->findText (rig);
    if (i >= 0) rigCombo_->setCurrentIndex (i);
  }
  {
    QString cp = ini.value ("CATSerialPort").toString ();
    int ci2 = catPortCombo_->findText (cp);
    if (ci2 >= 0) catPortCombo_->setCurrentIndex (ci2);
  }
  int baud = ini.value ("CATSerialRate", 38400).toInt ();
  int bi = baudCombo_->findData (baud);
  if (bi >= 0) baudCombo_->setCurrentIndex (bi);

  int di = dataBitsCombo_->findData (
    ini.value ("CATDataBits", TransceiverFactory::eight_data_bits).toInt ());
  if (di >= 0) dataBitsCombo_->setCurrentIndex (di);
  int si = stopBitsCombo_->findData (
    ini.value ("CATStopBits", TransceiverFactory::one_stop_bit).toInt ());
  if (si >= 0) stopBitsCombo_->setCurrentIndex (si);
  int hi = handshakeCombo_->findData (
    ini.value ("CATHandshake", TransceiverFactory::handshake_default).toInt ());
  if (hi >= 0) handshakeCombo_->setCurrentIndex (hi);
  forceDtrCheck_->setChecked (ini.value ("CATForceDTR").toBool ());
  dtrCombo_->setCurrentIndex (ini.value ("DTR").toBool () ? 0 : 1);
  forceRtsCheck_->setChecked (ini.value ("CATForceRTS").toBool ());
  rtsCombo_->setCurrentIndex (ini.value ("RTS").toBool () ? 0 : 1);
  pollSpin_->setValue (ini.value ("Polling", 0).toInt () & 0x7fff);
  int spi = splitCombo_->findData (
    ini.value ("SplitMode", TransceiverFactory::split_mode_none).toInt ());
  if (spi >= 0) splitCombo_->setCurrentIndex (spi);

  int pi = pttMethodCombo_->findData (
    ini.value ("PTTMethod", TransceiverFactory::PTT_method_VOX).toInt ());
  if (pi >= 0) pttMethodCombo_->setCurrentIndex (pi);
  {
    QString pp = ini.value ("PTTport").toString ();
    int pi2 = pttPortCombo_->findText (pp);
    if (pi2 >= 0) pttPortCombo_->setCurrentIndex (pi2);
  }

  QString ain  = ini.value ("SoundInName").toString ();
  if (!ain.isEmpty ()) {
    int ii = audioInCombo_->findText (ain);
    if (ii >= 0) audioInCombo_->setCurrentIndex (ii);
  }
  audioInChCombo_->setCurrentText (ini.value ("AudioInputChannel", "Mono").toString ());
  QString aout = ini.value ("SoundOutName").toString ();
  if (!aout.isEmpty ()) {
    int oi = audioOutCombo_->findText (aout);
    if (oi >= 0) audioOutCombo_->setCurrentIndex (oi);
  }
  audioOutChCombo_->setCurrentText (ini.value ("AudioOutputChannel", "Mono").toString ());
  int txi = txAudioCombo_->findData (
    ini.value ("TXAudioSource", TransceiverFactory::TX_audio_source_rear).toInt ());
  if (txi >= 0) txAudioCombo_->setCurrentIndex (txi);
}

void RadioProfileDialog::saveToIni ()
{
  QSettings ini {iniPath_, QSettings::IniFormat};
  ini.setValue ("Profile/Name",        nameEdit_->text ().trimmed ());
  ini.setValue ("Profile/Visible",     true);
  ini.setValue ("Rig",                 rigCombo_->currentText ());
  ini.setValue ("CATSerialPort",       catPortCombo_->currentText ());
  ini.setValue ("CATSerialRate",       baudCombo_->currentData ().toInt ());
  ini.setValue ("CATDataBits",         dataBitsCombo_->currentData ().toInt ());
  ini.setValue ("CATStopBits",         stopBitsCombo_->currentData ().toInt ());
  ini.setValue ("CATHandshake",        handshakeCombo_->currentData ().toInt ());
  ini.setValue ("CATForceDTR",         forceDtrCheck_->isChecked ());
  ini.setValue ("DTR",                 dtrCombo_->currentIndex () == 0);
  ini.setValue ("CATForceRTS",         forceRtsCheck_->isChecked ());
  ini.setValue ("RTS",                 rtsCombo_->currentIndex () == 0);
  ini.setValue ("Polling",             pollSpin_->value ());
  ini.setValue ("SplitMode",           splitCombo_->currentData ().toInt ());
  ini.setValue ("PTTMethod",           pttMethodCombo_->currentData ().toInt ());
  ini.setValue ("PTTport",             pttPortCombo_->currentText ());
  ini.setValue ("SoundInName",         audioInCombo_->currentText ());
  ini.setValue ("AudioInputChannel",   audioInChCombo_->currentText ());
  ini.setValue ("SoundOutName",        audioOutCombo_->currentText ());
  ini.setValue ("AudioOutputChannel",  audioOutChCombo_->currentText ());
  ini.setValue ("TXAudioSource",       txAudioCombo_->currentData ().toInt ());
  ini.sync ();
}

void RadioProfileDialog::on_accept ()
{
  if (nameEdit_->text ().trimmed ().isEmpty ()) {
    QMessageBox::warning (this, tr ("Missing name"),
      tr ("Please enter a profile name."));
    return;
  }
  saveToIni ();
  accept ();
}

} // namespace wkjtx
