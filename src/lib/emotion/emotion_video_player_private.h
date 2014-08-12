#ifndef _EIO_MODEL_PRIVATE_H
#define _EIO_MODEL_PRIVATE_H

typedef struct _Emotion_Video_Player_Data Emotion_Video_Player_Data;

struct _Emotion_Video_Player_Data
{
   Eo *obj;
   const Evas_Object *evas;
   Evas_Object *emotion;
   char *filepath;
   char *module_filename;
   double position;
   double volume;
};

#endif
