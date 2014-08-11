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
