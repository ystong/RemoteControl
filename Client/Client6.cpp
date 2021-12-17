/*此版本无zmq*/
#include <fstream>  
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "opencv2/highgui/highgui.hpp"  
#include "opencv2/imgproc/imgproc.hpp"  
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <thread>
//#include "sys/time.h"
//#include <unistd.h>
//#include "zmq.hpp"
#include "transmit_helper_v4.h"


extern "C"
{
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<libavutil/avutil.h>
}

using namespace std;
using namespace cv;

#define MAX_DELAY_SECOND 0.1

const char* path = "rtmp://192.168.3.20:1935/wstv/home";
bool haveStart = false;
bool userEnd = false;
Mat control;



void  YUVToRGB(unsigned char* yuv, unsigned char* rgb, int width, int height)
{
    unsigned char* y = yuv;
    unsigned char* u = yuv + height * width;
    unsigned char* v = u + height * width / 4;

    unsigned char* bgr = nullptr;
    unsigned char bR = 0;
    unsigned char bG = 0;
    unsigned char bB = 0;
    unsigned char bY = 0;
    unsigned char bU = 0;
    unsigned char bV = 0;
    double temp = 0;
    for (int i = 0;i < height;++i)
    {
        for (int j = 0;j < width;++j)
        {
            bgr = rgb + i * width * 3 + j * 3;

            bY = *(y + i * width + j);
            bU = *u;
            bV = *v;

            temp = bY + 1.773 * (bU - 128);
            bB = temp < 0 ? 0 : (temp > 255 ? 255 : (unsigned char)temp);

            temp = (bY - (0.344) * (bU - 128) - (0.714) * (bV - 128));
            bG = temp < 0 ? 0 : (temp > 255 ? 255 : (unsigned char)temp);

            temp = (bY + (1.403) * (bV - 128));
            bR = temp < 0 ? 0 : (temp > 255 ? 255 : (unsigned char)temp);

            *bgr = bB;
            *(bgr + 1) = bG;
            *(bgr + 2) = bR;

            if (j % 2)
            {
                ++u;
                ++v;
            }
        }
        if (i % 2 == 0)
        {
            u = u - width / 2;
            v = v - width / 2;
        }
    }
}

void on_mouse_remote(int event, int x, int y, int flags, void* userdata)//event鼠标事件代号，x,y鼠标坐标，flags拖拽和键盘操作的代号
{
    UserRequest* ue_mouse = new UserRequest;
    if (event == CV_EVENT_LBUTTONDOWN)//左键按下
    {
        ue_mouse->mouse.valid = true;
        ue_mouse->mouse.type = MOUSE_L_CLICK;
    }
    else if (event == CV_EVENT_RBUTTONDOWN)//右键按下
    {
        ue_mouse->mouse.valid = true;
        ue_mouse->mouse.type = MOUSE_R_CLICK;
    }
    else if (event == CV_EVENT_MBUTTONDOWN)//中键按下
    {
        ue_mouse->mouse.valid = true;
        ue_mouse->mouse.type = MOUSE_M_CLICK;
    }
    else if (event == CV_EVENT_LBUTTONDBLCLK)//左键双击
    {
        ue_mouse->mouse.valid = true;
        ue_mouse->mouse.type = MOUSE_L_DCLICK;
    }else {
        ue_mouse->mouse.valid = true;
        ue_mouse->mouse.type = MOUSE_NONE;
    }

    ue_mouse->mouse.x = float(x * 1.0 / 1920.0);
    ue_mouse->mouse.y = float(y * 1.0 / 1080.0);
    clientHelper::getInstance().setRequest(ue_mouse);
    //printf("x y %f %f\n", ue_mouse->mouse.x, ue_mouse->mouse.y);
    delete ue_mouse;

   
}

