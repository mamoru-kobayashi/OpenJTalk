LOCAL_PATH := $(call my-dir)

#==== GLOBAL =================================================================

MY_CFLAGS := -DDEBUG
#MY_CFLAGS :=
HTS_ENGINE_DIR := $(LOCAL_PATH)/hts_engine_API-1.10
OPEN_JTALK_DIR := $(LOCAL_PATH)/open_jtalk-1.09
FLITE_DIR := $(LOCAL_PATH)/flite+hts_engine-1.06/flite

#==== HTS ENGINE API  ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := hts-api
SRC_DIR := $(HTS_ENGINE_DIR)
LOCAL_SRC_FILES := \
	$(SRC_DIR)/lib/os/HTS_audio_android.c \
	$(SRC_DIR)/lib/HTS_engine.c \
	$(SRC_DIR)/lib/HTS_gstream.c \
	$(SRC_DIR)/lib/HTS_label.c \
	$(SRC_DIR)/lib/HTS_misc.c \
	$(SRC_DIR)/lib/HTS_model.c \
	$(SRC_DIR)/lib/HTS_pstream.c \
	$(SRC_DIR)/lib/HTS_sstream.c \
	$(SRC_DIR)/lib/HTS_vocoder.c
LOCAL_C_INCLUDES := $(SRC_DIR)/include $(SRC_DIR)/lib
LOCAL_CFLAGS := -std=c99 $(MY_CFLAGS)
include $(BUILD_STATIC_LIBRARY)

#==== OpenJTalk ==============================================================

#---- text2mecab -------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := text2mecab
SRC_DIR := $(OPEN_JTALK_DIR)/text2mecab
LOCAL_SRC_FILES := \
	$(SRC_DIR)/text2mecab.c
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- mecab ------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := mecab
SRC_DIR := $(OPEN_JTALK_DIR)/mecab/src
LOCAL_SRC_FILES := \
	$(SRC_DIR)/char_property.cpp \
	$(SRC_DIR)/connector.cpp \
	$(SRC_DIR)/context_id.cpp \
	$(SRC_DIR)/dictionary.cpp \
	$(SRC_DIR)/dictionary_compiler.cpp \
	$(SRC_DIR)/dictionary_generator.cpp \
	$(SRC_DIR)/dictionary_rewriter.cpp \
	$(SRC_DIR)/eval.cpp \
	$(SRC_DIR)/feature_index.cpp \
	$(SRC_DIR)/iconv_utils.cpp \
	$(SRC_DIR)/lbfgs.cpp \
	$(SRC_DIR)/learner.cpp \
	$(SRC_DIR)/learner_tagger.cpp \
	$(SRC_DIR)/libmecab.cpp \
	$(SRC_DIR)/mecab.cpp \
	$(SRC_DIR)/nbest_generator.cpp \
	$(SRC_DIR)/param.cpp \
	$(SRC_DIR)/string_buffer.cpp \
	$(SRC_DIR)/tagger.cpp \
	$(SRC_DIR)/tokenizer.cpp \
	$(SRC_DIR)/utils.cpp \
	$(SRC_DIR)/viterbi.cpp \
	$(SRC_DIR)/writer.cpp 
LOCAL_CFLAGS := $(MY_CFLAGS) \
	-DHAVE_CONFIG_H \
	-DCHARSET_UTF_8 -DDIC_VERSION=102 \
	-DMECAB_WITHOUT_MUTEX_LOCK \
	-DMECAB_DEFAULT_RC="dummy"
include $(BUILD_STATIC_LIBRARY)

#---- mecab2njd --------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := mecab2njd
SRC_DIR := $(OPEN_JTALK_DIR)/mecab2njd
LOCAL_SRC_FILES := \
	$(SRC_DIR)/mecab2njd.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS)
include $(BUILD_STATIC_LIBRARY)

#---- njd --------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd
SRC_DIR := $(OPEN_JTALK_DIR)/njd
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd.c \
	$(SRC_DIR)/njd_node.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd-set-pronunciation --------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd-set-pronunciation
SRC_DIR := $(OPEN_JTALK_DIR)/njd_set_pronunciation
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd_set_pronunciation.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd-set-digit ----------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd-set-digit
SRC_DIR := $(OPEN_JTALK_DIR)/njd_set_digit
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd_set_digit.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd-set-accent-phrase --------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd-set-accent-phrase
SRC_DIR := $(OPEN_JTALK_DIR)/njd_set_accent_phrase
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd_set_accent_phrase.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd-set-accent-type ----------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd-set-accent-type
SRC_DIR := $(OPEN_JTALK_DIR)/njd_set_accent_type
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd_set_accent_type.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd-set-unvoiced-vowel -------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd-set-unvoiced-vowel
SRC_DIR := $(OPEN_JTALK_DIR)/njd_set_unvoiced_vowel
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd_set_unvoiced_vowel.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd-set-long-vowel -----------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd-set-long-vowel
SRC_DIR := $(OPEN_JTALK_DIR)/njd_set_long_vowel
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd_set_long_vowel.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- njd2jpcommon -----------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := njd2jpcommon
SRC_DIR := $(OPEN_JTALK_DIR)/njd2jpcommon
LOCAL_SRC_FILES := \
	$(SRC_DIR)/njd2jpcommon.c
LOCAL_C_INCLUDES := $(SRC_DIR)/../njd $(SRC_DIR)/../jpcommon
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#---- jpcommon ---------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := jpcommon
SRC_DIR := $(OPEN_JTALK_DIR)/jpcommon
LOCAL_SRC_FILES := \
	$(SRC_DIR)/jpcommon.c \
	$(SRC_DIR)/jpcommon_node.c \
	$(SRC_DIR)/jpcommon_label.c
