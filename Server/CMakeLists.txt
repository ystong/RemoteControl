cmake_minimum_required(VERSION 3.0.0)
project(Yun VERSION 0.1.0)

find_package(Protobuf REQUIRED)

set(INC_DIRS ${Protobuf_INCLUDE_DIRS} proto_out/ include/)
set(LD_LIBS  ${Protobuf_LIBRARIES} avcodec avformat  avutil swresample swscale  avdevice )

add_executable(Yun main2.cpp epolls.cpp proto_out/great_proto.v1.pb.cc transmit_helper.v2.cc)
target_include_directories(Yun PUBLIC ${INC_DIRS})
target_link_libraries(Yun PUBLIC ${LD_LIBS})

# target_link_libraries(Yun avcodec)

# target_link_libraries(Yun avformat)

# target_link_libraries(Yun avutil)

# target_link_libraries(Yun swresample)

# target_link_libraries(Yun swscale)

# target_link_libraries(Yun avdevice)