void PlayFromPath(const char* path)
{
    AVFormatContext* pFormatCtx = nullptr;
    pFormatCtx = avformat_alloc_context();

    AVDictionary* Dict = nullptr;
    av_dict_set(&Dict,"probesize","20480",0);
    av_dict_set(&Dict, "fflags", "nobuffer", 0);
    //av_dict_set(&Dict, "stimeout", "20000", 0);
    //av_dict_set(&Dict, "rtmp_transport", "tcp", 0);
    av_dict_set(&Dict, "max_delay", "500", 0);
    av_dict_set(&Dict, "max_analyze_duration", "100000", 0);

    if (avformat_open_input(&pFormatCtx, path, nullptr, &Dict) != 0)
    {
        printf("open path failed\n");
        return;
    }

    avformat_find_stream_info(pFormatCtx, &Dict);

    int videoStreamInd = av_find_best_stream(pFormatCtx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    int streamCount = pFormatCtx->nb_streams;

    AVCodecParameters* codecpar = nullptr;
    codecpar = avcodec_parameters_alloc();

    AVStream* stream = pFormatCtx->streams[videoStreamInd];
    avcodec_parameters_copy(codecpar, stream->codecpar);

    AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);

    AVCodecContext* codecCtx = nullptr;
    codecCtx = avcodec_alloc_context3(codec);

    avcodec_parameters_to_context(codecCtx, codecpar);

    codecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;

    if (avcodec_open2(codecCtx, codec, nullptr) != 0)
    {
        printf("avcodec_open2 failed\n");
        return;
    }

    cvNamedWindow("Remote Desktop");

    printf("Player Init Successful\n");
    int64_t pts00;
    bool FirstFrame = false;
    bool isAccelarate = false;
    time_t start;

    while (1)
    {
        if (userEnd)
        {
            userEnd = false;
            if (pFormatCtx != nullptr)
            {
                avformat_free_context(pFormatCtx);
                pFormatCtx = nullptr;
            }
            if (codecpar != nullptr)
            {
                avcodec_parameters_free(&codecpar);
                codecpar = nullptr;
            }
            if (codecCtx != nullptr)
            {
                avcodec_free_context(&codecCtx);
                codecCtx = nullptr;
            }
            cvDestroyWindow("Remote Desktop");
            return;
        }

        AVPacket* pkt = nullptr;
        pkt = av_packet_alloc();

        av_read_frame(pFormatCtx, pkt);

        //丢帧策略
        if (!FirstFrame)
        {
            FirstFrame = true;
            start = clock();
            pts00 = pkt->pts;
        }
        /*if (isAccelarate)
        {
            if (!(pkt->flags & AV_PKT_FLAG_KEY)) {
                if (pkt != nullptr)
                {
                    av_packet_free(&pkt);
                    pkt = nullptr;
                }
                continue;
            }
            else isAccelarate = false;
        }
        */
        else
        {
            double st = (pkt->pts-pts00) * av_q2d(stream->time_base);
            time_t end = clock();
            double nt = double(end - start) / CLOCKS_PER_SEC;
            if (nt - st > MAX_DELAY_SECOND && !(pkt->flags & AV_PKT_FLAG_KEY))
            {
                if (pkt != nullptr)
                {
                    av_packet_free(&pkt);
                    pkt = nullptr;
                }
                continue;
                isAccelarate = true;
            }
        }
        


        if (pkt->stream_index != videoStreamInd)
        {
            if (pkt != nullptr)
            {
                av_packet_free(&pkt);
                pkt = nullptr;
            }
            continue;
        }

        if (avcodec_send_packet(codecCtx, pkt) != 0)
        {
            if (pkt != nullptr)
            {
                av_packet_free(&pkt);
                pkt = nullptr;
            }
            continue;
        }

        while (1)
        {
            AVFrame* frame = nullptr;
            frame = av_frame_alloc();
            if (avcodec_receive_frame(codecCtx, frame) != 0)
            {
                if (frame != nullptr)
                {
                    av_frame_free(&frame);
                    frame = nullptr;
                }
                break;
            }
            
            int width = frame->width, height = frame->height;
            int y_size = frame->width * frame->height;

            unsigned char* yuv = (unsigned char*)malloc(y_size * 3 / 2);


            for (int i = 0;i < height;++i)
            {
                memcpy(yuv + i * width, frame->data[0] + i * frame->linesize[0], width);
            }
            width /= 2;
            height /= 2;
            for (int i = 0;i < height;++i)
            {
                memcpy(yuv + y_size + i * width, frame->data[1] + i * frame->linesize[1], width);
            }
            for (int i = 0;i < height;++i)
            {
                memcpy(yuv + y_size * 5 / 4 + i * width, frame->data[2] + i * frame->linesize[2], width);
            }

            width = frame->width;
            height = frame->height;
            //show

            unsigned char* rgb = (unsigned char*)malloc(width * height * 3);

            IplImage* image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);


            YUVToRGB(yuv, rgb, width, height);
            cvSetData(image, rgb, width * 3);


            cvShowImage("Remote Desktop", image);
            int q = waitKey(1);
            if (q == -1) {
                //
            } else {
                UserRequest* ue_key = new UserRequest;
                ue_key->key.valid = true;
                ue_key->key.val = q;
                clientHelper::getInstance().setRequest(ue_key);
                cout << "UE key " << ue_key->key.val << endl;
                delete ue_key;
            }
            cv::setMouseCallback("Remote Desktop", on_mouse_remote);

            

            free(rgb);
            free(yuv);
            if (frame != nullptr)
            {
                av_frame_free(&frame);
                frame = nullptr;
            }
        }
        if (pkt != nullptr)
        {
            av_packet_free(&pkt);
            pkt = nullptr;
        }

    }

    avcodec_send_packet(codecCtx, nullptr);

    while (1)
    {
        AVFrame* frame = nullptr;
        frame = av_frame_alloc();
        if (avcodec_receive_frame(codecCtx, frame) != 0)
        {
            if (frame!= nullptr)
            {
                av_frame_free(&frame);
                frame = nullptr;
            }
            break;
        }
        int width = frame->width, height = frame->height;
        int y_size = frame->width * frame->height;

        unsigned char* yuv = (unsigned char*)malloc(y_size);

        for (int i = 0;i < height;++i)
        {
            memcpy(yuv + i * width, frame->data[0] + i * frame->linesize[0], width);
        }
        width /= 2;
        height /= 2;
        for (int i = 0;i < height;++i)
        {
            memcpy(yuv + y_size + i * width, frame->data[1] + i * frame->linesize[1], width);
        }
        for (int i = 0;i < height;++i)
        {
            memcpy(yuv + y_size * 5 / 4 + i * width, frame->data[2] + i * frame->linesize[2], width);
        }

        width = frame->width;
        height = frame->height;
        //show

        unsigned char* rgb = (unsigned char*)malloc(width * height * 2);

        IplImage* image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 3);

        YUVToRGB(yuv, rgb, width, height);
        cvSetData(image, rgb, width * 3);

        cvShowImage("Remote Desktop", image);

        waitKey(1);

        free(rgb);
        free(yuv);
        if (frame != nullptr)
        {
            av_frame_free(&frame);
            frame = nullptr;
        }
    }


    if (pFormatCtx != nullptr)
    {
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }
    if (codecpar != nullptr)
    {
        avcodec_parameters_free(&codecpar);
        codecpar = nullptr;
    }
    if (codecCtx != nullptr)
    {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    cvDestroyAllWindows();
}

