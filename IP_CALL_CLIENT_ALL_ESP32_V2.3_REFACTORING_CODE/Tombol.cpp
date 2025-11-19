#include "Tombol.h"

Tombol::Tombol(uint8_t pinTombol, uint8_t pinBuzzer, int activeState, long delay) {
  _pinTombol = pinTombol;
  _pinBuzzer = pinBuzzer;
  _debounceDelay = delay;
  _activeState = activeState;
  _inactiveState = (_activeState == HIGH) ? LOW : HIGH;
  _stateTerakhir = _inactiveState;
}

void Tombol::begin() {
  if (_activeState == LOW) pinMode(_pinTombol, INPUT_PULLUP);
  else pinMode(_pinTombol, INPUT);
  pinMode(_pinBuzzer, OUTPUT);
  digitalWrite(_pinBuzzer, HIGH);
}

void Tombol::onValidClick(FungsiAksi fungsi) { _fungsiAksiOnClick = fungsi; }
void Tombol::onHold10s(FungsiAksi fungsi) { _fungsiAksiTahan10s = fungsi; }
void Tombol::onHold30s(FungsiAksi fungsi) { _fungsiAksiTahan30s = fungsi; }

void Tombol::update() {
  int stateSekarang = digitalRead(_pinTombol);

  if (_stateTerakhir == _inactiveState && stateSekarang == _activeState) {
    _sedangDitekan = true;
    _waktuTekan = millis();
    _buzzerAktif = false;
    _aksi10sTerpicu = false;
    _aksi30sTerpicu = false;
  }

  if (_sedangDitekan && stateSekarang == _activeState) {
    unsigned long durasiTahan = millis() - _waktuTekan;

    if (!_buzzerAktif && durasiTahan >= _debounceDelay) {
      digitalWrite(_pinBuzzer, LOW);
      _buzzerAktif = true;
    }

    if (!_aksi10sTerpicu && durasiTahan >= 7000) {
      if (_fungsiAksiTahan10s != nullptr) _fungsiAksiTahan10s();
      _aksi10sTerpicu = true;
    }

    if (!_aksi30sTerpicu && durasiTahan >= 15000) {
      if (_fungsiAksiTahan30s != nullptr) _fungsiAksiTahan30s();
      _aksi30sTerpicu = true;
    }
  }

  if (_stateTerakhir == _activeState && stateSekarang == _inactiveState) {
    if (_sedangDitekan) {
      long durasiTekan = millis() - _waktuTekan;
      if (!_aksi10sTerpicu && !_aksi30sTerpicu) {
        if (durasiTekan >= _debounceDelay && _fungsiAksiOnClick != nullptr) {
          _fungsiAksiOnClick();
        }
      }
      _sedangDitekan = false;
      digitalWrite(_pinBuzzer, HIGH);
    }
  }

  _stateTerakhir = stateSekarang;
}
