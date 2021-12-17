#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>  
#include <libswscale/swscale.h>  
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
}
#include <thread>

#include "include/transmit_helper_v2.h"
#include "epolls.h"

# define WIDTH 1920
# define HEIGHT 1080

void* screen_shot(void* arg)
{
    const char* TargetURL="rtmp://localhost:1935/wstv/home";
    AVFormatContext* pFormatCtx;
    AVDictionary* options=nullptr;
    AVStream* videoStream=nullptr;
    AVCodecContext *pCodecCtx=nullptr;
    AVCodec* pCodec=nullptr;
    AVFrame* pFrame=nullptr;
    AVFrame* m_pYUVFrame=nullptr;
    AVPacket* raw_pkt;

    SwsContext* img_convert_ctx;

    int videoindex=-1;
    int width=WIDTH;
    int height=HEIGHT;
    int decode_flag=0;
    int got_picture=-1;

    avdevice_register_all();
    av_dict_set(&options,"framerate","25",0);
    av_dict_set(&options,"follow_mouse","centered",0);
    av_dict_set(&options,"video_size","1920x1080",0);

    pFormatCtx=avformat_alloc_context();
    AVInputFormat* ifmt=av_find_input_format("x11grab");

    if(ifmt==nullptr)
    {
        printf("av_find_input_format not found\n");
        return nullptr;
    }

    if(avformat_open_input(&pFormatCtx,":0.0",ifmt,&options)!=0)
    {
        printf("Counldn't open input stream\n");
        return nullptr;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)   // 
    {
        printf("Couldn't find stream information\n");
        return nullptr;
    }

    for (int i=0; i<pFormatCtx->nb_streams; ++i){
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }
    }

    if (videoindex == -1)
    {
        printf("Didn't find a video stream\n");
        return nullptr;
    }
    videoStream = pFormatCtx->streams[videoindex];

    pCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);

    if (pCodec == nullptr)
    {
        printf("Codec not found\n");
        return nullptr;
    }

    pCodecCtx =  avcodec_alloc_context3(pCodec);
    if(!pCodecCtx)
    {
      printf("decoder codec context not found\n");
      return nullptr;
    }

    avcodec_parameters_to_context(pCodecCtx, videoStream->codecpar);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) 
    {
        printf("Could not open codec\n");
        return (void *)-1;
    }

    m_pYUVFrame = av_frame_alloc();  
    pFrame = av_frame_alloc();

    unsigned char *yuv_buf = (unsigned char*) av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,pCodecCtx->height, 1));
            
    av_image_fill_arrays(m_pYUVFrame->data, m_pYUVFrame->linesize, yuv_buf, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
    
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,SWS_BICUBIC, NULL, NULL, NULL);


    //Flv encoder

    AVCodec* FLVcodec=avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext* FLVCodecCtx=avcodec_alloc_context3(FLVcodec);

    FLVCodecCtx->codec_id=FLVcodec->id;
    FLVCodecCtx->thread_count=8;

    AVDictionary *FLVparam=0;

    FLVCodecCtx->flags|=AV_CODEC_FLAG_GLOBAL_HEADER | AV_CODEC_FLAG_LOW_DELAY;

    //av_dict_set(&FLVparam,"preset","superfast",0);
    av_dict_set(&FLVparam,"tune","zerolatency",0);
    strcpy(crf,"18");
    av_dict_set(&FLVparam,"crf","18",0);

    FLVCodecCtx->width=WIDTH;
    FLVCodecCtx->height=HEIGHT;

    //FLVCodecCtx->rc_min_rate=50*1024*1;
    //FLVCodecCtx->rc_max_rate=50*1024*8;
    FLVCodecCtx->bit_rate=50*1024*8;
    FLVCodecCtx->time_base={1,25};
    FLVCodecCtx->framerate={25,1};
    FLVCodecCtx->qmin=10;
    FLVCodecCtx->qmax=40;

    FLVCodecCtx->gop_size=2;
    FLVCodecCtx->max_b_frames=0;
    FLVCodecCtx->pix_fmt=AV_PIX_FMT_YUV420P;

    if(avcodec_open2(FLVCodecCtx,FLVcodec,&FLVparam)!=0)
    {
        printf("open flv encoder false\n");
        return nullptr;
    }

    AVFormatContext* FLVpFormatCtx=nullptr;

    if(avformat_alloc_output_context2(&FLVpFormatCtx,0,"flv",NULL)!=0)
    {
        printf("avformat_alloc_output_context2 false\n");
        return nullptr;
    }

    AVStream* FLVStream=avformat_new_stream(FLVpFormatCtx,NULL);
    FLVStream->codec->codec_tag=0;

    avcodec_parameters_from_context(FLVStream->codecpar,FLVCodecCtx);

    FLVStream->time_base={1,25};

    av_dump_format(FLVpFormatCtx,0,TargetURL,1);

    if(avio_open(&FLVpFormatCtx->pb,TargetURL,AVIO_FLAG_WRITE)<0)
    {
        printf("avio_open\n");
        return nullptr;
    }


    if(avformat_write_header(FLVpFormatCtx,NULL)!=0)
    {
        printf("Write Header failed\n");
        return nullptr;
    }

    AVPacket FLVpkt;
    memset(&FLVpkt,0,sizeof(AVPacket));
    av_init_packet(&FLVpkt);
    int vpts=0;
    int framecnt=0;



    while (1)
    {
        raw_pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
        if (av_read_frame(pFormatCtx, raw_pkt) >= 0) 
        {
            // cout<<"read"<<endl;
            if (raw_pkt->stream_index == videoindex)   
            {
                
                int ret1 = avcodec_send_packet(pCodecCtx, raw_pkt);
                if (ret1 < 0)
                {
                    printf("Decode Error.\n");
                    return nullptr;
                }
                int ret2 = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret2 < 0)
                {
                    printf("Decode Error.\n");
                    return nullptr;

                }

                if (ret2 >= 0){
            
                    sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, m_pYUVFrame->data, m_pYUVFrame->linesize);
                    
                    //dosomething
                    m_pYUVFrame->pts=vpts;
                    vpts++;

                    int got_packet=0;
                    av_init_packet(&FLVpkt);
                    if(FLVCodecCtx->codec_type==AVMEDIA_TYPE_VIDEO)
                    {
                        av_opt_set(FLVCodecCtx->priv_data,"crf",(const char*)crf,0);
                        avcodec_encode_video2(FLVCodecCtx,&FLVpkt,m_pYUVFrame,&got_packet);
                    }

                    FLVpkt.pts=av_rescale_q(FLVpkt.pts,FLVCodecCtx->time_base,FLVStream->time_base);
                    FLVpkt.dts=FLVpkt.pts;
                    FLVpkt.duration=av_rescale_q(FLVpkt.duration,FLVCodecCtx->time_base,FLVStream->time_base);

                    av_write_frame(FLVpFormatCtx,&FLVpkt);
                    av_free_packet(&FLVpkt);  

                }

            }

        } 
        av_packet_unref(raw_pkt);
        av_free(raw_pkt);
    }


    if(pFormatCtx!=nullptr)
    {
        avformat_free_context(pFormatCtx);
        pFormatCtx=nullptr;
    }
    if(pCodecCtx!=nullptr)
    {
        avcodec_free_context(&pCodecCtx);
        pCodecCtx=nullptr;
    }
    if(m_pYUVFrame!=nullptr)
    {
        av_frame_free(&m_pYUVFrame);
        m_pYUVFrame=nullptr;
    }
    if(pFrame!=nullptr)
    {
        av_frame_free(&pFrame);
        pFrame=nullptr;
    }
    free(yuv_buf);
    if(FLVCodecCtx!=nullptr)
    {
        avcodec_free_context(&FLVCodecCtx);
        FLVCodecCtx=nullptr;
    }

}

void* screes(void* args)
{
    screen_shot(nullptr);
}


int main(int, char**) {
  pthread_t t;
  pthread_create(&t,0,screes,nullptr);
  
  // 接收命令并执行
  int listenfd;
  listenfd = socket_bind(IPADDRESS,PORT);
  listen(listenfd,LISTENQ);
  do_epoll(listenfd);

  pthread_join(t, NULL);
  //screen_shot(nullptr);

    return 0;
}