#include <string.h>
#include <android/log.h>

#include "OpenJTalk.h"

#include "mecab.h"
#include "njd.h"
#include "jpcommon.h"
#include "HTS_engine.h"

#include "text2mecab.h"
#include "mecab2njd.h"
#include "njd_set_pronunciation.h"
#include "njd_set_digit.h"
#include "njd_set_accent_phrase.h"
#include "njd_set_accent_type.h"
#include "njd_set_unvoiced_vowel.h"
#include "njd_set_long_vowel.h"
#include "njd2jpcommon.h"

#ifdef	DEBUG
#define	LOGV(...)	__android_log_print(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define	LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, __VA_ARGS__)
#else
#define	LOGV(...)
#define	LOGD(...)
#endif

#define	TAG	"OpenJTalk"

class OpenJTalk
{
	Mecab _mecab;
	NJD _njd;
	JPCommon _jpcommon;
	HTS_Engine _engine;
	
public:
	//------------------------------------------------------------------------
	//	コンストラクタ
	//------------------------------------------------------------------------

	OpenJTalk();
	virtual ~OpenJTalk();

	//------------------------------------------------------------------------
	//	属性
	//------------------------------------------------------------------------

	int samplingFrequency();
	void setSamplingFrequency(int freq);
	double alpha();
	void setAlpha(double alpha);
	double beta();
	void setBeta(double beta);
	void setSpeed(double speed);
	void addHalfTone(double value);
	double msdThreshold(int index);
	void setMsdThreshold(int index, double t);
	double gvWeight(int index);
	void setGvWeight(int index, double w);
	double volume();
	void setVolume(double v);
	int audioBufferSize();
	void setAudioBufferSize(int size);

	//------------------------------------------------------------------------
	//	操作
	//------------------------------------------------------------------------

	bool load(const char* dir_mecab, const char* fn_voice);

	bool talk(const char* txt, const char* wave, const char* log);
};

//------------------------------------------------------------------------
//	コンストラクタ
//------------------------------------------------------------------------

OpenJTalk::OpenJTalk()
{
	Mecab_initialize(&_mecab);
	NJD_initialize(&_njd);
	JPCommon_initialize(&_jpcommon);
	HTS_Engine_initialize(&_engine);
}

OpenJTalk::~OpenJTalk()
{
	Mecab_clear(&_mecab);
	NJD_clear(&_njd);
	JPCommon_clear(&_jpcommon);
	HTS_Engine_clear(&_engine);
}

//------------------------------------------------------------------------
//	属性
//------------------------------------------------------------------------

int OpenJTalk::samplingFrequency()
{
	return HTS_Engine_get_sampling_frequency(&_engine);
}

void OpenJTalk::setSamplingFrequency(int freq)
{
	HTS_Engine_set_sampling_frequency(&_engine, freq);
}

double OpenJTalk::alpha()
{
	return HTS_Engine_get_alpha(&_engine);
}

void OpenJTalk::setAlpha(double value)
{
	HTS_Engine_set_alpha(&_engine, value);
}

double OpenJTalk::beta()
{
	return HTS_Engine_get_beta(&_engine);
}

void OpenJTalk::setBeta(double value)
{
	HTS_Engine_set_beta(&_engine, value);
}

void OpenJTalk::setSpeed(double speed)
{
	HTS_Engine_set_speed(&_engine, speed);
}

void OpenJTalk::addHalfTone(double value)
{
	HTS_Engine_add_half_tone(&_engine, value);
}

double OpenJTalk::msdThreshold(int index)
{
	return HTS_Engine_get_msd_threshold(&_engine, index);
}

void OpenJTalk::setMsdThreshold(int index, double value)
{
	HTS_Engine_set_msd_threshold(&_engine, index, value);
}

double OpenJTalk::gvWeight(int index)
{
	return HTS_Engine_get_gv_weight(&_engine, index);
}

void OpenJTalk::setGvWeight(int index, double value)
{
	HTS_Engine_set_gv_weight(&_engine, index, value);
}

double OpenJTalk::volume()
{
	return HTS_Engine_get_volume(&_engine);
}

void OpenJTalk::setVolume(double value)
{
	HTS_Engine_set_volume(&_engine, value);
}

int OpenJTalk::audioBufferSize()
{
	return HTS_Engine_get_audio_buff_size(&_engine);
}

void OpenJTalk::setAudioBufferSize(int value)
{
	HTS_Engine_set_audio_buff_size(&_engine, value);
}

//------------------------------------------------------------------------
//	操作
//------------------------------------------------------------------------

bool OpenJTalk::load(const char* dn_mecab, const char* fn_voice)
{
	LOGV(TAG, "OpenJTalk.load dn_mecab=%s, fn_voice=%s", dn_mecab, fn_voice);
	if (Mecab_load(&_mecab, dn_mecab) != TRUE) {
		LOGD(TAG, "Mecab_load failed");
		return false;
	}
	if (HTS_Engine_load(&_engine, (char**)&fn_voice, 1) != TRUE) {
		LOGD(TAG, "HTS_Engine_load failed");
		return false;
	}
	if (strcmp(HTS_Engine_get_fullcontext_label_format(&_engine),
		"HTS_TTS_JPN") != 0) {
		LOGD(TAG, "HTS_Engine_fulltext_label_format is not 'HTS_TTS_JPN'");
		return false;
	}
	return true;
}

