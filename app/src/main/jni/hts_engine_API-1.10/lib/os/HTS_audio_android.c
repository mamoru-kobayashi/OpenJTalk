/* ----------------------------------------------------------------- */
/*           The HMM-Based Speech Synthesis Engine "hts_engine API"  */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2001-2015  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2001-2008  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the HTS working group nor the names of its  */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#ifndef HTS_AUDIO_C
#define HTS_AUDIO_C

#ifdef __cplusplus
#define HTS_AUDIO_C_START extern "C" {
#define HTS_AUDIO_C_END   }
#else
#define HTS_AUDIO_C_START
#define HTS_AUDIO_C_END
#endif                          /* __CPLUSPLUS */

HTS_AUDIO_C_START;

/* hts_engine libralies */
#include "HTS_hidden.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <android/log.h>

#ifdef	DEBUG
#define	LOGV(...)	__android_log_print(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define	LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, __VA_ARGS__)
#else
#define	LOGV(...)
#define	LOGD(...)
#endif

#define	TAG		"HTS_audio"

/* HTS_AudioInterface: audio output for OpenSLES */

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool lock;
	bool init;
} ThreadLock;

typedef struct {
	// engine interface
	SLObjectItf engine_obj;
	SLEngineItf engine;

	// output mix interfaces
	SLObjectItf mix_obj;

	// buffer queue player interaces
	SLObjectItf player_obj;
	SLPlayItf player;
	SLAndroidSimpleBufferQueueItf queue;

	// buffer indexes
	int out_ptr;

	// current buffer half (0,1)
	int out_plane;

	// buffers
	short* out_buff[2];

	// size of buffers
	int out_samples;

	// locks
	ThreadLock out_lock;

	double time;
	int sample_rate;
} HTS_AudioInterface;

static bool init_lock(ThreadLock* lock);
static void destroy_lock(ThreadLock* lock);
static void wait_lock(ThreadLock* lock);
static void notify_lock(ThreadLock* lock);

static HTS_AudioInterface* open_device(int sample_rate, int max_buffer_size);
static void write_device(HTS_AudioInterface* ai, short* buff, size_t buff_size);
static void flush_device(HTS_AudioInterface* ai);
static void close_device(HTS_AudioInterface* ai);

static bool init_lock(ThreadLock* lock)
{
	memset(lock, 0, sizeof(*lock));
	if (pthread_mutex_init(&lock->mutex, (pthread_mutexattr_t*)NULL) != 0) {
		LOGD(TAG, "pthread_mutex_init failed");
		return false;
	}
	if (pthread_cond_init(&lock->cond, (pthread_condattr_t*)NULL) != 0) {
		destroy_lock(lock);
		return false;
	}
	lock->lock = true;
	lock->init = true;
	return true;
}

static void destroy_lock(ThreadLock* lock)
{
	if (lock->init) {
		notify_lock(lock);
		pthread_cond_destroy(&lock->cond);
		pthread_mutex_destroy(&lock->mutex);
		lock->init = false;
	}
}

static void wait_lock(ThreadLock* lock)
{
	pthread_mutex_lock(&lock->mutex);
	while (!lock->lock)
		pthread_cond_wait(&lock->cond, &lock->mutex);
	lock->lock = false;
	pthread_mutex_unlock(&lock->mutex);
}

static void notify_lock(ThreadLock* lock)
{
	pthread_mutex_lock(&lock->mutex);
	lock->lock = true;
	pthread_cond_signal(&lock->cond);
	pthread_mutex_unlock(&lock->mutex);
}

static void play_callback(SLAndroidSimpleBufferQueueItf bq, void* context)
{
	HTS_AudioInterface* ai = (HTS_AudioInterface*)context;
	notify_lock(&ai->out_lock);
}

