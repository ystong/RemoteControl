
#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include "include/transmit_helper_v2.h"
#include <stdio.h>
#include <unistd.h>

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

void parseProto2Request(void* data, int size, UserRequest* req){
  safehd::UserEvent* event = new safehd::UserEvent();
  event->ParseFromArray(data, size);
  req->timestamp = event->timestamp();
  req->run = event->run();
  safehd::KeyEvent key = event->key();
  safehd::MouseEvent mouse = event->mouse();
  req->key.val = key.val();
  req->key.valid = key.valid();
  req->mouse.valid = mouse.valid();
  req->mouse.x = mouse.x();
  req->mouse.y = mouse.y();

  switch (mouse.type()) {
  case safehd::MouseEventType::MOUSE_R_CLICK:
    req->mouse.type = MouseEventType::MOUSE_R_CLICK;
    break;
  case safehd::MouseEventType::MOUSE_M_CLICK:
    req->mouse.type = MouseEventType::MOUSE_M_CLICK;
    break;
  case safehd::MouseEventType::MOUSE_L_CLICK:
    req->mouse.type = MouseEventType::MOUSE_L_CLICK;
    break;
  case safehd::MouseEventType::MOUSE_L_DCLICK:
    req->mouse.type = MouseEventType::MOUSE_L_DCLICK;
    break;
  case safehd::MouseEventType::MOUSE_NONE:
    req->mouse.type = MouseEventType::MOUSE_NONE;
    break;
  default:
    break;
  }

  switch (event->quality()) {
  case safehd::ImgQuality::QUALITY_HIGH:
    req->quality = ImgQuality::QUALITY_HIGH;
    break;
  case safehd::ImgQuality::QUALITY_MID:
    req->quality = ImgQuality::QUALITY_MID;
    break;
  case safehd::ImgQuality::QUALITY_LOW:
    req->quality = ImgQuality::QUALITY_LOW;
    break;
  case safehd::ImgQuality::AUTO:
    req->quality = ImgQuality::AUTO;
    break;
  default:
    break;
  }
}


void convertRequest2Proto(UserRequest* req, std::string& proto) {
  safehd::UserEvent* ue = new safehd::UserEvent();
  ue->set_run(req->run);
  ue->set_timestamp(req->timestamp);
  
  safehd::MouseEvent* me = ue->mutable_mouse();
  me->set_valid(req->mouse.valid);
  me->set_x(req->mouse.x);
  me->set_y(req->mouse.y);

  switch (req->mouse.type) {
    case MouseEventType::MOUSE_L_CLICK:
      me->set_type(safehd::MOUSE_L_CLICK);
      break;
    case MouseEventType::MOUSE_M_CLICK:
      me->set_type(safehd::MOUSE_M_CLICK);
      break;
    case MouseEventType::MOUSE_R_CLICK:
      me->set_type(safehd::MOUSE_L_CLICK);
      break;
    case MouseEventType::MOUSE_L_DCLICK:
      me->set_type(safehd::MOUSE_L_DCLICK);
      break;
    case MouseEventType::MOUSE_NONE:
      me->set_type(safehd::MOUSE_NONE);
      break;
    default:
      break;
  }

  safehd::KeyEvent* ke = ue->mutable_key();
  ke->set_valid(req->key.valid);
  ke->set_val(req->key.val);

  switch (req->quality) {
    case ImgQuality::QUALITY_LOW:
      ue->set_quality(safehd::QUALITY_LOW);
      break;
    case ImgQuality::QUALITY_MID:
      ue->set_quality(safehd::QUALITY_MID);
      break;
    case ImgQuality::QUALITY_HIGH:
      ue->set_quality(safehd::QUALITY_HIGH);
      break;
    case ImgQuality::AUTO:
      ue->set_quality(safehd::AUTO);
      break;      
    default:
      break;
  }
  ue->SerializeToString(&proto);
}


bool moveMouse(float x, float y, float width, float height, UserRequest* ue) {
  char command[64];
  int w = x * width;
  int h = y * height;
//   if (ue->mouse.valid == false) return false;

  if (ue->mouse.type == MOUSE_L_CLICK) {
    sprintf(command, "xdotool click 1");
  } else {
    sprintf(command, "xdotool mousemove %d %d", w, h);
  }
  
  std::cout << command << std::endl;
  if (system(command) < 0) return false;
  return true;
}


bool procUserRequest(UserRequest* ue, bool& changeQuality) {
  char command[64];
  bool rclick = false;
  if (ue->mouse.valid) {  
    int w = ue->mouse.x * SCREEN_WIDTH;
    int h = ue->mouse.y * SCREEN_HEIGHT;
    // std::cout << "mouse type " << ue->mouse.type << std::endl;
    if (ue->mouse.type == MOUSE_L_CLICK) {
      sprintf(command, "xdotool click 1");
    } else if (ue->mouse.type == MOUSE_M_CLICK) {
      sprintf(command, "xdotool click 2");
    } else if (ue->mouse.type == MOUSE_R_CLICK) {
      sprintf(command, "xdotool click 3");
      rclick = true;
    } else {
      sprintf(command, "xdotool mousemove %d %d", w, h);
    }
    // std::cout << command << std::endl;
    if (system(command) < 0) return false;
    return true;    
  } else if (ue->key.valid) {
    if (ue->key.val >= 20 && ue->key.val <= 127) {
      sprintf(command, "xdotool key 0x00%02x", ue->key.val);
    } else {
      sprintf(command, "xdotool key 0xff%02x", ue->key.val);
    }
    if (system(command) < 0) return false;
    return true; 
  } else if (ue->run) {
    changeQuality = true;
  } else {
    return false;
  }
}
