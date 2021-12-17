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

nginx.conf配置：

user www-data;
worker_processes auto;
pid /run/nginx.pid;
include /etc/nginx/modules-enabled/*.conf;

events {
        worker_connections 768;
        # multi_accept on;
}

http {

        ##
        # Basic Settings
        ##

        sendfile on;
        tcp_nopush on;
        tcp_nodelay on;
        keepalive_timeout 65;
        types_hash_max_size 2048;
        # server_tokens off;

        # server_names_hash_bucket_size 64;
        # server_name_in_redirect off;

        include /etc/nginx/mime.types;
        default_type application/octet-stream;

        ##
        # SSL Settings
        ##

        ssl_protocols TLSv1 TLSv1.1 TLSv1.2 TLSv1.3; # Dropping SSLv3, ref: POODLE
        ssl_prefer_server_ciphers on;

        ##
        # Logging Settings
        ##

        access_log /var/log/nginx/access.log;
        error_log /var/log/nginx/error.log;

        ##
        # Gzip Settings
        ##

        gzip on;

        # gzip_vary on;
        # gzip_proxied any;
        # gzip_comp_level 6;
        # gzip_buffers 16 8k;
        # gzip_http_version 1.1;
        # gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;

        ##
        # Virtual Host Configs
        ##

        include /etc/nginx/conf.d/*.conf;
        include /etc/nginx/sites-enabled/*;
}


#mail {
#        # See sample authentication script at:
#        # http://wiki.nginx.org/ImapAuthenticateWithApachePhpScript
# 
#        # auth_http localhost/auth.php;
#        # pop3_capabilities "TOP" "USER";
#        # imap_capabilities "IMAP4rev1" "UIDPLUS";
# 
#        server {
#                listen     localhost:110;
#                protocol   pop3;
#                proxy      on;
#        }
# 
#        server {
#                listen     localhost:143;
#                protocol   imap;
#                proxy      on;
#        }
#}

rtmp{
        server{
                listen 1935;
                
                chunk_size 4000;
                
                application wstv
                {
                        live on;
                        
                        record all;
                        record_path /tmp/av;
                        record_max_size 1k;
                        
                        record_unique on;
                        
                        allow play all;
                }
        }
}

