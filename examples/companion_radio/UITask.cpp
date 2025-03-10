#include "UITask.h"
#include <Arduino.h>
#include <helpers/TxtDataHelpers.h>

#define AUTO_OFF_MILLIS   15000   // 15 seconds

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe, 
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc, 
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00, 
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00, 
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0, 
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00, 
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00, 
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8, 
};

void UITask::begin(const char* node_name, const char* build_date, uint32_t pin_code) {
  _prevBtnState = HIGH;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  clearMsgPreview();
  _node_name = node_name;
  _build_date = build_date;
  _pin_code = pin_code;
  _display->turnOn();
}

void UITask::clearMsgPreview() {
  _origin[0] = 0;
  _msg[0] = 0;
}

void UITask::showMsgPreview(uint8_t path_len, const char* from_name, const char* text) {
  if (path_len == 0xFF) {
    sprintf(_origin, "(F) %s", from_name);
  } else {
    sprintf(_origin, "(%d) %s", (uint32_t) path_len, from_name);
  }
  StrHelper::strncpy(_msg, text, sizeof(_msg));

  if (!_display->isOn()) _display->turnOn();
  _auto_off = millis() + AUTO_OFF_MILLIS;  // extend the auto-off timer
}

void UITask::renderCurrScreen() {
  char tmp[80];
  if (_origin[0] && _msg[0]) {
    // render message preview
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->print(_node_name);

    _display->setCursor(0, 12);
    _display->print(_origin);
    _display->setCursor(0, 24);
    _display->print(_msg);

    //_display->setCursor(100, 9);   TODO
    //_display->setTextSize(2);
    //_display->printf("%d", msgs);
  } else {
    // render 'home' screen
    _display->drawXbm(0, 0, meshcore_logo, 128, 13);
    _display->setCursor(0, 20);
    _display->setTextSize(1);
    _display->print(_node_name);

    sprintf(tmp, "Build: %s", _build_date);
    _display->setCursor(0, 32);
    _display->print(tmp);

    if (_connected) {
      //_display->printf("freq : %03.2f sf %d\n", _prefs.freq, _prefs.sf);
      //_display->printf("bw   : %03.2f cr %d\n", _prefs.bw, _prefs.cr);
    } else if (_pin_code != 0) {
      _display->setTextSize(2);
      _display->setCursor(0, 43);
      sprintf(tmp, "Pin:%d", _pin_code);
      _display->print(tmp);
    }
  }
}

void UITask::loop() {
  if (millis() >= _next_read) {
    int btnState = digitalRead(PIN_USER_BTN);
    if (btnState != _prevBtnState) {
      if (btnState == LOW) {  // pressed?
        if (_display->isOn()) {
          clearMsgPreview();
        } else {
          _display->turnOn();
        }
        _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
      }
      _prevBtnState = btnState;
    }
    _next_read = millis() + 100;  // 10 reads per second
  }

  if (_display->isOn()) {
    if (millis() >= _next_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }
}
