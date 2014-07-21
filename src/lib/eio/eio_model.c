#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Emodel.h>
#include <Eina.h>
#include <eio_model.h>
#include <Eio.h>
#include <Ecore.h>
#include <Eo.h>

#include "eio_model_private.h"

#define MY_CLASS EIO_MODEL_CLASS
#define MY_CLASS_NAME "Eio_Model"

static Eina_Value_Struct_Desc *EIO_MODEL_PROPERTIES_DESC = NULL;
static void _eio_prop_set_error_cb(void *, Eio_File *, int);

static void
_set_load_status(Eio_Model_Data *priv, Emodel_Load_Status stat)
{
   if (priv->load_status < stat)
     {
         priv->load_status = stat;
         eo_do(priv->obj, eo_event_callback_call(EMODEL_EVENT_LOAD_STATUS, &stat));
     }
}

static void
_stat_pro_set(Eio_Model_Data *priv, int prop_id, void *pvalue, Emodel_Property_EVT *evt)
{
   Emodel_Property_Pair *pair;
   const char *prop = EIO_MODEL_PROPERTIES_DESC->members[prop_id].name;
   Eina_Bool check = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN(priv);
   EINA_SAFETY_ON_NULL_RETURN(priv->properties);
   EINA_SAFETY_ON_NULL_RETURN(evt);

   pair = calloc(1, sizeof(Emodel_Property_Pair));
   EINA_SAFETY_ON_NULL_RETURN(pair);
   pair->property = prop;

   switch(prop_id)
     {
        //Eina_Bool
       case EIO_MODEL_PROP_IS_DIR:
       case EIO_MODEL_PROP_IS_LNK:
             {
                Eina_Bool *val = (Eina_Bool*)pvalue, cvalue;
                check = eina_value_struct_get(priv->properties, prop, &cvalue);
                if((check == EINA_TRUE) && (cvalue == *val)) return;
                eina_value_struct_set(priv->properties, pair->property, *val);
                eina_value_struct_value_get(priv->properties, pair->property, &pair->value);
             }
           break;
        //double
       case EIO_MODEL_PROP_MTIME:
             {
                double *val = (double*)pvalue, cvalue;
                if((check == EINA_TRUE) && (cvalue == *val)) return;
                eina_value_struct_set(priv->properties, pair->property, *val);
                eina_value_struct_value_get(priv->properties, pair->property, &pair->value);
             }
           break;
        //long long
       case EIO_MODEL_PROP_SIZE:
             {
                long long *val = (long long*)pvalue, cvalue;
                if((check == EINA_TRUE) && (cvalue == *val)) return;
                eina_value_struct_set(priv->properties, pair->property, *val);
                eina_value_struct_value_get(priv->properties, pair->property, &pair->value);
             }
           break;
     }

   evt->changed_properties = eina_list_append(evt->changed_properties, pair);
}

/**
 *  Callbacks
 *  Property
 */
static void
_eio_stat_done_cb(void *data, Eio_File *handler EINA_UNUSED, const Eina_Stat *stat)
{
   Emodel_Property_EVT evt;
   Eio_Model_Data *priv = data;

   EINA_SAFETY_ON_FALSE_RETURN(eo_ref_get(priv->obj));

   priv->stat = stat;

   memset(&evt, 0, sizeof(Emodel_Property_EVT));

   Eina_Bool data_bool = eio_file_is_dir(stat);
   _stat_pro_set(priv, EIO_MODEL_PROP_IS_DIR, (void*)&data_bool, &evt);
   data_bool = eio_file_is_lnk(stat);
   _stat_pro_set(priv, EIO_MODEL_PROP_IS_LNK, &data_bool, &evt);

   double data_double = eio_file_mtime(stat);
   _stat_pro_set(priv, EIO_MODEL_PROP_MTIME, &data_double, &evt);

   long long data_long_long = eio_file_size(stat);
   _stat_pro_set(priv, EIO_MODEL_PROP_SIZE, &data_long_long, &evt);

   if (evt.changed_properties != NULL)
     {
         Emodel_Property_Pair *pair;
         eo_do(priv->obj, eo_event_callback_call(EMODEL_EVENT_PROPERTIES_CHANGED, &evt));
         EINA_LIST_FREE(evt.changed_properties, pair)
         {
             eina_value_free(&pair->value);
             free(pair);
         }
     }

   priv->properties_loaded = EINA_TRUE;
   if (priv->children_loaded == EINA_TRUE)
     _set_load_status(priv, EMODEL_LOAD_STATUS_LOADED);
}

