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

#if __USE_EDJE /**< Defined in emotion_video_player_private.h */
/**
 * Video player Edje action callbacks
 */
static void
video_obj_signal_play_cb(void *data, Evas_Object *o, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Emotion_Video_Player_Data *priv = data;

   emotion_object_play_set(priv->emotion, 1);
   edje_object_signal_emit(priv->edje, "video_state", "play");
}

static void
video_obj_signal_pause_cb(void *data, Evas_Object *o, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Emotion_Video_Player_Data *priv = data;

   emotion_object_play_set(priv->emotion, 0);
   edje_object_signal_emit(priv->edje_eo, "video_state", "pause");
}

static void
video_obj_signal_stop_cb(void *data, Evas_Object *o, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Emotion_Video_Player_Data *priv = data;

   emotion_object_play_set(priv->emotion, 0);
   emotion_object_position_set(priv->emotion, 0);
   edje_object_signal_emit(priv->edje_eo, "video_state", "stop");
}
#endif

static void EINA_UNUSED
_emotion_video_player_file_set(Eo *obj, Emotion_Video_Player_Data *priv EINA_UNUSED,
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

#if __USE_EDJE
   eo_do(priv->edje_eo, edje_object_signal_callback_add(obj, "video_control", "play", video_obj_signal_play_cb, priv));
   eo_do(priv->edje_eo, edje_object_signal_callback_add(obj, "video_control", "pause", video_obj_signal_pause_cb, priv));
   eo_do(priv->edje_eo, edje_object_signal_callback_add(obj, "video_control", "stop", video_obj_signal_stop_cb, priv));
#else
   (void)obj;
#endif

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
        emotion_object_audio_volume_set(priv->emotion, volume);
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
_emotion_video_player_constructor(Eo *obj, Emotion_Video_Player_Data *priv, const Evas_Object *evas, struct Opaque_Frame_Data *opaque_obj EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN(evas);
   EINA_SAFETY_ON_NULL_RETURN(opaque_obj);

   eo_do_super(obj, MY_CLASS, eo_constructor());
   priv->evas = evas;
   priv->last_known_position = 0.0;
#if __USE_EDJE
   priv->edje_eo = eo_add_custom(MY_CLASS, NULL, edje_object_constructor());
#endif

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
