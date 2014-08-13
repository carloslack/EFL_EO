#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Emotion.h>
#include <Eina.h>
#include <Evas.h>
#include <Eo.h>
#include <emotion_audio_player.h>

#include "emotion_audio_player_private.h"

#define MY_CLASS EMOTION_AUDIO_PLAYER_CLASS
#define MY_CLASS_NAME "Emotion_Audio_Player"

static void
_emotion_audio_player_file_set(Eo *obj EINA_UNUSED, Emotion_Audio_Player_Data *priv EINA_UNUSED, const char *filepath EINA_UNUSED, const char *backend EINA_UNUSED)
{
}

static void
_emotion_audio_player_position_set(Eo *obj EINA_UNUSED, Emotion_Audio_Player_Data *priv EINA_UNUSED,double position EINA_UNUSED)
{
}

static void
_emotion_audio_player_volume_set(Eo *obj EINA_UNUSED, Emotion_Audio_Player_Data *priv EINA_UNUSED,double volume EINA_UNUSED)
{
}

static void
_emotion_audio_player_play_set(Eo *obj EINA_UNUSED, Emotion_Audio_Player_Data *priv EINA_UNUSED,Eina_Bool play EINA_UNUSED)
{
}

static void EINA_UNUSED
_emotion_audio_player_eo_base_constructor(Eo *obj, Emotion_Audio_Player_Data *priv EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
}

static void EINA_UNUSED
_emotion_audio_player_constructor(Eo *obj, Emotion_Audio_Player_Data *priv EINA_UNUSED,
                                  const Evas_Object *evas EINA_UNUSED, struct Opaque_Frame_Audio_Player_Data *opaque_obj EINA_UNUSED)
{
   _emotion_audio_player_eo_base_constructor(obj, priv);
}

static void EINA_UNUSED
_emotion_audio_player_eo_base_destructor(Eo *obj, Emotion_Audio_Player_Data *priv EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_destructor());
}

#include "emotion_audio_player.eo.c"