static void
_eio_progress_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const Eio_Progress *info EINA_UNUSED)
{
   //TODO: implement
}

static void
_eio_move_done_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Emodel_Property_EVT evt;
   Emodel_Property_Pair pair_path, pair_filename;
   Eio_Model_Data *priv = data;
   Eina_Value_Struct_Desc *desc = EIO_MODEL_PROPERTIES_DESC;

   EINA_SAFETY_ON_FALSE_RETURN(eo_ref_get(priv->obj));

   memset(&evt, 0, sizeof(Emodel_Property_EVT));

   /**
    * When mv is executed we update our values and
    * notify both path and filename properties listeners.
    */
   pair_path.property = desc->members[EIO_MODEL_PROP_PATH].name;
   eina_value_struct_set(priv->properties, pair_path.property, priv->path);
   eina_value_struct_value_get(priv->properties, pair_path.property, &pair_path.value);
   evt.changed_properties = eina_list_append(evt.changed_properties, &pair_path);

   pair_filename.property = desc->members[EIO_MODEL_PROP_FILENAME].name;
   eina_value_struct_set(priv->properties, pair_filename.property, basename(priv->path));
   eina_value_struct_value_get(priv->properties, pair_filename.property, &pair_filename.value);
   evt.changed_properties = eina_list_append(evt.changed_properties, &pair_filename);


   eo_do(priv->obj, eo_event_callback_call(EMODEL_EVENT_PROPERTIES_CHANGED, &evt));
   eina_list_free(evt.changed_properties);
}

static void
_eio_error_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error)
{
   if(error != 0)
     {
        fprintf(stderr, "Error: %s : %d: %s\n", __FUNCTION__, error, strerror(errno));
        EINA_SAFETY_ON_FALSE_RETURN(EINA_FALSE); /**< force check error only to be more verbose */
     }
   fprintf(stdout, "%s : %d\n", __FUNCTION__, error);
}

static void
_eio_prop_set_error_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error EINA_UNUSED)
{
   fprintf(stdout, "%s : %d\n", __FUNCTION__, error);
}

#if 0
/**
 *   Callbacks
 *   Child Add
 */
static void
_eio_done_mkdir_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_Model_Child_Add *_data = (Eio_Model_Child_Add *)data;
   Eo *parent = _data->priv->obj;

   EINA_SAFETY_ON_FALSE_RETURN(eo_ref_get(parent));
   /* save child object in userdata, callback can ignore this field */

   _data->child = eo_add_custom(MY_CLASS, NULL, eio_model_constructor(_data->fullpath));
   eo_do(_data->child, eio_model_children_filter_set(_data->priv->filter_cb, _data->priv->filter_userdata)); //XXX: set parent filter to child
   /* dispatch callback to user */
   _data->callback(_data->name, parent, _data->child, 0);
   _emodel_dealloc_memory(_data->fullpath, _data->name, _data, NULL);
}

static void
_eio_done_error_mkdir_cb(void *data, Eio_File *handler EINA_UNUSED, int error)
{
   if(0 != error)
     {
        fprintf(stderr, "%s: err=%d\n", __FUNCTION__, error);
        Eio_Model_Child_Add *_data = (Eio_Model_Child_Add *)data;
        Eo *parent = _data->priv->obj;

        EINA_SAFETY_ON_FALSE_RETURN(eo_ref_get(parent));
        /* save child object in userdata, callback can ignore this field */
        _data->callback(_data->name, parent, NULL, error);
        _emodel_dealloc_memory(_data->fullpath, _data->name, _data, NULL);
     }
}
#endif

