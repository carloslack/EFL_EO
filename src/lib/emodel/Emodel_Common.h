#ifndef _EMODEL_COMMON_H
#define _EMODEL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// XXX/TODO/FIXME: Remove this enum (and possibly other data) from here
// as soon as eolian translates these data types in .eo's.
enum _Emodel_Load_Status
  {
     EMODEL_LOAD_STATUS_ERROR = -1,
     EMODEL_LOAD_STATUS_FETCHING,
     EMODEL_LOAD_STATUS_FETCHED,
     EMODEL_LOAD_STATUS_LOADING,
     EMODEL_LOAD_STATUS_LOADED,
     EMODEL_LOAD_STATUS_UNLOADING,
     EMODEL_LOAD_STATUS_UNLOADED
  };

typedef enum _Emodel_Load_Status Emodel_Load_Status;

struct _Emodel_Property_Pair
{
   Eina_Value value; /**< the property value */
   const char *property; /**< the property name */
};
typedef struct _Emodel_Property_Pair Emodel_Property_Pair;

struct _Emodel_Property_EVT
{
   Eina_List *changed_properties; /**< the property value */
   Eina_Array *invalidated_properties; /**< the property name */
};

typedef struct _Emodel_Property_EVT Emodel_Property_EVT;

struct _Emodel_Children_EVT
{
   Eo *child; /**< child, for child_add */
   int idx; /**< child index */
};

typedef struct _Emodel_Children_EVT Emodel_Children_EVT;


#ifdef __cplusplus
}
#endif
#endif
