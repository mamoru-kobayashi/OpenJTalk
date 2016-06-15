#ifndef	H_OPEN_JTALK_H
#define	H_OPEN_JTALK_H

#include <jni.h>

extern "C" {

JNIEXPORT jlong JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeInit(JNIEnv* env, jclass cls);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeDestroy(
	JNIEnv* env, jclass cls, jlong instance);

JNIEXPORT jint JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetSamplingFrequency(
	JNIEnv* env, jclass cls, jlong instance);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetSamplingFrequency(
	JNIEnv* env, jclass cls, jlong instance, jint freq);

JNIEXPORT jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetAlpha(
	JNIEnv* env, jclass cls, jlong instance);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetAlpha(
	JNIEnv* env, jclass cls, jlong instance, jdouble value);

JNIEXPORT jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetBeta(
	JNIEnv* env, jclass cls, jlong instance);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetBeta(
	JNIEnv* env, jclass cls, jlong instance, jdouble value);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetSpeed(
	JNIEnv* env, jclass cls, jlong instance, jdouble value);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeAddHalfTone(
	JNIEnv* env, jclass cls, jlong instance, jdouble value);

JNIEXPORT jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetMsdThreshold(
	JNIEnv* env, jclass cls, jlong instance, jint index);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetMsdThreshold(
	JNIEnv* env, jclass cls, jlong instance, jint index, jdouble value);

JNIEXPORT jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetGvWeight(
	JNIEnv* env, jclass cls, jlong instance, jint index);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetGvWeight(
	JNIEnv* env, jclass cls, jlong instance, jint index, jdouble value);

JNIEXPORT jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetVolume(
	JNIEnv* env, jclass cls, jlong instance);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetVolume(
	JNIEnv* env, jclass cls, jlong instance, jdouble value);

JNIEXPORT jint JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetAudioBufferSize(
	JNIEnv* env, jclass cls, jlong instance);

JNIEXPORT void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetAudioBufferSize(
	JNIEnv* env, jclass cls, jlong instance, jint value);

JNIEXPORT jboolean JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeLoad(
	JNIEnv* env, jclass cls, jlong instance,
	jstring lang_obj, jstring dict_obj, jstring voice_obj);

JNIEXPORT jboolean JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeTalk(
	JNIEnv* env, jclass cls, jlong instance,
	jstring text_obj, jstring wavefile_obj, jstring logfile_obj);

}
#endif	/* H_OPEN_JTALK_H */
