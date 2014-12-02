#include <com_fb_screenshotapp_fbjni.h>
#include <stdio.h>
#include "fbbuffer.h"

JNIEXPORT jstring JNICALL Java_com_fb_screenshotapp_fbjni_test
  (JNIEnv *env, jclass jca){
	return (*env)->NewStringUTF(env, "Hello world!");
}


JNIEXPORT jint JNICALL Java_com_fb_screenshotapp_fbjni_bufferlength
  (JNIEnv *env, jclass jca, jstring path ){
	jint ret = 10;
    return ret;
}

JNIEXPORT jbyteArray JNICALL Java_com_fb_screenshotapp_fbjni_screenPngBytes
  (JNIEnv *env, jclass jca,jint size){

    unsigned char* fbb = read_fb_buffer();
    jbyteArray arr = (*env)->NewByteArray(env, size);
    (*env)->SetByteArrayRegion(env,arr,0,size, (jbyte*)fbb);

    freefbb(fbb);

    return arr;
}


