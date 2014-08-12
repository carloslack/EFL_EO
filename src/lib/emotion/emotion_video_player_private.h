#ifndef _EMOTION_VIDEO_PLAYER_PRIVATE_H
#define _EMOTION_VIDEO_PLAYER_PRIVATE_H

typedef struct _Emotion_Video_Player_Data Emotion_Video_Player_Data;

struct _Emotion_Video_Player_Data
{
   Eo *obj;
   const Evas_Object *evas;
   Evas_Object *emotion;
   char *filepath;
   char *module_filename;
   double position;
   double last_known_position;
   double last_known_volume;
};

#endif
