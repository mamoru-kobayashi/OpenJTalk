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

extern "C" {
#include "cst_synth.h"
#include "cst_utt_utils.h"
#include "cst_math.h"
#include "cst_file.h"
#include "cst_val.h"
#include "cst_string.h"
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_tokenstream.h"
#include "cst_string.h"
#include "cst_regex.h"
#include "cst_features.h"
#include "cst_utterance.h"
#include "flite.h"
#include "cst_synth.h"
#include "cst_utt_utils.h"

extern cst_voice* register_cmu_us_kal(const char* dict);
extern void unregister_cmu_us_kal(cst_voice* vox);
}

#ifdef	DEBUG
#define	LOGV(...)	__android_log_print(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define	LOGD(...)	__android_log_print(ANDROID_LOG_DEBUG, __VA_ARGS__)
#else
#define	LOGV(...)
#define	LOGD(...)
#endif

#define	TAG	"OpenJTalk"

static const int MAXBUFLEN = 1024;

class OpenJTalk
{
public:
	struct Grammar {
		virtual ~Grammar() {}
		virtual bool canTalk(const char* label) { return true; }
		virtual char** labels() = 0;
		virtual int count() = 0;
		virtual int minCount() = 0;
		virtual bool parse(const char* text) = 0;
		virtual void reset() {}
		virtual void log(FILE* fp) {}
		static Grammar* load(const char* lang, const char* dict_dir);
	};
	friend class JPGrammar;
	friend class FliteGrammar;

private:
	//------------------------------------------------------------------------
	//	Instance variables
	//------------------------------------------------------------------------

	HTS_Engine _engine;
	Grammar* _grammar;
	
public:
	//------------------------------------------------------------------------
	//	Constructors
	//------------------------------------------------------------------------

	OpenJTalk();
	virtual ~OpenJTalk();

	//------------------------------------------------------------------------
	//	Properties
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
	//	Operations
	//------------------------------------------------------------------------

	bool load(const char* lang, const char* dict, const char* voice);

	bool talk(const char* txt, const char* wave, const char* log);
};

//------------------------------------------------------------------------
//	Grammar implementation classes
//------------------------------------------------------------------------

class JPGrammar : public OpenJTalk::Grammar
{
	Mecab _mecab;
	NJD _njd;
	JPCommon _jpcommon;

	JPGrammar();

public:
	virtual ~JPGrammar();

	virtual bool canTalk(const char* label);

	virtual char** labels() {
		return JPCommon_get_label_feature(&_jpcommon);
	}
	virtual int count() {
		return JPCommon_get_label_size(&_jpcommon);
	}
	virtual int minCount() { return 2; }

	virtual bool parse(const char* text);
	virtual void reset();
	virtual void log(FILE* fp);

	static Grammar* load(const char* dict_dir);
};

class FliteGrammar : public OpenJTalk::Grammar
{
	cst_voice* _voice;
	cst_utterance* _u;
	char** _labels;
	int _count;

	FliteGrammar(cst_voice* voice)
		: _voice(voice), _u(0), _labels(0), _count(0) {}
public:
	virtual ~FliteGrammar();

	virtual char** labels() { return _labels; }
	virtual int count() { return _count; }
	virtual int minCount() { return 1; }

	virtual bool parse(const char* text);
	virtual void reset();

	static Grammar* load(const char* dict_dir);

private:
	void createLabel(cst_item* item, char* label);
};

//------------------------------------------------------------------------
//	Constructors
//------------------------------------------------------------------------

OpenJTalk::OpenJTalk()
	: _grammar(0)
{
	HTS_Engine_initialize(&_engine);
}

OpenJTalk::~OpenJTalk()
{
	delete _grammar;
	HTS_Engine_clear(&_engine);
}

//------------------------------------------------------------------------
//	Properties
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
//	Operations
//------------------------------------------------------------------------

bool OpenJTalk::load(const char* lang, const char* dict, const char* voice)
{
	LOGV(TAG, "OpenJTalk.load lang=%s,dict=%s,voice=%s", lang, dict, voice);

	// load grammar
	delete _grammar;
	_grammar = 0;
	Grammar* grammar = Grammar::load(lang, dict);
	if (grammar == 0)
		return false;
	if (HTS_Engine_load(&_engine, (char**)&voice, 1) != TRUE) {
		LOGD(TAG, "HTS_Engine_load failed");
		delete grammar;
		return false;
	}
	if (grammar->canTalk(HTS_Engine_get_fullcontext_label_format(&_engine))) {
		_grammar = grammar;
		return true;
	}
	delete grammar;
	return false;
}

