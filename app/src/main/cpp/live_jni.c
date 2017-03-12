//
// Created by cuifei on 2017/3/8.
//

#include <jni.h>
#include <android/log.h>
#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "libavutil/imgutils.h"

#define LOG_TAG "FFmpeg"

#define LOGE(format,...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, format, ##__VA_ARGS__)
#define LOGI(format,...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, format, ##__VA_ARGS__)

AVFormatContext *ofmt_ctx = NULL;
AVStream *out_stream = NULL;
AVPacket pkt;
AVCodecContext *pCodecCtx = NULL;
AVCodec *pCodec = NULL;
AVFrame *yuv_frame = NULL;

int frame_count;
int src_width;
int src_height;
int y_length;
int uv_length;
int64_t start_time;


void live_log(void *ptr,int level, const char* fmt,va_list vl){
    FILE *fp = fopen("/storage/emulate/0/123/live_log.txt", "a+");
    if(fp) {
       vfprintf(fp, fmt, vl);
       fflush(fp);
       fclose(fp);
    }
}


int encode(AVCodecContext *pCodecCtx,AVPacket *pPkt,AVFrame *pFrame,int *got_packet){

  int ret;
  *got_packet = 0;
  ret = avcodec_send_frame(pCodecCtx,pFrame);

  if(ret <0 && ret != AVERROR_EOF) {
          return ret;
      }

     ret = avcodec_receive_packet(pCodecCtx,pPkt);

     if(ret < 0 && ret != AVERROR(EAGAIN)) {
             return ret;
         }

         if(ret >= 0) {
             *got_packet = 1;
         }

         return 0;

}


JNIEXPORT jint JNICALL
Java_com_cuifei_testffmpeg_LiveActivity_streamRelease(JNIEnv *env, jobject instance){

  if(pCodecCtx){
      avcodec_close(pCodecCtx);
      pCodecCtx = NULL;
  }

  if(ofmt_ctx){
     avio_close(ofmt_ctx->pb);
  }

  if(ofmt_ctx){
    avformat_free_context(ofmt_ctx);
    ofmt_ctx = NULL;
  }

  if(yuv_frame){
    av_frame_free(yuv_frame);
    yuv_frame = NULL;
  }


}

JNIEXPORT jint JNICALL
Java_com_cuifei_testffmpeg_LiveActivity_streamerFlush(JNIEnv *env,jobject instance){

  int ret;
  int got_packet;


  AVPacket packet;

  if(!(pCodec->capabilities & CODEC_CAP_DELAY)) {
          return 0;
  }

   while(1){
    packet.data = NULL;
    packet.size =0;
    av_init_packet(&packet);
    if(ret < 0) {
        break;
    }
    if(!got_packet) {
         ret = 0;
         break;
    }

    LOGI("Encode 1 frame size:%d\n", packet.size);

    AVRational time_base = ofmt_ctx->streams[0]->time_base;
    AVRational r_frame_rate1 = {60,2};
    AVRational time_base_q = {1,AV_TIME_BASE};

    int64_t calc_duration = (double)AV_TIME_BASE * (1/av_q2d(r_frame_rate1));
    packet.pts = av_rescale_q(frame_count * calc_duration,time_base_q,time_base);
    packet.dts = packet.pts;
    packet.pos = -1;
    frame_count++;
    ofmt_ctx->duration = packet.duration * frame_count;
    ret = av_interleaved_write_frame(ofmt_ctx,&packet);

    if(ret < 0) {
                break;
    }

   }

    av_write_trailer(ofmt_ctx);
    return 0;

}