/**
 *  Callbacks
 *  Ecore Events
 */
static Eina_Bool
 _emodel_evt_added_ecore_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Eio_Monitor_Event *evt = (Eio_Monitor_Event*)event;
   Eio_Model_Data *priv = data;
   Emodel_Children_EVT cevt;

   cevt.child = eo_add_custom(MY_CLASS, NULL, eio_model_constructor(evt->filename));
   cevt.idx = -1;

   return eo_do(priv->obj, eo_event_callback_call(EMODEL_EVENT_CHILD_ADDED, &cevt));
}

static Eina_Bool
_emodel_evt_deleted_ecore_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   //Eio_Monitor_Event *evt = (Eio_Monitor_Event*)event;
   Eio_Model_Data *priv = data;
   Emodel_Children_EVT cevt;

   cevt.child = NULL;
   cevt.idx = -1;

   return eo_do(priv->obj, eo_event_callback_call(EMODEL_EVENT_CHILD_REMOVED, &cevt));
}

static void
_eio_monitors_list_load(Eio_Model_Data *priv)
{
   priv->mon.mon_event_child_add[0] = EIO_MONITOR_DIRECTORY_CREATED;
   priv->mon.mon_event_child_add[1] = EIO_MONITOR_FILE_CREATED;
   priv->mon.mon_event_child_add[2] = EIO_MONITOR_ERROR;
   priv->mon.mon_event_child_del[0] = EIO_MONITOR_DIRECTORY_DELETED;
   priv->mon.mon_event_child_del[1] = EIO_MONITOR_FILE_DELETED;
   priv->mon.mon_event_child_del[2] = EIO_MONITOR_ERROR;
}

Eina_Bool
_eio_monitor_evt_added_cb(void *data EINA_UNUSED, Eo *obj, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Eio_Model_Data *priv = eo_data_scope_get(obj, MY_CLASS);
   const Eo_Callback_Array_Item *callback_array = event_info;
   unsigned int i;

   if((callback_array->desc != EMODEL_EVENT_CHILD_ADDED) && (callback_array->desc != EMODEL_EVENT_CHILD_REMOVED))
     return EO_CALLBACK_CONTINUE;

   if((priv->cb_count_child_add == 0) && (priv->cb_count_child_del == 0))
     {
        Eio_Monitor *_mon  = eio_monitor_add(priv->path);
        if(!_mon) return EO_CALLBACK_CONTINUE;
        priv->monitor = _mon;
     }

   if(callback_array->desc == EMODEL_EVENT_CHILD_ADDED)
     {
        for(i = 0; priv->mon.mon_event_child_add[i] != EIO_MONITOR_ERROR ; ++i)
          {
             priv->mon.ecore_child_add_handler[i] =
                 ecore_event_handler_add(priv->mon.mon_event_child_add[i], _emodel_evt_added_ecore_cb, priv);
          }
        priv->cb_count_child_add++;
     }
  else if(callback_array->desc == EMODEL_EVENT_CHILD_REMOVED)
     {
        for(i = 0; priv->mon.mon_event_child_del[i] != EIO_MONITOR_ERROR ; ++i)
          {
             priv->mon.ecore_child_add_handler[i] =
                 ecore_event_handler_add(priv->mon.mon_event_child_del[i], _emodel_evt_deleted_ecore_cb, priv);
          }
        priv->cb_count_child_del++;
     }

    return EO_CALLBACK_CONTINUE;
}

