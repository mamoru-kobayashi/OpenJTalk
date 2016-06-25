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

#include <alsa/asoundlib.h>

#define	PCM_DEVICE	"default"
#define	true	1
#define	false	0
typedef char bool;

/* HTS_AudioInterface: audio output for PortAudio */
typedef struct _HTS_AudioInterface {
   snd_pcm_t* pcm;
   snd_pcm_hw_params_t* params;
   short* buffer;
   bool prepared;
   bool error;
} HTS_AudioInterface;

/* HTS_AudioInterface_write: send data to audio device */
static void HTS_AudioInterface_write(HTS_AudioInterface* ai, const short *buff, size_t buff_size)
{
   int err;
   if (!ai->prepared) {
      err = snd_pcm_prepare(ai->pcm);
      if (err < 0) {
         HTS_error(0, "hts_engine: Can't prepare audio device. %s\n",
            snd_strerror(err));
         ai->error = true;
      }
      ai->prepared = true;
   }
   if (ai->error) {
      return;
   }
   while (buff_size > 0) {
      snd_pcm_sframes_t result = snd_pcm_writei(ai->pcm, buff, buff_size);
      switch (result) {
      case -EPIPE:
         ai->prepared = false;
         ai->error = false;
         return;
      case -EBADFD:
         HTS_error(0, "hts_engine: PCM is not in the right state.\n");
         return;
      case -ESTRPIPE:
         do {
            err = snd_pcm_resume(ai->pcm);
         } while (err == -EAGAIN);
         snd_pcm_prepare(ai->pcm);
         break;
      }
      buff_size -= result;
      buff += result;
   }
}

/* HTS_AudioInterface_close: close audio device */
static void HTS_AudioInterface_close(HTS_AudioInterface* ai)
{
   if (ai == 0)
      return;
   if (ai->pcm != 0) {
      snd_pcm_close(ai->pcm);
      ai->pcm = 0;
   }
   if (ai->params != 0) {
      snd_pcm_hw_params_free(ai->params);
      ai->params = 0;
   }
   if (ai->buffer != 0) {
      HTS_free(ai->buffer);
      ai->buffer = 0;
   }
   HTS_free(ai);
}

static HTS_AudioInterface *HTS_AudioInterface_open(size_t sampling_frequency, size_t max_buff_size)
{
   HTS_AudioInterface *ai;
   int err;

   ai = HTS_calloc(1, sizeof(HTS_AudioInterface));
   if (ai == 0) {
      HTS_error(0, "hts_engine: Can't allocate memory.");
      return 0;
   }
   ai->buffer = HTS_calloc(max_buff_size, sizeof(short));
   if (ai->buffer == 0) {
      HTS_error(0, "hts_engine: Can't allocate memory.");
      HTS_AudioInterface_close(ai);
      return 0;
   }

   /* Open the PCM device in playback mode */
   err = snd_pcm_open(&ai->pcm, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
   if (err < 0) {
      HTS_error(0, "hts_engine: Can't open \"%s\" PCM device. %s\n",
         PCM_DEVICE, snd_strerror(err));
      HTS_AudioInterface_close(ai);
      return 0;
   }

   /* Allocate parameters object and fill it with default values */
   snd_pcm_hw_params_malloc(&ai->params);
   snd_pcm_hw_params_any(ai->pcm, ai->params);

   /* Set parameters */
   err = snd_pcm_hw_params_set_access(
            ai->pcm, ai->params, SND_PCM_ACCESS_RW_INTERLEAVED);
   if (err < 0) {
      HTS_error(0, "hts_engine: Can't set interleaved mode. %s\n",
         snd_strerror(err));
      HTS_AudioInterface_close(ai);
      return 0;
   }
   err = snd_pcm_hw_params_set_format(
            ai->pcm, ai->params, SND_PCM_FORMAT_S16_LE);
   if (err < 0) {
      HTS_error(0, "hts_engine: Can't set format. %s\n", snd_strerror(err));
      HTS_AudioInterface_close(ai);
      return 0;
   }
   err = snd_pcm_hw_params_set_channels(ai->pcm, ai->params, 1);
   if (err < 0) {
      HTS_error(0, "hts_engine: Can't set channels. %s\n", snd_strerror(err));
      HTS_AudioInterface_close(ai);
      return 0;
   }
   unsigned int rate = sampling_frequency;
   err = snd_pcm_hw_params_set_rate_near(ai->pcm, ai->params, &rate, 0);
   if (err < 0) {
      HTS_error(0, "hts_engine: Can't set rate. %s\n", snd_strerror(err));
      HTS_AudioInterface_close(ai);
      return 0;
   }

   /* Write parameters */
   err = snd_pcm_hw_params(ai->pcm, ai->params);
   if (err < 0) {
      HTS_error(0, "hts_engine: Can't set hardware parameters. %s\n",
          snd_strerror(err));
      HTS_AudioInterface_close(ai);
      return 0;
   }

   snd_pcm_hw_params_free(ai->params);
   ai->params = 0;
   ai->prepared = false;
   ai->error = false;

   return ai;
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