LOCAL_CFLAGS := $(MY_CFLAGS) -DCHARSET_UTF_8
include $(BUILD_STATIC_LIBRARY)

#==== Flite ==================================================================

include $(CLEAR_VARS)
LOCAL_MODULE := flite
LANG_DIR := $(FLITE_DIR)/lang
SRC_DIR := $(FLITE_DIR)/src
LOCAL_SRC_FILES := \
        $(LANG_DIR)/cmu_us_kal/cmu_us_kal.c \
        $(LANG_DIR)/cmulex/cmu_lex.c \
        $(LANG_DIR)/cmulex/cmu_lex_data.c \
        $(LANG_DIR)/cmulex/cmu_lex_entries.c \
        $(LANG_DIR)/cmulex/cmu_lts_model.c \
        $(LANG_DIR)/cmulex/cmu_lts_rules.c \
        $(LANG_DIR)/cmulex/cmu_postlex.c \
        $(LANG_DIR)/usenglish/us_aswd.c \
        $(LANG_DIR)/usenglish/us_expand.c \
        $(LANG_DIR)/usenglish/us_ffeatures.c \
        $(LANG_DIR)/usenglish/us_gpos.c \
        $(LANG_DIR)/usenglish/us_int_accent_cart.c \
        $(LANG_DIR)/usenglish/us_int_tone_cart.c \
        $(LANG_DIR)/usenglish/us_nums_cart.c \
        $(LANG_DIR)/usenglish/us_phoneset.c \
        $(LANG_DIR)/usenglish/us_phrasing_cart.c \
        $(LANG_DIR)/usenglish/us_pos_cart.c \
        $(LANG_DIR)/usenglish/us_text.c \
        $(LANG_DIR)/usenglish/usenglish.c \
        $(SRC_DIR)/hrg/cst_ffeature.c \
        $(SRC_DIR)/hrg/cst_item.c \
        $(SRC_DIR)/hrg/cst_relation.c \
        $(SRC_DIR)/hrg/cst_utterance.c \
        $(SRC_DIR)/lexicon/cst_lexicon.c \
        $(SRC_DIR)/lexicon/cst_lts.c \
        $(SRC_DIR)/regex/cst_regex.c \
        $(SRC_DIR)/regex/regexp.c \
        $(SRC_DIR)/stats/cst_cart.c \
        $(SRC_DIR)/synth/cst_ffeatures.c \
        $(SRC_DIR)/synth/cst_phoneset.c \
        $(SRC_DIR)/synth/cst_synth.c \
        $(SRC_DIR)/synth/cst_utt_utils.c \
        $(SRC_DIR)/synth/cst_voice.c \
        $(SRC_DIR)/synth/flite.c \
        $(SRC_DIR)/utils/cst_alloc.c \
        $(SRC_DIR)/utils/cst_error.c \
        $(SRC_DIR)/utils/cst_features.c \
        $(SRC_DIR)/utils/cst_string.c \
        $(SRC_DIR)/utils/cst_tokenstream.c \
        $(SRC_DIR)/utils/cst_val.c \
        $(SRC_DIR)/utils/cst_val_const.c \
        $(SRC_DIR)/utils/cst_val_user.c
LOCAL_C_INCLUDES := \
	$(FLITE_DIR)/include \
	$(LANG_DIR)/cmulex \
	$(LANG_DIR)/usenglish
LOCAL_CFLAGS := $(MY_CFLAGS) -DFLITE_PLUS_HTS_ENGINE -DNO_UNION_INITIALIZATION
include $(BUILD_STATIC_LIBRARY)

#==== SHARED LIBRARY  ========================================================

include $(CLEAR_VARS)
LOCAL_MODULE := open-jtalk
SRC_DIR := $(LOCAL_PATH)
LOCAL_SRC_FILES := \
	$(SRC_DIR)/OpenJTalk.cc
LOCAL_C_INCLUDES := \
	$(HTS_ENGINE_DIR)/include \
	$(HTS_ENGINE_DIR)/lib \
	$(OPEN_JTALK_DIR)/text2mecab \
	$(OPEN_JTALK_DIR)/mecab/src \
	$(OPEN_JTALK_DIR)/mecab2njd \
	$(OPEN_JTALK_DIR)/njd \
	$(OPEN_JTALK_DIR)/njd_set_pronunciation \
	$(OPEN_JTALK_DIR)/njd_set_digit \
	$(OPEN_JTALK_DIR)/njd_set_accent_phrase \
	$(OPEN_JTALK_DIR)/njd_set_accent_type \
	$(OPEN_JTALK_DIR)/njd_set_unvoiced_vowel \
	$(OPEN_JTALK_DIR)/njd_set_long_vowel \
	$(OPEN_JTALK_DIR)/njd2jpcommon \
	$(OPEN_JTALK_DIR)/jpcommon \
	$(FLITE_DIR)/include
LOCAL_CFLAGS := $(MY_CFLAGS) -DFLITE_PLUS_HTS_ENGINE -DNO_UNION_INITIALIZATION
LOCAL_STATIC_LIBRARIES := hts-api \
	text2mecab mecab mecab2njd njd njd-set-pronunciation njd-set-digit \
	njd-set-accent-phrase njd-set-accent-type njd-set-unvoiced-vowel \
	njd-set-long-vowel njd2jpcommon \
	jpcommon \
	flite
LOCAL_LDLIBS := -llog -landroid -lOpenSLES
include $(BUILD_SHARED_LIBRARY)
