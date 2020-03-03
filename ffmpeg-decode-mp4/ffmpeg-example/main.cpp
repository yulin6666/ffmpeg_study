//
//  main.cpp
//  ffmpeg-example
//
//  Created by yulin9 on 2020/3/1.
//  Copyright © 2020 yulin9. All rights reserved.
//

#include <iostream>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

int main(int argc, const char * argv[]) {

//初始化
    av_register_all();//注册所有组件
    avformat_network_init();//初始化网络

//打开文件
    AVFormatContext *pFormatContext;
    pFormatContext = avformat_alloc_context();//初始化一个AVFormatContext
    char filepath[]="/Users/yulin9/Documents/project/mac/ffmpeg_study/ffmpeg-decode-mp4/ffmpeg-example/1.mp4";
    if(avformat_open_input(&pFormatContext,filepath,NULL,NULL)!=0){//打开输入的视频文件
        printf("Couldn't open input stream.\n");
        return -1;
    }
    
    // 第三步：查找视频流
    // 如果是视频解码，那么查找视频流，如果是音频解码，那么就查找音频流
    // avformat_find_stream_info();
    // AVProgram 视频的相关信息
   int ret = avformat_find_stream_info(pFormatContext, NULL);
    if(ret < 0)
    {
        std::cout << "find video stream failed "<<std::endl;
    }
    /*
     * 1.查找视频流索引位置
     *
    */
    int streamIndex =0,i;
    for(i=0; i< streamIndex; i++)
    {
        // 判断流的类型
        // 旧的接口 formatContext->streams[i]->codec->codec_type
        // 4.0.0以后新加入的类型用于替代codec
        // codec -> codecpar
        enum AVMediaType mediaType = pFormatContext->streams[i]->codecpar->codec_type;
        if(mediaType == AVMEDIA_TYPE_VIDEO)  //视频流
        {
            streamIndex =i;
            break;
        }
        else if(mediaType == AVMEDIA_TYPE_AUDIO)
        {
            //音频流
        }
        else
        {
            //其他流
        }
    }
    

//查找视频解码器
    /*
        * 2.根据视频流索引，获取解码器上下文
        * 旧的接口，拿到上下文，pFormatContext->streams[i]->codec
        * 4.0.0以后新加入的类型代替codec
        * codec-codecpar 此处的新接口不需要上下文
       */
AVCodecParameters * avcodecParameters = pFormatContext->streams[streamIndex]->codecpar;
enum AVCodecID codecId = avcodecParameters->codec_id;

/*
* 3.根据解码器上下文，获得解码器ID，然后查找解码器
* avvodec_find_encoder(enum AVCodecId id) 编码器
*/
AVCodec * codec = avcodec_find_decoder(codecId);

//打开解码器
    /*
      * 第五步：打开解码器
      * avcodec_open2()
      * 旧接口直接使用codec作为上下文传入
      * pFormatContext->streams[avformat_stream_index]->codec被遗弃
      * 新接口如下
     */
AVCodecContext * avCodecContext = avcodec_alloc_context3(NULL);
if(avCodecContext == NULL)
{
    //创建解码器上下文失败
    std::cout<<"create condectext failed "<<std::endl;
    return -1;
}
// avcodec_parameters_to_context(AVCodecContext *codec, const AVCodecParameters *par)
// 将新的API中的 codecpar 转成 AVCodecContext
avcodec_parameters_to_context(avCodecContext, avcodecParameters);
ret = avcodec_open2(avCodecContext,codec,NULL);
if(ret < 0)
{
    std::cout << "open decoder failed "<< std::endl;
    return -1;
}
std::cout << "decodec name: "<< codec->name<< std::endl;

    /*
     * 第六步：读取视频压缩数据->循环读取
     * av_read_frame(AVFoematContext *s, AVPacket *packet)
     * s: 封装格式上下文
     * packet:一帧的压缩数据
    */
    AVPacket *avPacket = (AVPacket *)av_mallocz(sizeof(AVPacket));
    AVFrame *avFrameIn = av_frame_alloc();  //用于存放解码之后的像素数据
    // sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat, int dstW, int dstH, enum AVPixelFormat dstFormat, int flags, SwsFilter *srcFilter, SwsFilter *dstFilter, const double *param)
    // 原始数据
    // scrW: 原始格式宽度
    // scrH: 原始格式高度
    // scrFormat: 原始数据格式
    // 目标数据
    // dstW: 目标格式宽度
    // dstH: 目标格式高度
    // dstFormat: 目标数据格式
    // 当遇到Assertion desc failed at src/libswscale/swscale_internal.h:668
    // 这个问题就是获取元数据的高度有问题
    struct SwsContext *swsContext = sws_getContext(avcodecParameters->width,
                                                   avcodecParameters->height,
                                                   avCodecContext->pix_fmt,
                                                   avcodecParameters->width,
                                                   avcodecParameters->height,
                                                   AV_PIX_FMT_YUV420P,
                                                   SWS_BITEXACT, NULL, NULL, NULL);
    //创建缓冲区
    //创建一个YUV420视频像素数据格式缓冲区（一帧数据）
    AVFrame *pAVFrameYUV420P = av_frame_alloc();
    //给缓冲区设置类型->yuv420类型
     //得到YUV420P缓冲区大小
     // av_image_get_buffer_size(enum AVPixelFormat pix_fmt, int width, int height, int align)
     //pix_fmt: 视频像素数据格式类型->YUV420P格式
     //width: 一帧视频像素数据宽 = 视频宽
     //height: 一帧视频像素数据高 = 视频高
     //align: 字节对齐方式->默认是1
     int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,avCodecContext->width,avCodecContext->height,1);
     std::cout << "size:"<<bufferSize<<std::endl;
     //开辟一块内存空间
     uint8_t *outBuffer = (uint8_t *)av_malloc(bufferSize);
     //向pAVFrameYUV420P->填充数据
     // av_image_fill_arrays(uint8_t **dst_data, int *dst_linesize, const uint8_t *src, enum AVPixelFormat pix_fmt, int width, int height, int align)
     //dst_data: 目标->填充数据(pAVFrameYUV420P)
     //dst_linesize: 目标->每一行大小
     //src: 原始数据
     //pix_fmt: 目标->格式类型
     //width: 宽
     //height: 高
     //align: 字节对齐方式
     av_image_fill_arrays(pAVFrameYUV420P->data,
                          pAVFrameYUV420P->linesize,
                          outBuffer,
                          AV_PIX_FMT_YUV420P,
                          avCodecContext->width,
                          avCodecContext->height,
                          1);
     FILE * fileYUV420P = fopen("/Users/yulin9/out.yuv","wb+");
     int currentIndex=0,ySize,uSize,vSize;

     while(av_read_frame(pFormatContext,avPacket) >=0)
     {
        //判断是不是视频
         if(avPacket->stream_index == streamIndex)
         {
             //读取每一帧数据，立马解码一帧数据
             //解码之后得到视频的像素数据->YUV
             // avcodec_send_packet(AVCodecContext *avctx, AVPacket *pkt)
             // avctx: 解码器上下文
             // pkt: 获取到数据包
             // 获取一帧数据
             avcodec_send_packet(avCodecContext,avPacket);

             //解码
             ret = avcodec_receive_frame(avCodecContext,avFrameIn);
             if(ret == 0)  //解码成功
             {
                //此处无法保证视频的像素格式一定是YUV格式
                 //将解码出来的这一帧数据，统一转类型为YUV
                 // sws_scale(struct SwsContext *c, const uint8_t *const *srcSlice, const int *srcStride, int srcSliceY, int srcSliceH, uint8_t *const *dst, const int *dstStride)
                  // SwsContext *c: 视频像素格式的上下文
                  // srcSlice: 原始视频输入数据
                  // srcStride: 原数据每一行的大小
                  // srcSliceY: 输入画面的开始位置，一般从0开始
                  // srcSliceH: 原始数据的长度
                  // dst: 输出的视频格式z
                  // dstStride: 输出的画面大小
                 sws_scale(swsContext,(const uint8_t * const *)avFrameIn->data,
                           avFrameIn->linesize,
                           0,
                           avCodecContext->height,
                           pAVFrameYUV420P->data,
                           pAVFrameYUV420P->linesize);
                 //方式一：直接显示视频上面去
                 //方式二：写入yuv文件格式
                 //5、将yuv420p数据写入.yuv文件中
                 //5.1 计算YUV大小
                 //分析一下原理?
                 //Y表示：亮度
                 //UV表示：色度
                 //有规律
                 //YUV420P格式规范一：Y结构表示一个像素(一个像素对应一个Y)
                 //YUV420P格式规范二：4个像素点对应一个(U和V: 4Y = U = V)
                 ySize = avCodecContext->width * avCodecContext->height;
                 uSize = ySize/4;
                 vSize = ySize/4;
                 fwrite(pAVFrameYUV420P->data[0],1,ySize,fileYUV420P);
                 fwrite(pAVFrameYUV420P->data[1],1,uSize,fileYUV420P);
                 fwrite(pAVFrameYUV420P->data[2],1,vSize,fileYUV420P);
                 currentIndex++;
                 std::cout <<"当前解码 "<<currentIndex<<"帧"<<std::endl;
             }
         }
     }

}