void on_mouse_control(int event, int x, int y, int flags, void* userdata)//event鼠标事件代号，x,y鼠标坐标，flags拖拽和键盘操作的代号
{
    UserRequest* ue = (UserRequest*)userdata;
    if (event == CV_EVENT_LBUTTONDOWN)//左键按下
    {
        if (x > 127 && x < 261 && y>132 && y < 188 && haveStart == false) {
            printf("Remote control Start\n");
            haveStart = true;
            control = imread("control_start.png");
            imshow("control", control);
            waitKey(1);
            PlayFromPath(path);
        }

        if (x > 127 && x < 261 && y>243 && y < 300 && haveStart) {
            printf("Remote control End\n");
            haveStart = false;
            userEnd = true;
            control = imread("control_stop.png");
            imshow("control", control);
            waitKey(1);
        }

        if (x > 435 && x < 620 && y>130 && y <167 && haveStart && ue->quality != QUALITY_HIGH) {
            printf("High Quality\n");
            control = imread("control_high.png");
            imshow("control", control);
            waitKey(1);
            ue->quality = QUALITY_HIGH; 
            clientHelper::getInstance().setRequest(ue);
        }
        if (x > 435 && x < 620 && y>198 && y < 237 && haveStart && ue->quality != QUALITY_MID) {
            printf("Medium Quality\n");
            control = imread("control_medium.png");
            imshow("control", control);
            waitKey(1);
            ue->quality = QUALITY_MID;
            clientHelper::getInstance().setRequest(ue);
        }
        if (x > 435 && x < 620 && y>270 && y < 308 && haveStart && ue->quality != QUALITY_LOW) {
            printf("Low Quality\n");
            control = imread("control_low.png");
            imshow("control", control);
            waitKey(1);
            ue->quality = QUALITY_LOW;
            clientHelper::getInstance().setRequest(ue);

        }
    }
}


int main(int, char**) {

    clientHelper::getInstance();
    std::thread t(&clientHelper::run, &clientHelper::getInstance());
    t.detach();

    control= imread("welcome.png");
	imshow("control", control);
	waitKey(2000);
	control = imread("control.png");
    UserRequest* ue_quality = new UserRequest;
	cv::setMouseCallback("control", on_mouse_control, ue_quality);
    imshow("control", control);
    
	while (1) {
        waitKey(1);

	}
    return 0;
}
