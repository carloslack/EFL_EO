//Compile with:
// gcc -o emodel_test_file emodel_test_file.c `pkg-config --cflags --libs emodel`
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eo.h>
#include <Eio.h>
#include <Ecore.h>
#include <Emodel.h>
#include <eio_model.h>
#include <stdio.h>

#include <check.h>

#define EMODEL_TEST_FILENAME_PATH "/tmp"
#define EMODEL_MAX_TEST_CHILDS 16

/**
 * The following test works however
 * it is going to rename (move) the original directory to
 * new one so '/tmp' as root dir doesn't work , you'll need to use
 * '/tmp/some_other_dir' as root instead.
 */
//#define _RUN_LOCAL_TEST

struct reqs_t {
   /* property change */
   int changed_is_dir;
   int changed_is_lnk;
   int changed_size;
   int changed_mtime;
   int changed_icon;

   /* properties list */
   int proplist_filename;
   int proplist_path;
   int proplist_icon;
   int proplist_mtime;
   int proplist_is_dir;
   int proplist_is_lnk;
   int proplist_size;

   /* misc tests for data or propeties */
   int properties;
   int children;
   int child_add;
   int child_del;
};

static struct reqs_t reqs;
static Ecore_Event_Handler *handler;

static Eina_Bool
 exit_func(void *data EINA_UNUSED, int ev_type EINA_UNUSED, void *ev)
 {
    Ecore_Event_Signal_Exit *e;

    e = (Ecore_Event_Signal_Exit *)ev;
    if (e->interrupt)      fprintf(stdout, "Exit: interrupt\n");
    else if (e->quit)      fprintf(stdout, "Exit: quit\n");
    else if (e->terminate) fprintf(stdout, "Exit: terminate\n");
    ecore_main_loop_quit();
    return ECORE_CALLBACK_CANCEL;
 }

static Eina_Bool
_load_slice_cb(void *data, Eo *obj, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Eina_Value value_prop;
   static int children_n = 0;
   int *children_total = data;
   Emodel_Load_Status *st = event_info;

   if (*st == EMODEL_LOAD_STATUS_FETCHED)
     {
         children_n++;
         eo_do(obj, emodel_property_get("filename", &value_prop));
         printf("emodel_fetched filename %s\n", eina_value_to_string(&value_prop));
         if (children_n == *children_total)
           {
                ecore_main_loop_quit();
           }
     }

   return EINA_TRUE;
}

static Eina_Bool
_load_status_cb(void *data EINA_UNUSED, Eo *obj, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   Eina_Value value_prop;
   static int children_n = 0;
   static Eo *filemodel = NULL;
   Emodel_Load_Status *st = event_info;

   if (*st == EMODEL_LOAD_STATUS_LOADED)
     {
         const Eina_List *children_list;
         Eina_List *l;
         Eo *child;
         size_t total;

         filemodel = obj;
         printf("Model is Loaded\n");
         eo_do(obj, emodel_property_get("filename", &value_prop));
         printf("emodel_loaded filename %s\n", eina_value_to_string(&value_prop));
         eo_do(obj, emodel_property_get("size", &value_prop));
         printf("emodel_loaded size %s\n", eina_value_to_string(&value_prop));
         eo_do(obj, emodel_property_get("mtime", &value_prop));
         printf("emodel_loaded mtime %s\n", eina_value_to_string(&value_prop));

         eo_do(obj, emodel_children_count_get(&total));
         printf("emodel_test count %d\n", (int)total);

         eo_do(obj, emodel_children_get(&children_list));
         printf("emodel_test children_list_size %d\n", eina_list_count(children_list));

         EINA_LIST_FOREACH((Eina_List *)children_list, l, child)
           if (child)
             eo_do(child, eo_event_callback_add(EMODEL_EVENT_LOAD_STATUS, _load_status_cb, NULL));

     }

   if (*st == EMODEL_LOAD_STATUS_FETCHED)
     {
         children_n++;
         eo_do(obj, emodel_property_get("filename", &value_prop));
         printf("emodel_fetched %d = filename %s\n", (int)children_n, eina_value_to_string(&value_prop));
         if (children_n == reqs.children)
           {
                const Eina_List *children_l;
                Eina_List *l, *cl;
                Eo *child;

                if (filemodel)
                  {
                      eo_do(filemodel, emodel_children_slice_get(5, 5, &children_l));
                      cl = eina_list_clone(children_l);
                      children_n = eina_list_count(cl);
                      printf("emodel_test children_list_size %d\n", children_n);
                      if (children_n)
                      ecore_main_loop_quit();

                      EINA_LIST_FOREACH(cl, l, child)
                        if (child) {
                          eo_ref(child);
                          eo_do(child, eo_event_callback_add(EMODEL_EVENT_LOAD_STATUS, _load_slice_cb, &children_n));
                        }
                  }

           }
     }

   return EINA_TRUE;
}

