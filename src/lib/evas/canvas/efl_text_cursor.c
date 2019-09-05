#include "evas_common_private.h"
#include "evas_private.h"

#include "efl_canvas_text_internal.h"

#define MY_CLASS EFL2_TEXT_CURSOR_CLASS

struct _Efl2_Text_Cursor_Data
{
   Efl2_Canvas_Text *obj;
   Efl2_Text_Cursor_Handle *cur;
};

typedef struct _Efl2_Text_Cursor_Data Efl2_Text_Cursor_Data;

EOLIAN static void
_efl2_text_cursor_position_set(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, int pos)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   _canvas_text_cursor_position_set(cur, pos);
}

EOLIAN static int
_efl2_text_cursor_position_get(const Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   // FIXME: Probably don't need this as we are only changing text nodes. Add this back if we do (+ we removed more) ASYNC_BLOCK;
   // FIXME: Check everywhere that cursor is not null and return an error if yes? Missing here.. Mostly other places missing errors.
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_position_get(cur);
}

EOLIAN static Eina_Unicode
_efl2_text_cursor_content_get(const Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_content_get(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_geometry_get(const Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor_Type ctype, int *cx, int *cy, int *cw, int *ch, int *cx2, int *cy2, int *cw2, int *ch2)
{
   // FIXME
   (void) pd;
   (void) ctype;
   (void) cx;
   (void) cy;
   (void) cw;
   (void) ch;
   (void) cx2;
   (void) cy2;
   (void) cw2;
   (void) ch2;
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl2_text_cursor_equal(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *obj2)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   Efl2_Text_Cursor_Data *pd2 = efl_data_scope_get(obj2, MY_CLASS);
   Efl2_Text_Cursor_Handle *cur2 = pd2->cur;
   return _canvas_text_cursor_equal(cur, cur2);
}

EOLIAN static int
_efl2_text_cursor_compare(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *obj2)
{
   Efl2_Text_Cursor_Handle *cur1 = pd->cur;
   Efl2_Text_Cursor_Data *pd2 = efl_data_scope_get(obj2, MY_CLASS);
   Efl2_Text_Cursor_Handle *cur2 = pd2->cur;

   return _canvas_text_cursor_compare(cur1, cur2);
}

EOLIAN static void
_efl2_text_cursor_copy(Eo *obj, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *dst)
{
   Efl2_Text_Cursor_Handle *cur_src = pd->cur;
   Efl2_Text_Cursor_Data *pd_dst = efl_data_scope_get(dst, MY_CLASS);
   Efl2_Text_Cursor_Handle *cur_dest = pd_dst->cur;
   if (!cur_src || !cur_dest) return;
   if (!_efl2_text_cursor_equal(obj, pd, dst))
     {
        _canvas_text_cursor_copy(cur_dest, cur_src);
        // FIXME: Trigger callbacks about this being changed here and everywhere that changes it
     }
}

EOLIAN static Eina_Bool
_efl2_text_cursor_char_next(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_next(cur, EINA_FALSE);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_char_prev(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_prev(cur, EINA_FALSE);
}

EOLIAN static void
_efl2_text_cursor_char_delete(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_char_delete(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_cluster_start(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   // FIXME
   (void) pd;
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl2_text_cursor_cluster_end(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   // FIXME
   (void) pd;
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_efl2_text_cursor_paragraph_start(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_paragraph_start(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_paragraph_end(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_paragraph_end(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_word_start(Eo *eo_obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_word_start(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_word_end(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_word_end(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_line_start(Eo *eo_obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_line_start(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_line_end(Eo *eo_obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_line_end(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_paragraph_first(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_paragraph_first(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_paragraph_last(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_paragraph_last(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_paragraph_next(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_paragraph_next(cur);
}

EOLIAN static Eina_Bool
_efl2_text_cursor_paragraph_prev(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_paragraph_prev(cur);
}

EOLIAN static void
_efl2_text_cursor_line_number_set(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, int line_number)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   _canvas_text_cursor_line_number_set(cur, line_number);
}

EOLIAN static int
_efl2_text_cursor_line_number_get(const Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   Efl2_Text_Cursor_Handle *cur = pd->cur;
   return _canvas_text_cursor_line_number_get(cur);
}

EOLIAN static int
_efl2_text_cursor_line_jump_by(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, int by)
{
   return -1;
}

EOLIAN static void
_efl2_text_cursor_char_coord_set(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, int x, int y)
{
}

EOLIAN static int
_efl2_text_cursor_text_insert(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, const char *text)
{
   // FIXME: bring back async block to here and all the ones that change the text nodes
   Efl2_Text_Cursor_Handle *cur = pd->cur;

   return _canvas_text_cursor_text_insert(cur, text);
}

EOLIAN static char *
_efl2_text_cursor_range_text_get(const Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *obj2)
{
   Efl2_Text_Cursor_Handle *cur1 = pd->cur;
   Efl2_Text_Cursor_Data *pd2 = efl_data_scope_get(obj2, MY_CLASS);
   Efl2_Text_Cursor_Handle *cur2 = pd2->cur;

   return _canvas_text_cursor_text_plain_get(cur1, cur2);
}

EOLIAN static Eina_Iterator *
_efl2_text_cursor_range_geometry_get(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *cur2)
{
}

EOLIAN static Eina_Iterator *
_efl2_text_cursor_range_precise_geometry_get(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *cur2)
{
}

EOLIAN static void
_efl2_text_cursor_range_delete(Eo *eo_obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor *obj2)
{
   Efl2_Text_Cursor_Handle *cur1 = pd->cur;
   Efl2_Text_Cursor_Data *pd2 = efl_data_scope_get(obj2, MY_CLASS);
   Efl2_Text_Cursor_Handle *cur2 = pd2->cur;

   _canvas_text_cursor_range_delete(cur1, cur2);
}

EOLIAN static void
_efl2_text_cursor_handle_set(Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd, Efl2_Text_Cursor_Handle *handle)
{
   // FIXME: What about refcouting?
   pd->cur = handle;
}

EOLIAN static Efl2_Text_Cursor_Handle *
_efl2_text_cursor_handle_get(const Eo *obj EINA_UNUSED, Efl2_Text_Cursor_Data *pd)
{
   return pd->cur;
}

EOLIAN static void
_efl2_text_cursor_handle_ref(Efl2_Text_Cursor_Handle *handle)
{
   handle->ref++;
}

EOLIAN static void
_efl2_text_cursor_handle_unref(Efl2_Text_Cursor_Handle *handle)
{
   handle->ref--;
   // FIXME: Actually do something
}

#include "canvas/efl2_text_cursor.eo.c"
