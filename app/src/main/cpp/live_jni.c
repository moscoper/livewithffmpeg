//
// Created by cuifei on 2017/3/8.
//

#include <jni.h>
#include "libavcodec/avcodec.h"

JNIEXPORT jstring JNICALL
Java_com_cuifei_testffmpeg_MainActivity_helloFFmpeg(JNIEnv *env,jobject instance){

    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());


    return (*env)->NewStringUTF(env, info);


}