Eina_Bool
_eio_monitor_evt_deleted_cb(void *data EINA_UNUSED, Eo *obj, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
    Eio_Model_Data *priv = eo_data_scope_get(obj, MY_CLASS);
    const Eo_Callback_Array_Item *callback_array = event_info;
    unsigned int i;

    if((callback_array->desc != EMODEL_EVENT_CHILD_ADDED) && (callback_array->desc != EMODEL_EVENT_CHILD_REMOVED))
         return EO_CALLBACK_CONTINUE;

    if(!priv->monitor)
        return EO_CALLBACK_CONTINUE;

    if(callback_array->desc == EMODEL_EVENT_CHILD_ADDED)
      {
         if(priv->cb_count_child_add > 0)
           {
              for(i = 0; priv->mon.mon_event_child_add[i] != EIO_MONITOR_ERROR ; ++i)
                {
                   ecore_event_handler_del(priv->mon.ecore_child_add_handler[i]);
                }
              priv->cb_count_child_add--;
           }
      }
    else if(callback_array->desc == EMODEL_EVENT_CHILD_REMOVED)
      {
         if(priv->cb_count_child_del > 0)
           {
              for(i = 0; priv->mon.mon_event_child_del[i] != EIO_MONITOR_ERROR ; ++i)
                {
                   ecore_event_handler_del(priv->mon.ecore_child_del_handler[i]);
                }
              priv->cb_count_child_del--;
           }
      }

    if((priv->cb_count_child_add == 0) && (priv->cb_count_child_del == 0))
      {
         eio_monitor_del(priv->monitor);
      }

    return EO_CALLBACK_CONTINUE;
}

/*
 *  Callbacks
 *  Children Get
 */
static void
_eio_main_children_fetch_cb(void *data, Eio_File *handler EINA_UNUSED, const Eina_File_Direct_Info *info)
{
   Eio_Model_Children_Data *cdata = data;
   Eo *child;

   EINA_SAFETY_ON_NULL_RETURN(cdata);
   EINA_SAFETY_ON_NULL_RETURN(cdata->priv->obj);

   child = eina_list_nth(cdata->children_list, cdata->cidx);
   if (child)
     {
         Eina_Value *path = eina_value_new(EINA_VALUE_TYPE_STRING);
         eina_value_set(path, info->path);
         eo_do(child, emodel_property_set("path", path),
                    eio_model_children_filter_set(cdata->priv->filter_cb, cdata->priv->filter_userdata));
         eina_value_free(path);
         cdata->cidx++;
     }
}

 static Eina_Bool
_eio_filter_children_fetch_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   Eio_Model_Children_Data *cdata = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(cdata, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cdata->priv, EINA_FALSE);

   if (cdata->priv->filter_cb)
     {
         return cdata->priv->filter_cb(cdata->priv->filter_userdata, handler, info);
     }

   return EINA_TRUE;
}

static void
_eio_done_children_fetch_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_Model_Children_Data *cdata = data;
   _emodel_dealloc_memory(cdata, NULL);
}

static void
_eio_error_children_fetch_cb(void *data, Eio_File *handler EINA_UNUSED, int error)
{

   if(0 != error)
     {
        Eio_Model_Children_Data *cdata = data;
        fprintf(stderr, "%s: err=%d\n", __FUNCTION__, error);
        eina_list_free(cdata->children_list);
        _emodel_dealloc_memory(cdata, NULL);
     }
}

/**
 *  Callbacks
 *  Children Slice Get
 */
static Eina_Bool
_eio_filter_children_slice_get_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   Eina_Bool ret = EINA_FALSE;
   Eio_Model_Children_Data *cdata = data;

   if (_eio_filter_children_fetch_cb(data, handler, info))
     {
        if(cdata->idx >= cdata->start && cdata->idx < cdata->count)
          {
             ret = EINA_TRUE;
          }

        if(cdata->idx == cdata->count)
          {
             eio_file_cancel(cdata->lsref);
          }

        cdata->idx++;
     }

   return ret;
}

static void
_eio_done_children_slice_get_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_Model_Children_Data *cdata = data;
   eina_list_free(cdata->children_list);
   _emodel_dealloc_memory(cdata, NULL);
}

/**
 *  Callbacks
 *  Children Count Get
 */