bool OpenJTalk::talk(const char* text, const char* wave, const char* log)
{
	if (_grammar == 0 || !_grammar->parse(text))
		return false;

	bool success = false;
	int n = _grammar->count();
	if (n > _grammar->minCount()) {
		char** labels = _grammar->labels();
		success = HTS_Engine_synthesize_from_strings(&_engine, labels, n);
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
				_grammar->log(fp);
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
	_grammar->reset();
	return success;
}

//------------------------------------------------------------------------
//	Grammar
//------------------------------------------------------------------------

OpenJTalk::Grammar*
OpenJTalk::Grammar::load(const char* lang, const char* dict_dir)
{
	if (strncasecmp(lang, "ja", 2) == 0)
		return JPGrammar::load(dict_dir);
	else
		return FliteGrammar::load(dict_dir);
}

//------------------------------------------------------------------------
//	Grammar - JPGrammar
//------------------------------------------------------------------------

OpenJTalk::Grammar*
JPGrammar::load(const char* dict_dir)
{
	JPGrammar* grammar = new JPGrammar();
	if (Mecab_load(&grammar->_mecab, dict_dir) != TRUE) {
		LOGD(TAG, "Mecab_load failed");
		delete grammar;
		return 0;
	}
	return grammar;
}

JPGrammar::JPGrammar()
{
	Mecab_initialize(&_mecab);
	NJD_initialize(&_njd);
	JPCommon_initialize(&_jpcommon);
}

JPGrammar::~JPGrammar()
{
	Mecab_clear(&_mecab);
	NJD_clear(&_njd);
	JPCommon_clear(&_jpcommon);
}

bool
JPGrammar::canTalk(const char* format)
{
	if (strcmp(format, "HTS_TTS_JPN") != 0) {
		LOGD(TAG, "HTS_Engine_fulltext_label_format is not 'HTS_TTS_JPN'");
		return false;
	}
	return true;
}

bool
JPGrammar::parse(const char* text)
{
	char buff[MAXBUFLEN];
	text2mecab(buff, text);
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
	return true;
}

void
JPGrammar::reset()
{
	JPCommon_refresh(&_jpcommon);
	NJD_refresh(&_njd);
	Mecab_refresh(&_mecab);
}

void
JPGrammar::log(FILE* fp)
{
	fprintf(fp, "[Text analysis result]\n");
	NJD_fprint(&_njd, fp);
}

//------------------------------------------------------------------------
//	Grammar - FliteGrammar
//------------------------------------------------------------------------

OpenJTalk::Grammar*
FliteGrammar::load(const char* dict_dir)
{
	cst_voice* voice = register_cmu_us_kal(dict_dir);
	if (voice == 0) {
		LOGD(TAG, "register_cmu_us_kal failed");
		return 0;
	}
	return new FliteGrammar(voice);
}

FliteGrammar::~FliteGrammar()
{
	reset();
	unregister_cmu_us_kal(_voice);
}

bool
FliteGrammar::parse(const char* text)
{
	reset();
	_u = flite_synth_text(text, _voice);
	if (_u == 0) {
		LOGD(TAG, "flite_synth_text failed");
		return false;
	}
	_count = 0;
	cst_item* head = relation_head(utt_relation(_u, "Segment"));
	for (cst_item* s = head; s != 0; s = item_next(s)) {
		_count++;
	}
	_labels = (char**)calloc(_count, sizeof(char*));
	int i = 0;
	for (cst_item* s = head; s != 0; s = item_next(s)) {
		_labels[i] = (char*)calloc(MAXBUFLEN, sizeof(char));
		createLabel(s, _labels[i]);
		i++;
	}
	return true;
}

void
FliteGrammar::reset()
{
	if (_labels != 0) {
		for (int i = 0; i < _count; i++) {
			if (_labels[i] != 0)
				free(_labels[i]);
		}
		_labels = 0;
	}
	_count = 0;
	if (_u != 0) {
		delete_utterance(_u);
		_u = 0;
	}
}

void
FliteGrammar::createLabel(cst_item* item, char* label)
{
   const char *p1 = ffeature_string(item, "p.p.name");
   const char *p2 = ffeature_string(item, "p.name");
   const char *p3 = ffeature_string(item, "name");
   const char *p4 = ffeature_string(item, "n.name");
   const char *p5 = ffeature_string(item, "n.n.name");

   if (strcmp(p3, "pau") == 0) {
      /* for pause */
      int a3 = ffeature_int(item, "p.R:SylStructure.parent.R:Syllable.syl_numphones");
      int c3 = ffeature_int(item, "n.R:SylStructure.parent.R:Syllable.syl_numphones");
      int d2 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Word.word_numsyls");
      int f2 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Word.word_numsyls");
      int g1 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase");
      int g2 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase");
      int i1 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase");
      int i2 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase");
      int j1, j2, j3;
      if (item_next(item) != NULL) {
         j1 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
         j2 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
         j3 = ffeature_int(item, "n.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      } else {
         j1 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
         j2 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
         j3 = ffeature_int(item, "p.R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      }
      sprintf(label, "%s^%s-%s+%s=%s@x_x/A:%s_%s_%s/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:%s+%s+%s/D:%s_%s/E:x+x@x+x&x+x#x+x/F:%s_%s/G:%s_%s/H:x=x^x=x|x/I:%s=%s/J:%d+%d-%d",     /* */
              strcmp(p1, "0") == 0 ? "x" : p1,  /* p1 */
              strcmp(p2, "0") == 0 ? "x" : p2,  /* p2 */
              p3,               /* p3 */
              strcmp(p4, "0") == 0 ? "x" : p4,  /* p4 */
              strcmp(p5, "0") == 0 ? "x" : p5,  /* p5 */
              a3 == 0 ? "x" : ffeature_string(item, "p.R:SylStructure.parent.R:Syllable.stress"),       /* a1 */
              a3 == 0 ? "x" : ffeature_string(item, "p.R:SylStructure.parent.R:Syllable.accented"),     /* a2 */
              a3 == 0 ? "x" : val_string(val_string_n(a3)),     /* a3 */
              c3 == 0 ? "x" : ffeature_string(item, "n.R:SylStructure.parent.R:Syllable.stress"),       /* c1 */
              c3 == 0 ? "x" : ffeature_string(item, "n.R:SylStructure.parent.R:Syllable.accented"),     /* c2 */
              c3 == 0 ? "x" : val_string(val_string_n(c3)),     /* c3 */
              d2 == 0 ? "x" : ffeature_string(item, "p.R:SylStructure.parent.parent.R:Word.gpos"),      /* d1 */
              d2 == 0 ? "x" : val_string(val_string_n(d2)),     /* d2 */
              f2 == 0 ? "x" : ffeature_string(item, "n.R:SylStructure.parent.parent.R:Word.gpos"),      /* f1 */
              f2 == 0 ? "x" : val_string(val_string_n(f2)),     /* f2 */
              g1 == 0 ? "x" : val_string(val_string_n(g1)),     /* g1 */
              g2 == 0 ? "x" : val_string(val_string_n(g2)),     /* g2 */
              i1 == 0 ? "x" : val_string(val_string_n(i1)),     /* i1 */
              i2 == 0 ? "x" : val_string(val_string_n(i2)),     /* i2 */
              j1,               /* j1 */
              j2,               /* j2 */
              j3);              /* j3 */
   } else {
      /* for no pause */
      int p6 = ffeature_int(item, "R:SylStructure.pos_in_syl") + 1;
      int a3 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.p.syl_numphones");
      int b3 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_numphones");
      int b4 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.pos_in_word") + 1;
      int b12 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_p_stress");
      int b13 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_n_stress");
      int b14 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_p_accent");
      int b15 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.lisp_distance_to_n_accent");
      int c3 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.n.syl_numphones");
      int d2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.p.word_numsyls");
      int e2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.word_numsyls");
      int e3 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.pos_in_phrase") + 1;
      int e7 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.lisp_distance_to_p_content");
      int e8 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.lisp_distance_to_n_content");
      int f2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Word.n.word_numsyls");
      int g1 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.p.lisp_num_syls_in_phrase");
      int g2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.p.lisp_num_words_in_phrase");
      int h2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_words_in_phrase");
      int h3 = ffeature_int(item, "R:SylStructure.parent.R:Syllable.sub_phrases") + 1;
      const char *h5 = ffeature_string(item, "R:SylStructure.parent.parent.R:Phrase.parent.daughtern.R:SylStructure.daughtern.endtone");
      int i1 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.n.lisp_num_syls_in_phrase");
      int i2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.n.lisp_num_words_in_phrase");
      int j1 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_syls");
      int j2 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_words");
      int j3 = ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_total_phrases");
      sprintf(label, "%s^%s-%s+%s=%s@%d_%d/A:%s_%s_%s/B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%s-%s;%s-%s|%s/C:%s+%s+%s/D:%s_%s/E:%s+%d@%d+%d&%d+%d#%s+%s/F:%s_%s/G:%s_%s/H:%d=%d^%d=%d|%s/I:%s=%s/J:%d+%d-%d",      /* */
              strcmp(p1, "0") == 0 ? "x" : p1,  /* p1 */
              strcmp(p2, "0") == 0 ? "x" : p2,  /* p2 */
              p3,               /* p3 */
              strcmp(p4, "0") == 0 ? "x" : p4,  /* p4 */
              strcmp(p5, "0") == 0 ? "x" : p5,  /* p5 */
              p6,               /* p6 */
              b3 - p6 + 1,      /* p7 */
              a3 == 0 ? "x" : ffeature_string(item, "R:SylStructure.parent.R:Syllable.p.stress"),       /* a1 */
              a3 == 0 ? "x" : ffeature_string(item, "R:SylStructure.parent.R:Syllable.p.accented"),     /* a2 */
              a3 == 0 ? "x" : val_string(val_string_n(a3)),     /* a3 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.stress"),    /* b1 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.accented"),  /* b2 */
              b3,               /* b3 */
              b4,               /* b4 */
              e2 - b4 + 1,      /* b5 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_in") + 1,        /* b6 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.syl_out") + 1,       /* b7 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.ssyl_in"),   /* b8 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.ssyl_out"),  /* b9 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.asyl_in"),   /* b10 */
              ffeature_int(item, "R:SylStructure.parent.R:Syllable.asyl_out"),  /* b11 */
              b12 == 0 ? "x" : val_string(val_string_n(b12)),   /* b12 */
              b13 == 0 ? "x" : val_string(val_string_n(b13)),   /* b13 */
              b14 == 0 ? "x" : val_string(val_string_n(b14)),   /* b14 */
              b15 == 0 ? "x" : val_string(val_string_n(b15)),   /* b15 */
              ffeature_string(item, "R:SylStructure.parent.R:Syllable.syl_vowel"),      /* b16 */
              c3 == 0 ? "x" : ffeature_string(item, "R:SylStructure.parent.R:Syllable.n.stress"),       /* c1 */
              c3 == 0 ? "x" : ffeature_string(item, "R:SylStructure.parent.R:Syllable.n.accented"),     /* c2 */
              c3 == 0 ? "x" : val_string(val_string_n(c3)),     /* c3 */
              d2 == 0 ? "x" : ffeature_string(item, "R:SylStructure.parent.parent.R:Word.p.gpos"),      /* d1 */
              d2 == 0 ? "x" : val_string(val_string_n(d2)),     /* d2 */
              ffeature_string(item, "R:SylStructure.parent.parent.R:Word.gpos"),        /* e1 */
              e2,               /* e2 */
              e3,               /* e3 */
              h2 - e3 + 1,      /* e4 */
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.content_words_in"),       /* e5 */
              ffeature_int(item, "R:SylStructure.parent.parent.R:Word.content_words_out"),      /* e6 */
              e7 == 0 ? "x" : val_string(val_string_n(e7)),     /* e7 */
              e8 == 0 ? "x" : val_string(val_string_n(e8)),     /* e8 */
              f2 == 0 ? "x" : ffeature_string(item, "R:SylStructure.parent.parent.R:Word.n.gpos"),      /* f1 */
              f2 == 0 ? "x" : val_string(val_string_n(f2)),     /* f2 */
              g1 == 0 ? "x" : val_string(val_string_n(g1)),     /* g1 */
              g2 == 0 ? "x" : val_string(val_string_n(g2)),     /* g2 */
              ffeature_int(item, "R:SylStructure.parent.parent.R:Phrase.parent.lisp_num_syls_in_phrase"),       /* h1 */
              h2,               /* h2 */
              h3,               /* h3 */
              j3 - h3 + 1,      /* h4 */
              strcmp(h5, "0") == 0 ? "NONE" : h5,       /* h5 */
              i1 == 0 ? "x" : val_string(val_string_n(i1)),     /* i1 */
              i2 == 0 ? "x" : val_string(val_string_n(i2)),     /* i2 */
              j1,               /* j1 */
              j2,               /* j2 */
              j3);              /* j3 */
   }
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
	jstring lang_obj, jstring dict_obj, jstring voice_obj)
{
	OpenJTalk* ojt = (OpenJTalk*)instance;
	const char* lang = env->GetStringUTFChars(lang_obj, NULL);
	const char* dict = 0;
	if (dict_obj != 0)
		dict = env->GetStringUTFChars(dict_obj, NULL);
	const char* voice = env->GetStringUTFChars(voice_obj, NULL);
	bool success = ojt->load(lang, dict, voice);
	env->ReleaseStringUTFChars(voice_obj, voice);
	if (dict_obj != 0)
		env->ReleaseStringUTFChars(dict_obj, dict);
	env->ReleaseStringUTFChars(lang_obj, lang);
	return success ? JNI_TRUE : JNI_FALSE;
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