JNIEXPORT jint JNICALL
Java_com_cuifei_testffmpeg_LiveActivity_streamerHandle(JNIEnv *env,jobject instance,jbyteArray data_,jlong timetamp){
  jbyte *data = (*env)->GetByteArrayElements(env,data_,NULL);

  int ret, i,resultCode;
  int got_packet=0;

  resultCode = 0;
  memcpy(yuv_frame->data[0],data,y_length);

  for(i=0; i < uv_length;i++){
    *(yuv_frame->data[2] + i) = *(data + y_length + 2 * i);
    *(yuv_frame->data[1] + i) = *(data + y_length + 2 * i + 1);
  }

  yuv_frame->format = pCodecCtx->pix_fmt;
  yuv_frame->width=src_width;
  yuv_frame->height=src_height;

 // yuv_frame->pts = (1.0 / 30) * 90 * frame_count;
 yuv_frame->pts = timetamp * 30 / 1000000;

  pkt.data = NULL;
  pkt.size = 0;

  av_init_packet(&pkt);

  ret = encode(pCodecCtx,&pkt,yuv_frame,&got_packet);

  if(got_packet){
       LOGI("Encode frame: %d\tsize:%d\n", frame_count, pkt.size);
       frame_count++;

       pkt.stream_index = out_stream->index;
       av_packet_rescale_ts(&pkt,pCodecCtx->time_base,out_stream->time_base);

      /* AVRational time_base1 = ofmt_ctx->streams[0]->time_base;
       AVRational r_frame_rate1 = {60,2};

       AVRational time_base_q={1,AV_TIME_BASE};
       int64_t calc_duration = (double)(AV_TIME_BASE) * (1 / av_q2d(r_frame_rate1));

       pkt.pts = av_rescale_q(frame_count * calc_duration, time_base_q, time_base1);
       pkt.dts = pkt.pts;
       pkt.duration = av_rescale_q(calc_duration, time_base_q, time_base1);
       pkt.pos = -1;

       //处理延迟
       int64_t pts_time = av_rescale_q(pkt.dts, time_base1, time_base_q);
       int64_t now_time = av_gettime() - start_time;
       if(pts_time > now_time) {
           av_usleep(pts_time - now_time);
       }*/

       ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
       if(ret < 0) {
           LOGE("Error muxing packet");
           resultCode = -1;
           goto end;
       }
       av_packet_unref(&pkt);


  }

  end:
      (*env)->ReleaseByteArrayElements(env, data_, data, 0);
      return resultCode;


}

JNIEXPORT jint JNICALL
Java_com_cuifei_testffmpeg_LiveActivity_streamerInit(JNIEnv *env,jobject instance,jint width,jint height){

    int ret = 0;
    //记得改
    const char *address = "rtmp://192.168.10.156/oflaDemo/test";

    src_height = height;
    src_width = width;

    y_length = width * height;
    uv_length = y_length/4;
    av_log_set_callback(live_log);

     av_register_all();

     avformat_network_init();

     avformat_alloc_output_context2(&ofmt_ctx,NULL,"flv",address);

    if(!ofmt_ctx) {
        LOGE("Could not create output context\n");
        return -2;
    }

    pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if(!pCodec) {
            LOGE("Can not find encoder!\n");
            return -3;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = width;
    pCodecCtx->height = height;

    pCodecCtx->time_base.num=1;
    pCodecCtx->time_base.den= 30;
    pCodecCtx->bit_rate= 800000;

    pCodecCtx->gop_size= 300;

    if(ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    pCodecCtx->max_b_frames = 3;

    AVDictionary *dicParams = NULL;
    av_dict_set(&dicParams, "preset", "ultrafast", 0);
    av_dict_set(&dicParams, "tune", "zerolatency", 0);

    //打开编码器
        if(avcodec_open2(pCodecCtx, pCodec, &dicParams) < 0) {
            LOGE("Failed to open encoder!\n");
            return -4;
        }


      //新建输出流
          out_stream = avformat_new_stream(ofmt_ctx, pCodec);
          if(!out_stream) {
              LOGE("Failed allocation output stream\n");
              return -5;
          }

        out_stream->time_base.num = 1;
        out_stream->time_base.den = 30;
        //复制一份编码器的配置给输出流
        avcodec_parameters_from_context(out_stream->codecpar, pCodecCtx);

        //打开输出流
        ret = avio_open(&ofmt_ctx->pb, address, AVIO_FLAG_WRITE);

         if(ret < 0) {
                 LOGE("Could not open output URL %s", address);
                 return -6;
         }

         ret = avformat_write_header(ofmt_ctx, NULL);
             if(ret < 0) {
                 LOGE("Error occurred when open output URL\n");
                 return -7;
             }

     //初始化一个帧的数据结构，用于编码用
         //指定AV_PIX_FMT_YUV420P这种格式的
         yuv_frame = av_frame_alloc();
         uint8_t *out_buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(pCodecCtx->pix_fmt, src_width, src_height, 1));
         av_image_fill_arrays(yuv_frame->data, yuv_frame->linesize, out_buffer, pCodecCtx->pix_fmt, src_width, src_height, 1);

         start_time = av_gettime();

         return 0;

}



JNIEXPORT jstring JNICALL
Java_com_cuifei_testffmpeg_MainActivity_helloFFmpeg(JNIEnv *env,jobject instance){

    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());


    return (*env)->NewStringUTF(env, info);


}