static void
_eio_main_children_count_get_cb(void *data, Eio_File *handler EINA_UNUSED, const Eina_File_Direct_Info *info EINA_UNUSED)
{
   Eio_Model_Children_Count *count_data = (Eio_Model_Children_Count *)data;
   EINA_SAFETY_ON_NULL_RETURN(count_data);
   count_data->total++;
}

static void
_eio_done_children_count_get_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_Model_Children_Count *count_data = (Eio_Model_Children_Count *)data;
   EINA_SAFETY_ON_NULL_RETURN(count_data);
   EINA_SAFETY_ON_FALSE_RETURN(eo_ref_get(count_data->priv->obj));

   if (count_data->priv->children_count != count_data->total)
     {
        count_data->priv->children_count = count_data->total;

        /* Should we really pass a size_t poiter? Maybe a struct instead? */
        eo_do(count_data->priv->obj, eo_event_callback_call(EMODEL_EVENT_CHILDREN_COUNT_CHANGED, &(count_data->total)));
     }

   count_data->priv->children_loaded = EINA_TRUE;
   if (count_data->priv->properties_loaded == EINA_TRUE)
     _set_load_status(count_data->priv, EMODEL_LOAD_STATUS_LOADED);

   _emodel_dealloc_memory(count_data, NULL);
}

static void
_eio_error_children_count_get_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error EINA_UNUSED)
{
   fprintf(stdout, "%s : %d\n", __FUNCTION__, error);
}

/**
 *  Callbacks
 *  Child Del
 */
static Eina_Bool
_eio_filter_child_del_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const Eina_File_Direct_Info *info EINA_UNUSED)
{
   return EINA_TRUE;
}

static void
_eio_progress_child_del_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const Eio_Progress *info EINA_UNUSED)
{}

static Eina_Bool
_null_cb(void *data, Eo *obj, const Eo_Event_Description *desc, void *event_info)
{
   (void) desc;
   (void) obj;
   (void) data;
   (void) event_info;
   return EO_CALLBACK_CONTINUE;
}

static void
_eio_done_unlink_cb(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_Model_Data *priv = (Eio_Model_Data *)data;

   EINA_SAFETY_ON_NULL_RETURN(priv);
   EINA_SAFETY_ON_NULL_RETURN(priv->obj);

   /** use dummy callback */
   /**
    * We generate these events here because _eio_monitor_evt_added_cb and
    * _eio_monitor_evt_deleted_cb can add or delete, respectively,
    * I/O event handlers for directories.
    */
   eo_do(priv->obj, eo_event_callback_add(EMODEL_EVENT_CHILD_ADDED, _null_cb, NULL));
   eo_do(priv->obj, eo_event_callback_del(EMODEL_EVENT_CHILD_ADDED, _null_cb, NULL));
   eo_do(priv->obj, eo_event_callback_add(EMODEL_EVENT_CHILD_REMOVED, _null_cb, NULL));
   eo_do(priv->obj, eo_event_callback_del(EMODEL_EVENT_CHILD_REMOVED, _null_cb, NULL));
}

static void
_eio_error_unlink_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error)
{
   fprintf(stdout, "%s : %d\n", __FUNCTION__, error);
}


/**
 * Interfaces impl.
 */
