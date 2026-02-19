

#ifndef AVCODEC_JNI_H
#define AVCODEC_JNI_H


int av_jni_set_java_vm(void *vm, void *log_ctx);


void *av_jni_get_java_vm(void *log_ctx);


int av_jni_set_android_app_ctx(void *app_ctx, void *log_ctx);


void *av_jni_get_android_app_ctx(void);

#endif 
