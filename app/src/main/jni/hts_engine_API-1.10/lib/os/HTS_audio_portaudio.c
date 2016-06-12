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
#include "portaudio.h"

/* HTS_AudioInterface: audio output for PortAudio */
typedef struct _HTS_AudioInterface {
   PaStreamParameters parameters;       /* parameters for output stream */
   PaStream *stream;            /* output stream */
} HTS_AudioInterface;

/* HTS_AudioInterface_write: send data to audio device */
static void HTS_AudioInterface_write(HTS_AudioInterface * audio_interface, const short *buff, size_t buff_size)
{
   PaError err;

   err = Pa_WriteStream(audio_interface->stream, buff, buff_size);
   if (err != paNoError && err != paOutputUnderflowed)
      HTS_error(0, "hts_engine: Cannot send datablocks to your output audio device to play waveform.\n");
}

/* HTS_AudioInterface_close: close audio device */
static void HTS_AudioInterface_close(HTS_AudioInterface * audio_interface)
{
   PaError err;

   err = Pa_StopStream(audio_interface->stream);
   if (err != paNoError)
      HTS_error(0, "hts_engine: Cannot stop your output audio device.\n");
   err = Pa_CloseStream(audio_interface->stream);
   if (err != paNoError)
      HTS_error(0, "hts_engine: Failed to close your output audio device.\n");
   Pa_Terminate();

   HTS_free(audio_interface);
}

static HTS_AudioInterface *HTS_AudioInterface_open(size_t sampling_frequency, size_t max_buff_size)
{
   HTS_AudioInterface *audio_interface;
   PaError err;

   audio_interface = HTS_calloc(1, sizeof(HTS_AudioInterface));
   audio_interface->stream = NULL;

   err = Pa_Initialize();
   if (err != paNoError) {
      HTS_error(0, "hts_engine: Failed to initialize your output audio device to play waveform.\n");
      HTS_free(audio_interface);
      return NULL;
   }

   audio_interface->parameters.device = Pa_GetDefaultOutputDevice();
   audio_interface->parameters.channelCount = 1;
   audio_interface->parameters.sampleFormat = paInt16;
   audio_interface->parameters.suggestedLatency = Pa_GetDeviceInfo(audio_interface->parameters.device)->defaultLowOutputLatency;
   audio_interface->parameters.hostApiSpecificStreamInfo = NULL;

   err = Pa_OpenStream(&audio_interface->stream, NULL, &audio_interface->parameters, sampling_frequency, max_buff_size, paClipOff, NULL, NULL);
   if (err != paNoError) {
      HTS_error(0, "hts_engine: Failed to open your output audio device to play waveform.\n");
      Pa_Terminate();
      HTS_free(audio_interface);
      return NULL;
   }

   err = Pa_StartStream(audio_interface->stream);
   if (err != paNoError) {
      HTS_error(0, "hts_engine: Failed to start your output audio device to play waveform.\n");
      Pa_CloseStream(audio_interface->stream);
      Pa_Terminate();
      HTS_free(audio_interface);
      return NULL;
   }

   return audio_interface;
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
void HTS_Audio_set_parameter(HTS_Audio * audio, size_t sampling_frequency, size_t max_buff_size)
{
   if (audio == NULL)
      return;

   if (audio->sampling_frequency == sampling_frequency && audio->max_buff_size == max_buff_size)
      return;

   HTS_Audio_clear(audio);

   if (sampling_frequency == 0 || max_buff_size == 0)
      return;

   audio->audio_interface = HTS_AudioInterface_open(sampling_frequency, max_buff_size);
   if (audio->audio_interface == NULL)
      return;

   audio->sampling_frequency = sampling_frequency;
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
         HTS_AudioInterface_write((HTS_AudioInterface *) audio->audio_interface, audio->buff, audio->max_buff_size);
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
      HTS_AudioInterface_write(audio_interface, audio->buff, audio->buff_size);
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
   HTS_AudioInterface_close(audio_interface);
   if (audio->buff != NULL)
      HTS_free(audio->buff);
   HTS_Audio_initialize(audio);
}

HTS_AUDIO_C_END;

#endif                          /* !HTS_AUDIO_C */