static Eina_Bool
_eio_model_emodel_properties_list_get(Eo *obj EINA_UNUSED,
                                       Eio_Model_Data *_pd, const Eina_List **properties_list)
{
   Eio_Model_Data *priv = _pd;
   unsigned int i;

   EINA_SAFETY_ON_NULL_RETURN_VAL(priv, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(priv->obj, EINA_FALSE);

   if(priv->properties_list == NULL)
     {
        Eina_Value_Struct_Desc *desc = EIO_MODEL_PROPERTIES_DESC;
        for(i = 0; i < desc->member_count; ++i)
          priv->properties_list = eina_list_append(priv->properties_list, desc->members[i].name);
     }

   *properties_list = priv->properties_list;

   return EINA_TRUE;
}

/**
 * Property Fetch
 */
static Eina_Bool
_eio_model_emodel_property_get(Eo *obj EINA_UNUSED, Eio_Model_Data *priv, const char *property, Eina_Value *value)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(property, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(priv, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(priv->obj, EINA_FALSE);

   return eina_value_struct_value_get(priv->properties, property, value);
}

/**
 * Property Set
 */
static Eina_Bool
_eio_model_emodel_property_set(Eo *obj, Eio_Model_Data *priv, const char *property, const Eina_Value *value)
{
   char *dest;

   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(property, EINA_FALSE);

   if (strcmp(property, "path") != 0)
     return EINA_FALSE;

   dest = strdup(eina_value_to_string(value));
   if (priv->path == NULL)
     {
         priv->path = dest;

         eina_value_struct_set(priv->properties, "path", priv->path);
         eina_value_struct_set(priv->properties, "filename", basename(priv->path));

         _eio_monitors_list_load(priv);
         eo_do(obj, eo_event_callback_add(EO_EV_CALLBACK_ADD, _eio_monitor_evt_added_cb, NULL));
         eo_do(obj, eo_event_callback_add(EO_EV_CALLBACK_DEL, _eio_monitor_evt_deleted_cb, NULL));

         _eio_move_done_cb(priv, NULL);
         _set_load_status(priv, EMODEL_LOAD_STATUS_FETCHED);
         return EINA_TRUE;
     }

   priv->file = eio_file_move(priv->path, dest, _eio_progress_cb, _eio_move_done_cb, _eio_prop_set_error_cb, priv);
   _emodel_dealloc_memory(priv->path, NULL);
   priv->path = dest;
   return EINA_TRUE;
}
/**
 * Children Count Get
 */
static Eina_Bool
_eio_model_emodel_children_count_get(Eo *obj EINA_UNUSED, Eio_Model_Data *priv, size_t *children_count)
{
   Eio_Model_Children_Count *count_data = calloc(1, sizeof(Eio_Model_Children_Count));
   EINA_SAFETY_ON_NULL_RETURN_VAL(count_data, EINA_FALSE);
   count_data->priv = priv;

   eio_file_direct_ls(priv->path, _eio_filter_children_fetch_cb,
                      _eio_main_children_count_get_cb, _eio_done_children_count_get_cb,
                      _eio_error_children_count_get_cb, count_data);

   *children_count = priv->children_count;
   return EINA_TRUE;
}

/**
 * Properties Load
 */
 static void
_eio_model_emodel_properties_load(Eo *obj EINA_UNUSED, Eio_Model_Data *priv)
{
   if (!priv->file && priv->load_status != EMODEL_LOAD_STATUS_LOADED)
     {
         priv->file = eio_file_direct_stat(priv->path, _eio_stat_done_cb, _eio_error_cb, priv);
     }
}

/**
 * Children Load
 */
 static void
_eio_model_emodel_children_load(Eo *obj, Eio_Model_Data *priv)
{
   if (priv->file && priv->load_status != EMODEL_LOAD_STATUS_LOADED)
     {
         size_t children_count;
         _eio_model_emodel_children_count_get(obj, priv, &children_count);
     }
}

/**
 * Load
 */
 static void
_eio_model_emodel_load(Eo *obj, Eio_Model_Data *priv)
{
   if (priv->load_status != EMODEL_LOAD_STATUS_LOADED)
     {
         _set_load_status(priv, EMODEL_LOAD_STATUS_LOADING);
         _eio_model_emodel_properties_load(obj, priv);
         _eio_model_emodel_children_load(obj, priv);
     }
}

/**
 * Load status get
 */
static Emodel_Load_Status
_eio_model_emodel_load_status_get(Eo *obj EINA_UNUSED, Eio_Model_Data *priv)
{
   return priv->status_load;
}

/**
 * Unload
 */
static void
_eio_model_emodel_unload(Eo *obj  EINA_UNUSED, Eio_Model_Data *priv)
{
   if (priv->load_status != EMODEL_LOAD_STATUS_UNLOADED)
     {
         Eo *child;
         EINA_LIST_FREE(priv->children_list, child)
           eo_unref(child);

         _set_load_status(priv, EMODEL_LOAD_STATUS_UNLOADING);
     }
}

static void
_eio_model_children_filter_set(Eo *obj EINA_UNUSED, Eio_Model_Data *priv, Eio_Filter_Direct_Cb filter_cb, void *data)
{
   priv->filter_cb = filter_cb;
   priv->filter_userdata = data;
}

/**
 * Child Add
 */
static Eo *
_eio_model_emodel_child_add(Eo *obj EINA_UNUSED, Eio_Model_Data *priv EINA_UNUSED)
{
   return eo_add(EIO_MODEL_CLASS, obj);
}

/**
 * Child Remove
 */
static Eina_Bool
_eio_model_emodel_child_remove(Eo *obj EINA_UNUSED, Eio_Model_Data *priv, Eo *child)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(child, EINA_FALSE);

   if (priv->children_list != NULL)
     {
         priv->children_list = eina_list_remove(priv->children_list, child);
     }

   Eio_Model_Data *child_priv = eo_data_scope_get(child, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(child_priv, EINA_FALSE);

   eio_dir_unlink(child_priv->path,
                  _eio_filter_child_del_cb,
                  _eio_progress_child_del_cb,
                  _eio_done_unlink_cb,
                  _eio_error_unlink_cb,
                  child_priv);
   return EINA_TRUE;
}

/**
 * Children Get
 */
static Eina_Bool
_eio_model_emodel_children_get(Eo *obj EINA_UNUSED, Eio_Model_Data *priv, const Eina_List **children_list)
{

   if (priv->children_list == NULL)
     {
         Eo *child;
         unsigned int pcount;

         Eio_Model_Children_Data *cdata = calloc(1, sizeof(Eio_Model_Children_Data));
         EINA_SAFETY_ON_NULL_RETURN_VAL(cdata, EINA_FALSE);

         cdata->priv = priv;
         cdata->start = 0;
         cdata->count = 0;
         cdata->idx = 0;
         cdata->cidx = 0;

         for (pcount = 0; pcount < priv->children_count; ++pcount)
           {
               child = eo_add(EIO_MODEL_CLASS, obj);
               priv->children_list = eina_list_append(priv->children_list, child);
           }

         cdata->children_list = priv->children_list;
         EINA_SAFETY_ON_FALSE_RETURN_VAL(eo_ref_get(cdata->priv->obj), EINA_FALSE);
         cdata->lsref = eio_file_direct_ls(cdata->priv->path, _eio_filter_children_fetch_cb,
                              _eio_main_children_fetch_cb, _eio_done_children_fetch_cb, _eio_error_children_fetch_cb, cdata);
     }

   *children_list = priv->children_list;

   return EINA_TRUE;
}

/**
 * Children Slice Get
 */
static Eina_Bool
_eio_model_emodel_children_slice_get(Eo *obj EINA_UNUSED, Eio_Model_Data *priv,
                                        size_t start, size_t count, const Eina_List **children_list)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(count > 0, EINA_FALSE);
   Eo *child;
   unsigned int pcount;

   Eio_Model_Children_Data *cdata = calloc(1, sizeof(Eio_Model_Children_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(cdata, EINA_FALSE);

   cdata->priv = priv;
   cdata->start = start;
   cdata->count = start + count;
   cdata->idx = 0;
   cdata->cidx = 0;

   if (cdata->priv->children_count < cdata->count)
     {
        count -= cdata->count - cdata->priv->children_count;
        cdata->count = cdata->priv->children_count;
        if (count == 0)
          {
               free(cdata);
               *children_list = NULL;
               return EINA_TRUE;
          }
     }

   for (pcount = 0; pcount < count; ++pcount)
     {
         child = eo_add(EIO_MODEL_CLASS, NULL);
         cdata->children_list = eina_list_append(cdata->children_list, child);
     }

   EINA_SAFETY_ON_FALSE_RETURN_VAL(eo_ref_get(cdata->priv->obj), EINA_FALSE);
   cdata->lsref = eio_file_direct_ls(cdata->priv->path,
                      _eio_filter_children_slice_get_cb,
                      _eio_main_children_fetch_cb,
                      _eio_done_children_slice_get_cb,
                      _eio_error_children_fetch_cb,
                      cdata);
   *children_list = cdata->children_list;
   return EINA_TRUE;
}

static void
_struct_properties_init()
{
   typedef struct _This_Eio_Properties
     {
        const char *filename;
        const char *path;
        const char *icon;
        double mtime;
        int is_dir;
        int is_lnk;
        int size;
     } This_Eio_Properties;

   static Eina_Value_Struct_Member prop_members[] = {
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, filename),
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, path),
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, icon), //XXX
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, mtime),
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, is_dir),
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, is_lnk),
     EINA_VALUE_STRUCT_MEMBER(NULL, This_Eio_Properties, size)
   };
   //XXX: Check data types
   prop_members[EIO_MODEL_PROP_FILENAME].type = EINA_VALUE_TYPE_STRING;
   prop_members[EIO_MODEL_PROP_PATH].type = EINA_VALUE_TYPE_STRING;
   prop_members[EIO_MODEL_PROP_ICON].type = EINA_VALUE_TYPE_STRING;
   prop_members[EIO_MODEL_PROP_MTIME].type = EINA_VALUE_TYPE_TIMEVAL;
   prop_members[EIO_MODEL_PROP_IS_DIR].type = EINA_VALUE_TYPE_INT;
   prop_members[EIO_MODEL_PROP_IS_LNK].type = EINA_VALUE_TYPE_INT;
   prop_members[EIO_MODEL_PROP_SIZE].type = EINA_VALUE_TYPE_INT64;

   static Eina_Value_Struct_Desc prop_desc = {
     EINA_VALUE_STRUCT_DESC_VERSION,
     NULL, // no special operations
     prop_members,
     EINA_C_ARRAY_LENGTH(prop_members),
     sizeof(This_Eio_Properties)
   };
   EIO_MODEL_PROPERTIES_DESC = &prop_desc;
}