static HTS_AudioInterface*
open_device(int sample_rate, int max_buffer_size)
{
	HTS_AudioInterface* ai = (HTS_AudioInterface*)calloc(sizeof(*ai), 1);
	SLresult result;

	LOGV(TAG, "open sample=%d,buffer=%d", sample_rate, max_buffer_size);

	// create engine
	result = slCreateEngine(&(ai->engine_obj), 0, NULL, 0, NULL, NULL);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "slCreateEngine failed: result=%d", (int)result);
		goto error;
	}

	// realize the engine
	result = (*ai->engine_obj)->Realize(ai->engine_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "engine->Realize failed: result=%d", (int)result);
		goto error;
	}

	// get the engine interface which is needed in order to create other objects
	result = (*ai->engine_obj)->GetInterface(ai->engine_obj, SL_IID_ENGINE, &(ai->engine));
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "engine->GetInterface failed:result=%d", (int)result);
		goto error;
	}

	// create lock
	if (!init_lock(&ai->out_lock)) {
		LOGD(TAG, "create lock failed");
		goto error;
	}

	// set parameters
	ai->sample_rate = sample_rate;

	// allocate buffers
	ai->out_samples = max_buffer_size;
	ai->out_buff[0] = (short*)calloc(ai->out_samples, sizeof(short));
	ai->out_buff[1] = (short*)calloc(ai->out_samples, sizeof(short));
	if (ai->out_buff[0] == NULL || ai->out_buff[1] == NULL) {
		LOGD(TAG, "buffer allocation error");
		goto error;
	}
	ai->out_plane = 0;
	ai->out_ptr = 0;

	// open the OpenSL ES device for output
	const SLInterfaceID mix_ids[] = { SL_IID_VOLUME };
	const SLboolean mix_req[] = { SL_BOOLEAN_FALSE };
	result = (*ai->engine)->CreateOutputMix(ai->engine, &(ai->mix_obj), 1, mix_ids, mix_req);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "engine->CreateOutputMix failed: result=%d", (int)result);
		goto error;
	}
	result = (*ai->mix_obj)->Realize(ai->mix_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "mix->Realize failed: result=%d", (int)result);
		goto error;
	}
	const int channels = 1;
	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, channels,
		sample_rate * 1000,
		SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
	SLDataLocator_AndroidSimpleBufferQueue loc = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
	};
	SLDataSource data_source = { &loc, &format_pcm };
	
	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = {
		SL_DATALOCATOR_OUTPUTMIX, ai->mix_obj };
	SLDataSink sink = { &loc_outmix, NULL };

	// create audio player
	SLInterfaceID player_ids[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
	SLboolean player_req[] = { SL_BOOLEAN_TRUE };
	result = (*ai->engine)->CreateAudioPlayer(ai->engine, &(ai->player_obj),
		&data_source, &sink, 1, player_ids, player_req); 
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "engine->CreateAudioPlayer failed: result=%d", (int)result);
		goto error;
	}

	// realize the player
	result = (*ai->player_obj)->Realize(ai->player_obj, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "player->Realize faileds: result=%d", (int)result);
		goto error;
	}

	// get the play interface
	result = (*ai->player_obj)->GetInterface(ai->player_obj, SL_IID_PLAY, &(ai->player));
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "player->GetInterface(PLAY) failed: result=%d", (int)result);
		goto error;
	}

	// get the buffer queue interface
	result = (*ai->player_obj)->GetInterface(ai->player_obj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(ai->queue));
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "player->GetInterface(BUFFER) failed: result=%d", (int)result);
		goto error;
	}

	// register callback on the buffer queue
	result = (*ai->queue)->RegisterCallback(ai->queue, play_callback, ai);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "register callback failed: result=%d", (int)result);
		goto error;
	}

	// set the player's state to playing
	result = (*ai->player)->SetPlayState(ai->player, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS) {
		LOGD(TAG, "set play state failed: result=%d", (int)result);
		goto error;
	}

	notify_lock(&ai->out_lock);
	return ai;
error:
	close_device(ai);
	return NULL;
}

static void
write_device(HTS_AudioInterface* ai, short* buff, size_t buff_size)
{
	if (ai == 0)
		return;
	int n = ai->out_samples;
	int index = ai->out_ptr;
	short* out = ai->out_buff[ai->out_plane];
	for (size_t i = 0; i < buff_size; i++) {
		out[index++] = buff[i];
		if (index >= n) {
			wait_lock(&ai->out_lock);
			(*ai->queue)->Enqueue(ai->queue, out, n * sizeof(short));
			ai->out_plane = ai->out_plane ? 0 : 1;
			index = 0;
			out = ai->out_buff[ai->out_plane];
		}
	}
	ai->out_ptr = index;
}

