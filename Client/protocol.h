/*
*   Author: ding.yin
*   Version: 0.3
*   Date: 2021-12-03
*   Details: 为协议传输临时制定的通讯协议
*/

#pragma once

#include <string>

// image quality
enum ImgQuality {
  QUALITY_LOW = 1,
  QUALITY_MID,
  QUALITY_HIGH,
  AUTO
};

// mouse click type
enum MouseEventType {
  MOUSE_R_CLICK = 1,
  MOUSE_L_CLICK,
  MOUSE_M_CLICK,
  MOUSE_L_DCLICK,
  MOUSE_NONE  
};

// mouse click event 
struct MouseEvent {
  bool valid;
  MouseEventType type;
  // position of the arrow
  float x; // 0.0 ~ 100.0, cols = WIDTH * x
  float y; // 0.0 ~ 100.0, rows = HEIGHT * y
  MouseEvent(): valid(false), type(MOUSE_NONE), x(50.0), y(50.0) {}
};

// keyboard event struct
struct KeyEvent {
  bool valid;
  unsigned int val; // 0 ~ 255
  KeyEvent(): valid(false), val(0) {}
};

struct UserRequest {
  MouseEvent mouse;
  KeyEvent key;
  ImgQuality quality;
  long timestamp;
  bool run; // stop or start
  UserRequest(): timestamp(0), quality(AUTO), run(false) {}
};


struct Stream {
  unsigned int height;
  unsigned int width;
  ImgQuality quality;
  long timestamp;
  std::string raw;
};

