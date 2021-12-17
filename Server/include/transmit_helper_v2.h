/*
*   Author: ding.yin
*   Version: 0.3
*   Date: 2021-12-10
*   Details: transmit helper using zmq & protobuf, header-only
*/

#ifndef __TRANSMIT
#define __TRANSMIT

#include "protocol.h"
#include "great_proto.v1.pb.h"

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>

void parseProto2Request(void* data, int size, UserRequest* req);
void convertRequest2Proto(UserRequest* req, std::string& proto);
bool moveMouse(float x, float y, float width, float height, UserRequest* ue);
bool procUserRequest(UserRequest* ue, bool& changeQuality);

#endif // __TRANSMIT