static void 
flush_device(HTS_AudioInterface* ai)
{
	if (ai->out_ptr > 0) {
		wait_lock(&ai->out_lock);
		(*ai->queue)->Enqueue(ai->queue, ai->out_buff[ai->out_plane], ai->out_ptr * sizeof(short));
		ai->out_ptr = 0;
		ai->out_plane = ai->out_plane ? 0 : 1;
	}
}

static void
close_device(HTS_AudioInterface* ai)
{
	if (ai == NULL)
		return;
	if (ai->player_obj != NULL) {
		(*ai->player_obj)->Destroy(ai->player_obj);
		ai->player_obj = NULL;
		ai->player = NULL;
		ai->queue = NULL;
	}
	if (ai->mix_obj != NULL) {
		(*ai->mix_obj)->Destroy(ai->mix_obj);
		ai->mix_obj = NULL;
	}
	if (ai->engine_obj != NULL) {
		(*ai->engine_obj)->Destroy(ai->engine_obj);
		ai->engine_obj = NULL;
		ai->engine = NULL;
	}
	destroy_lock(&ai->out_lock);
	if (ai->out_buff[0] != NULL) {
		free(ai->out_buff[0]);
		ai->out_buff[0] = NULL;
	}
	if (ai->out_buff[1] != NULL) {
		free(ai->out_buff[1]);
		ai->out_buff[1] = NULL;
	}
	free(ai);
}

/* HTS_Audio_initialize: initialize audio */
void HTS_Audio_initialize(HTS_Audio * audio)
{
   if (audio == NULL)
      return;

   audio->sampling_frequency = 0;
   audio->max_buff_size = 0;
   audio->buff = NULL;
   audio->buff_size = 0;
   audio->audio_interface = NULL;
}

/* HTS_Audio_set_parameter: set parameters for audio */
void HTS_Audio_set_parameter(HTS_Audio * audio, size_t sample_rate, size_t max_buff_size)
{
   if (audio == NULL)
      return;

   if (audio->sampling_frequency == sample_rate && audio->max_buff_size == max_buff_size)
      return;

   HTS_Audio_clear(audio);

   if (sample_rate == 0 || max_buff_size == 0)
      return;

   audio->audio_interface = open_device(sample_rate, max_buff_size);
   if (audio->audio_interface == NULL)
      return;

   audio->sampling_frequency = sample_rate;
   audio->max_buff_size = max_buff_size;
   audio->buff = (short *) HTS_calloc(max_buff_size, sizeof(short));
   audio->buff_size = 0;
}

/* HTS_Audio_write: send data to audio device */
void HTS_Audio_write(HTS_Audio * audio, short data)
{
	if (audio == NULL)
		return;

	audio->buff[audio->buff_size++] = data;

	if (audio->buff_size >= audio->max_buff_size) {
		if (audio->audio_interface != NULL)
			write_device((HTS_AudioInterface *) audio->audio_interface, audio->buff, audio->max_buff_size);
		audio->buff_size = 0;
	}
}

/* HTS_Audio_flush: flush remain data */
void HTS_Audio_flush(HTS_Audio * audio)
{
   HTS_AudioInterface *audio_interface;

   if (audio == NULL || audio->audio_interface == NULL)
      return;

   audio_interface = (HTS_AudioInterface *) audio->audio_interface;
   if (audio->buff_size > 0) {
      write_device(audio_interface, audio->buff, audio->buff_size);
	  flush_device(audio_interface);
      audio->buff_size = 0;
   }
}

/* HTS_Audio_clear: free audio */
void HTS_Audio_clear(HTS_Audio * audio)
{
   HTS_AudioInterface *audio_interface;

   if (audio == NULL || audio->audio_interface == NULL)
      return;
   audio_interface = (HTS_AudioInterface *) audio->audio_interface;

   HTS_Audio_flush(audio);
   close_device(audio_interface);
   if (audio->buff != NULL)
      HTS_free(audio->buff);
   HTS_Audio_initialize(audio);
}

HTS_AUDIO_C_END;

#endif                          /* !HTS_AUDIO_C */