static Eina_Bool
_properties_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   const Emodel_Property_EVT *evt = (Emodel_Property_EVT *)event_info;
   Emodel_Property_Pair *pair = NULL;
   Eina_List *l = NULL;

   EINA_LIST_FOREACH(evt->changed_properties, l, pair)
     {
        fprintf(stdout, "Received changed property=%s, value=%s\n",
                pair->property, eina_value_to_string(&pair->value));
        if(!strcmp(pair->property, "is_dir"))
          reqs.changed_is_dir = 1;
        if(!strcmp(pair->property, "is_lnk"))
          reqs.changed_is_lnk = 1;
        if(!strcmp(pair->property, "size"))
          reqs.changed_size = 1;
        if(!strcmp(pair->property, "mtime"))
          reqs.changed_mtime = 1;
        if(!strcmp(pair->property, "icon"))
          reqs.changed_icon = 1;
     }

   reqs.properties = 1;
   return EINA_TRUE;
}

static Eina_Bool
_children_count_cb(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, const Eo_Event_Description *desc EINA_UNUSED, void *event_info)
{
   size_t *len = event_info;
   size_t total;

   fprintf(stdout, "Children count number=%lu\n", *len);
   reqs.children = *len;

   eo_do(obj, emodel_children_count_get(&total));
   fprintf(stdout, "New total children count number=%lu\n", *len);

   return EINA_TRUE;
}

START_TEST(emodel_test_test_file)
{
   Eo *filemodel = NULL;
   Eina_Value value_prop;
   const Eina_List *properties_list;
   Eina_List *l;
   char *str;
   int i;

   memset(&reqs, -1, sizeof(struct reqs_t));

   fail_if(!eina_init(), "ERROR: Cannot init Eina!\n");
   fail_if(!ecore_init(), "ERROR: Cannot init Ecore!\n");
   fail_if(!eio_init(), "ERROR: Cannot init EIO!\n");

   filemodel = eo_add_custom(EIO_MODEL_CLASS, NULL, eio_model_constructor(EMODEL_TEST_FILENAME_PATH));
   fail_if(!filemodel, "ERROR: Cannot init model!\n");

   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_LOAD_STATUS, _load_status_cb, NULL));
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_PROPERTIES_CHANGED, _properties_cb, NULL));
   eo_do(filemodel, eo_event_callback_add(EMODEL_EVENT_CHILDREN_COUNT_CHANGED, _children_count_cb, NULL));

   eo_do(filemodel, emodel_load());

   handler = ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_func, NULL);

   eo_do(filemodel, emodel_property_get("filename", &value_prop));
   printf("emodel_test filename %s\n", eina_value_to_string(&value_prop));

   i = 0;
   eo_do(filemodel, emodel_properties_list_get(&properties_list));
   EINA_LIST_FOREACH((Eina_List *)properties_list, l, str)
      {
        fprintf(stdout, "Returned property list %d: %s\n", i++, str);
        if(!strcmp(str, "filename"))
          reqs.proplist_filename = 1;
        else if(!strcmp(str, "path"))
          reqs.proplist_path = 1;
        else if(!strcmp(str, "icon"))
          reqs.proplist_icon = 1;
        else if(!strcmp(str, "mtime"))
          reqs.proplist_mtime = 1;
        else if(!strcmp(str, "is_dir"))
          reqs.proplist_is_dir = 1;
        else if(!strcmp(str, "is_lnk"))
          reqs.proplist_is_lnk = 1;
        else if(!strcmp(str, "size"))
          reqs.proplist_size = 1;
      }

   ecore_main_loop_begin();

#ifdef _RUN_LOCAL_TEST
   Eina_Value *nameset = eina_value_new(EINA_VALUE_TYPE_STRING);
   eina_value_set(nameset, "/tmp/emodel_test");
   eo_do(filemodel, emodel_property_set("path", nameset));
   eo_do(filemodel, emodel_property_get("path", &value_prop));
#endif
   sleep(1); /**< EIO is asynchrounous so I must give some time for deletions to execute */

   eo_unref(filemodel);
   ecore_shutdown();
   eina_shutdown();
   eio_shutdown();
}
END_TEST

void
emodel_test_file(TCase *tc)
{
   tcase_add_test(tc, emodel_test_test_file);
}

