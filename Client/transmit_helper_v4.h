/*
*   Author: ding.yin
*   Version: 0.3
*   Date: 2021-12-10
*   Details: transmit helper using zmq & protobuf, header-only
*/

#pragma once

#include "protocol.h"
#include "great_proto.v1.pb.h"
#include <iostream>
//#include "zmq.hpp"
#include <sstream>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib,"ws2_32")
using namespace std;

void parseProto2Request(void* data, int size, UserRequest* req) {
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

void convertStream2Proto(Stream* stream, std::string& proto){
  safehd::Stream* s = new safehd::Stream();
  s->set_width(stream->width);
  s->set_height(stream->height);
  s->set_timestamp(stream->timestamp);
  
  switch (stream->quality) {
    case ImgQuality::QUALITY_LOW:
      s->set_quality(safehd::ImgQuality::QUALITY_LOW);
      break;
    case ImgQuality::QUALITY_MID:
      s->set_quality(safehd::ImgQuality::QUALITY_MID);
      break;
    case ImgQuality::QUALITY_HIGH:
      s->set_quality(safehd::ImgQuality::QUALITY_HIGH);
      break;
    case ImgQuality::AUTO:
      s->set_quality(safehd::ImgQuality::AUTO);
      break;
    default:
      break;
  }
  s->set_raw(stream->raw.size(), stream->raw.c_str());
  s->SerializeToString(&proto);
}

void parseProto2Stream(std::string& proto, Stream& stream) {
  safehd::Stream* s = new safehd::Stream();
  s->ParseFromString(proto);
  stream.width = s->width();
  stream.height = s->height();
  stream.timestamp = s->timestamp();
  switch (s->quality()) {
    case safehd::ImgQuality::QUALITY_LOW:
      stream.quality = ImgQuality::QUALITY_LOW;
      break;
    case safehd::ImgQuality::QUALITY_MID:
      stream.quality = ImgQuality::QUALITY_MID;
      break;
    case safehd::ImgQuality::QUALITY_HIGH:
      stream.quality = ImgQuality::QUALITY_HIGH;
      break;
    case safehd::ImgQuality::AUTO:
      stream.quality = ImgQuality::AUTO;
      break;    
    default:
      break;
  }
}

void convertRequest2Proto(UserRequest* req, void* data, int& size) {
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
  //ue->SerializeToString(&proto);
  size = ue->ByteSizeLong();
  ue->SerializeToArray(data, size);
}
 


class clientHelper {
 public:
  void init(std::string port) {
    port_ = port;
    data_ = malloc(256);
    size_ = 0;
  }
  
  void run() {

    run_ = true;
    userRequest_proto_ = "";

    WSADATA stData = { 0 };
    WORD dVer = MAKEWORD(2, 2);
    if (WSAStartup(dVer, &stData))
    {
        cout << "init fail" << endl;
        return ;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        cout << "socket fail" << endl;
        return;
    }

    sockaddr_in stAddr = { 0 };
    stAddr.sin_family = AF_INET;
    stAddr.sin_addr.S_un.S_addr = inet_addr("192.168.3.20");
    stAddr.sin_port = htons(5566);
    if (connect(sock, (sockaddr*)&stAddr, sizeof(sockaddr_in)) == SOCKET_ERROR)
    {
        cout << "connect fail" << endl; 
        return ;
    }

    while (run_) {
      if (pub_ready_) {
          
          pub_ready_ = false;
      }
    }
    //clear socket
    WSACleanup();
    closesocket(sock);
  }
  
  void setRequest(UserRequest* ue) {
    void* data = malloc(256);
    int size = 256;
    convertRequest2Proto(ue, data, size);
    int ret = send(sock, (const char*)data, size, NULL);
    //pub_ready_ = true;
    free(data);
  }

  void stop() {
    run_ = false;
    free(data_);
  }

  static clientHelper& getInstance() {
    static clientHelper s;
    return s;
  }
 private:
  clientHelper(){};
  std::string userRequest_proto_;
  std::string port_;
  bool run_;
  bool pub_ready_;
  void* data_;
  int size_;
  SOCKET sock;
};




