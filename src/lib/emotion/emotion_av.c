#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Emotion.h>
#include <Eina.h>
#include <Eo.h>
#include <emotion_av.h>

#include "emotion_av_private.h"

#define MY_CLASS EMOTION_AV_CLASS
#define MY_CLASS_NAME "Emotion_AV"

static void EINA_UNUSED
_emotion_av_file_set(Eo *obj EINA_UNUSED, Emotion_AV_Data *priv EINA_UNUSED, const char *filename EINA_UNUSED)
{
}

static void EINA_UNUSED
_emotion_av_position_set(Eo *obj EINA_UNUSED, Emotion_AV_Data *priv EINA_UNUSED, double position EINA_UNUSED)
{
}

static void EINA_UNUSED
_emotion_av_volume_set(Eo *obj EINA_UNUSED, Emotion_AV_Data *priv EINA_UNUSED, double volume EINA_UNUSED)
{
}

static void EINA_UNUSED
_emotion_av_play_set(Eo *obj EINA_UNUSED, Emotion_AV_Data *priv EINA_UNUSED, Eina_Bool play EINA_UNUSED)
{
}

/**
 * Class definitions
 */
static void EINA_UNUSED
_emotion_av_eo_base_constructor(Eo *obj, Emotion_AV_Data *priv EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
}

static void EINA_UNUSED
_emotion_av_eo_base_destructor(Eo *obj, Emotion_AV_Data *priv EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_destructor());
}
