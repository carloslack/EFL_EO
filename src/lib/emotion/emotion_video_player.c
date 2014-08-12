#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Emotion.h>
#include <Eina.h>
#include <Evas.h>
#include <Eo.h>
#include <emotion_video_player.h>

#include "emotion_video_player_private.h"

#define MY_CLASS EMOTION_VIDEO_PLAYER_CLASS
#define MY_CLASS_NAME "Emotion_Video_Player"

static void EINA_UNUSED
_emotion_video_player_file_set(Eo *obj EINA_UNUSED, Emotion_Video_Player_Data *priv EINA_UNUSED,
                               const char *filepath, const char *module_filename)
{
   EINA_SAFETY_ON_NULL_RETURN(filepath);
   EINA_SAFETY_ON_NULL_RETURN(module_filename);
   EINA_SAFETY_ON_NULL_RETURN(priv->evas);

   priv->filepath = strdup(filepath);
   priv->module_filename = strdup(module_filename);
   priv->emotion = emotion_object_add((Evas_Object*)priv->evas);
   EINA_SAFETY_ON_FALSE_RETURN(emotion_object_init(priv->emotion, priv->module_filename));
   EINA_SAFETY_ON_FALSE_RETURN(emotion_object_file_set(priv->emotion, priv->filepath));
}

static void EINA_UNUSED
_emotion_video_player_position_set(Eo *obj EINA_UNUSED, Emotion_Video_Player_Data *priv, double position)
{
   EINA_SAFETY_ON_NULL_RETURN(priv->emotion);
   if(position != priv->last_known_position)
     {
        emotion_object_position_set(priv->emotion, position);
        priv->last_known_position = position;
     }
}

static void EINA_UNUSED
_emotion_video_player_volume_set(Eo *obj EINA_UNUSED, Emotion_Video_Player_Data *priv EINA_UNUSED, double volume)
{
   EINA_SAFETY_ON_NULL_RETURN(priv->emotion);

   if(volume != priv->last_known_volume)
     {
        emotion_object_audio_volume_set(priv->emotion, volume);  /**< XXX:Do it through emotion_audio_player? */
        priv->last_known_volume = volume;
     }
}

static void EINA_UNUSED
_emotion_video_player_play_set(Eo *obj EINA_UNUSED, Emotion_Video_Player_Data *priv EINA_UNUSED, Eina_Bool play)
{
   EINA_SAFETY_ON_NULL_RETURN(priv->emotion);

   if(play != emotion_object_play_get(priv->emotion))
     {
        emotion_object_play_set(priv->emotion, play);
     }
}

/**
 * Class definitions
 */
static void EINA_UNUSED
_emotion_video_player_eo_base_constructor(Eo *obj, Emotion_Video_Player_Data *priv)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   priv->obj = obj;
}

static void EINA_UNUSED
_emotion_video_player_constructor(Eo *obj, Emotion_Video_Player_Data *priv, const Evas_Object *evas)
{
   EINA_SAFETY_ON_NULL_RETURN(evas);

   eo_do_super(obj, MY_CLASS, eo_constructor());
   priv->evas = evas;
   priv->last_known_position = 0.0;
}

static void EINA_UNUSED
_emotion_video_player_eo_base_destructor(Eo *obj, Emotion_Video_Player_Data *priv EINA_UNUSED)
{
   if(priv->filepath)
     free(priv->filepath);
   if(priv->module_filename)
     free(priv->module_filename);

   eo_do_super(obj, MY_CLASS, eo_destructor());
}

#include "emotion_video_player.eo.c"
