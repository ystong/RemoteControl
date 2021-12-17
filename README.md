# RemoteControl
Linux-Linux or Linux-windows

服务端需要安装：
ffmpeg 4.2.4
nginx

服务端需要将/include/epoll.h中
#define IPADDRESS "192.168.3.20"
修改为自己的IP

客户端需要安装
ffmpeg 4.2.4
opencv

客户端需要将client6.cpp中
const char* path = "rtmp://192.168.3.20:1935/wstv/home";
以及transmit_helper_v4.h中
stAddr.sin_addr.S_un.S_addr = inet_addr("192.168.3.20");
修改为服务端的IP

nginx.conf配置
