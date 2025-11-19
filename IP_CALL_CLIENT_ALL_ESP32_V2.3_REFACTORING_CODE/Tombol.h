#ifndef TOMBOL_H
#define TOMBOL_H

#include <Arduino.h>

// Tipe fungsi callback
typedef void (*FungsiAksi)();

class Tombol {
  private:
    uint8_t _pinTombol;
    uint8_t _pinBuzzer;
    long _debounceDelay;
    
    int _activeState;
    int _inactiveState;

    int _stateTerakhir;
    unsigned long _waktuTekan;
    bool _sedangDitekan;
    bool _buzzerAktif;

    bool _aksi10sTerpicu;
    bool _aksi30sTerpicu;

    FungsiAksi _fungsiAksiOnClick = nullptr;
    FungsiAksi _fungsiAksiTahan10s = nullptr;
    FungsiAksi _fungsiAksiTahan30s = nullptr;

  public:
    Tombol(uint8_t pinTombol, uint8_t pinBuzzer, int activeState = LOW, long delay = 200);

    void begin();
    void update();

    void onValidClick(FungsiAksi fungsi);
    void onHold10s(FungsiAksi fungsi);
    void onHold30s(FungsiAksi fungsi);
};

#endif
