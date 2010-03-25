#include "private.h"

typedef struct _Elm_Params_Check
{
   Elm_Params base;
   Evas_Object *icon;
   int state;
} Elm_Params_Check;

static void
external_check_state_set(void *data __UNUSED__, Evas_Object *obj, const void *from_params, const void *to_params, float pos __UNUSED__)
{
   const Elm_Params_Check *p1 = from_params, *p2 = to_params;

   p1 = from_params;
   p2 = to_params;

   if (!p2)
     {
	elm_check_label_set(obj, p1->base.label);
	elm_check_icon_set(obj, p1->icon);
	elm_check_state_set(obj, p1->state);
	return;
     }

   elm_check_label_set(obj, p2->base.label);
   elm_check_icon_set(obj, p2->icon);
   elm_check_state_set(obj, p2->state);
}

static Eina_Bool
external_check_param_set(void *data __UNUSED__, Evas_Object *obj, const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     elm_check_label_set(obj, param->s);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     Evas_Object *icon = external_common_param_icon_get(obj, param);
	     if (icon)
	       {
		  elm_check_icon_set(obj, icon);
		  return EINA_TRUE;
	       }
	  }
     }
   else if (!strcmp(param->name, "state"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     elm_check_state_set(obj, param->i);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static Eina_Bool
external_check_param_get(void *data __UNUSED__, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
	  {
	     param->s = elm_check_label_get(obj);
	     return EINA_TRUE;
	  }
     }
   else if (!strcmp(param->name, "icon"))
     {
	/* not easy to get icon name back from live object */
	return EINA_FALSE;
     }
   else if (!strcmp(param->name, "state"))
     {
	if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	  {
	     param->i = elm_check_state_get(obj);
	     return EINA_TRUE;
	  }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

static void *
external_check_params_parse(void *data, Evas_Object *obj, const Eina_List *params)
{
   Elm_Params_Check *mem;
   Edje_External_Param *param;

   mem = external_common_params_parse(Elm_Params_Check, data, obj, params);
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   param = edje_external_param_find(params, "state");
   if (param)
     mem->state = param->i;

   return mem;
}

static void
external_check_params_free(void *params)
{
   Elm_Params_Check *mem = params;

   if (mem->icon)
     evas_object_del(mem->icon);
   external_common_params_free(params);
}

static Edje_External_Param_Info external_check_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL_FULL("state", 0, "unchecked", "checked"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(check, "check")
DEFINE_EXTERNAL_TYPE_SIMPLE(check, "Check")