/**
 * Class definitions
 */
static void
_eio_model_eo_base_constructor(Eo *obj, Eio_Model_Data *priv)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   priv->obj = obj;
   _struct_properties_init();
   priv->properties = eina_value_struct_new(EIO_MODEL_PROPERTIES_DESC);
   EINA_SAFETY_ON_NULL_RETURN(priv->properties);

   priv->children_count = 0;
   priv->load_status = EMODEL_LOAD_STATUS_FETCHING;
}

static void
_eio_model_constructor(Eo *obj , Eio_Model_Data *priv, const char *path)
{
   _eio_model_eo_base_constructor(obj, priv);
   priv->path = strdup(path);

   eina_value_struct_set(priv->properties, "path", priv->path);
   eina_value_struct_set(priv->properties, "filename", basename(priv->path));

   _eio_monitors_list_load(priv);
   eo_do(obj, eo_event_callback_add(EO_EV_CALLBACK_ADD, _eio_monitor_evt_added_cb, NULL));
   eo_do(obj, eo_event_callback_add(EO_EV_CALLBACK_DEL, _eio_monitor_evt_deleted_cb, NULL));
   priv->load_status = EMODEL_LOAD_STATUS_FETCHED;
}

static void
_eio_model_eo_base_destructor(Eo *obj , Eio_Model_Data *priv)
{
   Eo *child;
   if(priv && priv->monitor)
     {
        eio_monitor_del(priv->monitor);
     }

   eina_list_free(priv->properties_list);
   eina_value_free(priv->properties);
   EINA_LIST_FREE(priv->children_list, child)
     eo_unref(child);

   eo_do_super(obj, MY_CLASS, eo_destructor());
}

#include "eio_model.eo.c"