bool OpenJTalk::talk(const char* txt, const char* wave, const char* log)
{
	bool success = false;
	char buff[1024];

	text2mecab(buff, txt);
	Mecab_analysis(&_mecab, buff);
	mecab2njd(&_njd, Mecab_get_feature(&_mecab), Mecab_get_size(&_mecab));
	njd_set_pronunciation(&_njd);
	njd_set_digit(&_njd);
	njd_set_accent_phrase(&_njd);
	njd_set_accent_type(&_njd);
	njd_set_unvoiced_vowel(&_njd);
	njd_set_long_vowel(&_njd);
	njd2jpcommon(&_jpcommon, &_njd);
	JPCommon_make_label(&_jpcommon);
	int n = JPCommon_get_label_size(&_jpcommon);
	if (n > 2) {
		if (HTS_Engine_synthesize_from_strings(&_engine,
			JPCommon_get_label_feature(&_jpcommon),
			JPCommon_get_label_size(&_jpcommon)) == TRUE)
			success = true;
		if (wave != 0) {
			FILE* fp = fopen(wave, "w");
			if (fp != 0) {
				LOGV(TAG, "OpenJTalk.talk save riff to=%s", wave);
				HTS_Engine_save_riff(&_engine, fp);
				fclose(fp);
			}
		}
		if (log != 0) {
			FILE* fp = fopen(log, "w");
			if (fp != 0) {
				LOGV(TAG, "OpenJTalk.talk save log to=%s", wave);
				fprintf(fp, "[Text analysis result]\n");
				NJD_fprint(&_njd, fp);
				fprintf(fp, "\n[Outputlabel]\n");
				HTS_Engine_save_label(&_engine, fp);
				fprintf(fp, "\n");
				HTS_Engine_save_information(&_engine, fp);
				fclose(fp);
			}
		}
		LOGV(TAG, "OpenJTalk.talk HTS_Engine_synthesize: %s",
			success ? "SUCCESS" : "ERROR");
		HTS_Engine_refresh(&_engine);
	}
	JPCommon_refresh(&_jpcommon);
	NJD_refresh(&_njd);
	Mecab_refresh(&_mecab);
	return success;
}

//------------------------------------------------------------------------
//	Java Interface
//------------------------------------------------------------------------

jlong JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeInit(JNIEnv* env, jclass cls)
{
	OpenJTalk* ojt = new OpenJTalk();
	return (jlong)ojt;
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeDestroy(
	JNIEnv* env, jclass cls, jlong instance)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	if (ojt != 0)
		delete ojt;
}

jint JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetSamplingFrequency(
	JNIEnv* env, jclass cls, jlong instance)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jint)ojt->samplingFrequency();
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetSamplingFrequency(
	JNIEnv* env, jclass cls, jlong instance, jint freq)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setSamplingFrequency((int)freq);
}

jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetAlpha(
	JNIEnv* env, jclass cls, jlong instance)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jdouble)ojt->alpha();
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetAlpha(
	JNIEnv* env, jclass cls, jlong instance, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setAlpha((double)value);
}

jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetBeta(
	JNIEnv* env, jclass cls, jlong instance)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jdouble)ojt->beta();
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetBeta(
	JNIEnv* env, jclass cls, jlong instance, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setBeta((double)value);
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetSpeed(
	JNIEnv* env, jclass cls, jlong instance, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setSpeed((double)value);
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeAddHalfTone(
	JNIEnv* env, jclass cls, jlong instance, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->addHalfTone((double)value);
}

jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetMsdThreshold(
	JNIEnv* env, jclass cls, jlong instance, jint index)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jdouble)ojt->msdThreshold((int)index);
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetMsdThreshold(
	JNIEnv* env, jclass cls, jlong instance, jint index, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setMsdThreshold((int)index, (double)value);
}

jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetGvWeight(
	JNIEnv* env, jclass cls, jlong instance, jint index)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jdouble)ojt->gvWeight((int)index);
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetGvWeight(
	JNIEnv* env, jclass cls, jlong instance, jint index, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setGvWeight((int)index, (double)value);
}

jdouble JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetVolume(
	JNIEnv* env, jclass cls, jlong instance)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jdouble)ojt->volume();
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetVolume(
	JNIEnv* env, jclass cls, jlong instance, jdouble value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setVolume((double)value);
}

jint JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeGetAudioBufferSize(
	JNIEnv* env, jclass cls, jlong instance)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	return (jint)ojt->audioBufferSize();
}

void JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeSetAudioBufferSize(
	JNIEnv* env, jclass cls, jlong instance, jint value)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	ojt->setAudioBufferSize((int)value);
}

jboolean JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeLoad(
	JNIEnv* env, jclass cls, jlong instance,
	jstring dn_mecab_obj, jstring fn_voice_obj)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	const char* dn_mecab = env->GetStringUTFChars(dn_mecab_obj, NULL);
	const char* fn_voice = env->GetStringUTFChars(fn_voice_obj, NULL);
	bool success = ojt->load(dn_mecab, fn_voice);
	env->ReleaseStringUTFChars(dn_mecab_obj, dn_mecab);
	env->ReleaseStringUTFChars(fn_voice_obj, fn_voice);
	return (jboolean)success;
}

jboolean JNICALL
Java_jp_itplus_openjtalk_OpenJTalk_nativeTalk(
	JNIEnv* env, jclass cls, jlong instance,
	jstring text_obj, jstring wavefile_obj, jstring log_obj)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	const char* text = env->GetStringUTFChars(text_obj, NULL);
	const char* wave = 0;
	const char* log = 0;
	if (wavefile_obj != 0)
		wave = env->GetStringUTFChars(wavefile_obj, NULL);
	if (log_obj != 0)
		log = env->GetStringUTFChars(log_obj, NULL);
	bool success = ojt->talk(text, wave, log);
	env->ReleaseStringUTFChars(text_obj, text);
	if (wave != 0)
		env->ReleaseStringUTFChars(wavefile_obj, wave);
	if (log != 0)
		env->ReleaseStringUTFChars(log_obj, log);
	return (jboolean)success;
}
