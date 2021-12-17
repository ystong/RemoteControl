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

修改为服务端的IP

nginx.conf配置
