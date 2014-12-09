/**
 * @internal
 * @subsection Evas_Object_Textblock2_Internal Internal Textblock2 Object Tutorial
 *
 * This explains the internal design of the Evas Textblock2 Object, it's assumed
 * that the reader of this section has already read @ref Evas_Object_Textblock2_Tutorial "Textblock2's usage docs.".
 *
 * @subsection textblock2_internal_intro Introduction
 * There are two main parts to the textblock2 object, the first being the node
 * system, and the second being the layout system. The former is just an
 * internal representation of the markup text, while the latter is the internal
 * visual representation of the text (i.e positioning, sizing, fonts and etc).
 *
 * @subsection textblock2_nodes The Nodes system
 * The nodes mechanism consists of two main data types:
 * ::Evas_Object_Textblock2_Node_Text and ::Evas_Object_Textblock2_Node_Format
 * the former is for Text nodes and the latter is for format nodes.
 * There's always at least one text node, even if there are only formats.
 *
 * @subsection textblock2_nodes_text Text nodes
 * Each text node is essentially a paragraph, it includes an @ref Eina_UStrbuf
 * that stores the actual paragraph text, a utf8 string to store the paragraph
 * text in utf8 (which is not used internally at all), A pointer to it's
 * main @ref textblock2_nodes_format_internal "Format Node" and the paragraph's
 * @ref evas_bidi_props "BiDi properties". The pointer to the format node may be
 * NULL if there's no format node anywhere before the end of the text node,
 * not even in previous text nodes. If not NULL, it points to the first format
 * node pointing to text inside of the text node, or if there is none, it points
 * to the previous's text nodes format node. Each paragraph has a format node
 * representing a paragraph separator pointing to it's last position except
 * for the last paragraph, which has no such constraint. This constraint
 * happens because text nodes are paragraphs and paragraphs are delimited by
 * paragraph separators.
 *
 * @subsection textblock2_nodes_format_internal Format Nodes - Internal
 * Each format node stores a group of format information, for example the
 * markup: \<font=Vera,Kochi font_size=10 align=left\> will all be inserted
 * inside the same format node, although it consists of different formatting
 * commands.
 * Each node has a pointer to it's text node, this pointer is NEVER NULL, even
 * if there's only one format, and no text, a text node is created. Each format
 * node includes an offset from the last format node of the same text node. For
 * example, the markup "0<b>12</b>" will create two format nodes, the first
 * having an offset of 1 and the second an offset of 2. Each format node also
 * includes a @ref Eina_Strbuf that includes the textual representation of the
 * format, and a boolean stating if the format is a visible format or not, see
 * @ref textblock2_nodes_format_visible
 *
 * @subsection textblock2_nodes_format_visible Visible Format Nodes
 * There are two types of format nodes, visible and invisible. They are the same
 * in every way, except for the representation in the text node. While invisible
 * format nodes have no representation in the text node, the visible ones do.
 * The Uniceode object replacement character (0xFFFC) is inserted to every place
 * a visible format node points to. This makes it very easy to treat visible
 * formats as items in the text, both for BiDi purposes and cursor handling
 * purposes.
 * Here are a few example visible an invisible formats:
 * Visible: newline char, tab, paragraph separator and an embedded item.
 * Invisible: setting the color, font or alignment of the text.
 *
 * @subsection textblock2_layout The layout system
 * @todo write @ref textblock2_layout
 */
#include "evas_common_private.h"
#include "evas_private.h"

//#define LYDBG(f, args...) printf(f, ##args)
#define LYDBG(f, args...)

#define MY_CLASS EVAS_TEXTBLOCK2_CLASS

#define MY_CLASS_NAME "Evas_Textblock2"

#include "linebreak.h"
#include "wordbreak.h"

/* save typing */
#define ENFN obj->layer->evas->engine.func
#define ENDT obj->layer->evas->engine.data.output

/* private magic number for textblock2 objects */
static const char o_type[] = "textblock2";

/* The char to be inserted instead of visible formats */
#define _REPLACEMENT_CHAR 0xFFFC
#define _PARAGRAPH_SEPARATOR 0x2029
#define _NEWLINE '\n'
#define _TAB '\t'

#define _REPLACEMENT_CHAR_UTF8 "\xEF\xBF\xBC"
#define _PARAGRAPH_SEPARATOR_UTF8 "\xE2\x80\xA9"
#define _NEWLINE_UTF8 "\n"
#define _TAB_UTF8 "\t"
#define EVAS_TEXTBLOCK2_IS_VISIBLE_FORMAT_CHAR(ch) \
   (((ch) == _REPLACEMENT_CHAR) || \
    ((ch) ==  _NEWLINE) || \
    ((ch) == _TAB) || \
    ((ch) == _PARAGRAPH_SEPARATOR))

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(EINA_LOG_DOMAIN_DEFAULT, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(EINA_LOG_DOMAIN_DEFAULT, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(EINA_LOG_DOMAIN_DEFAULT, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(EINA_LOG_DOMAIN_DEFAULT, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(EINA_LOG_DOMAIN_DEFAULT, __VA_ARGS__)

#define TB_NULL_CHECK(null_check, ...) \
   do \
     { \
        if (!null_check) \
          { \
             ERR("%s is NULL while it shouldn't be, please notify developers.", #null_check); \
             return __VA_ARGS__; \
          } \
     } \
   while(0)

/* private struct for textblock2 object internal data */
/**
 * @internal
 * @typedef Evas_Textblock2_Data
 * The actual textblock2 object.
 */
typedef struct _Evas_Object_Textblock2             Evas_Textblock2_Data;
/**
 * @internal
 * @typedef Evas_Object_Style_Tag
 * The structure used for finding style tags.
 */
typedef struct _Evas_Object_Style_Tag             Evas_Object_Style_Tag;
/**
 * @internal
 * @typedef Evas_Object_Style_Tag
 * The structure used for finding style tags.
 */
typedef struct _Evas_Object_Style_Tag_Base        Evas_Object_Style_Tag_Base;
/**
 * @internal
 * @typedef Evas_Object_Textblock2_Node_Text
 * A text node.
 */
typedef struct _Evas_Object_Textblock2_Node_Text   Evas_Object_Textblock2_Node_Text;

/**
 * @internal
 * @typedef Evas_Object_Textblock2_Paragraph
 * A layouting paragraph.
 */
typedef struct _Evas_Object_Textblock2_Paragraph   Evas_Object_Textblock2_Paragraph;
/**
 * @internal
 * @typedef Evas_Object_Textblock2_Line
 * A layouting line.
 */
typedef struct _Evas_Object_Textblock2_Line        Evas_Object_Textblock2_Line;
/**
 * @internal
 * @typedef Evas_Object_Textblock2_Item
 * A layouting item.
 */
typedef struct _Evas_Object_Textblock2_Item        Evas_Object_Textblock2_Item;
/**
 * @internal
 * @typedef Evas_Object_Textblock2_Item
 * A layouting text item.
 */
typedef struct _Evas_Object_Textblock2_Text_Item        Evas_Object_Textblock2_Text_Item;
/**
 * @internal
 * @typedef Evas_Object_Textblock2_Format_Item
 * A layouting format item.
 */
typedef struct _Evas_Object_Textblock2_Format_Item Evas_Object_Textblock2_Format_Item;
/**
 * @internal
 * @typedef Evas_Object_Textblock2_Format
 * A textblock2 format.
 */
typedef struct _Evas_Object_Textblock2_Format      Evas_Object_Textblock2_Format;
/**
 * @internal
 * @typedef Evas_Textblock2_Selection_Iterator
 * A textblock2 selection iterator.
 */
typedef struct _Evas_Textblock2_Selection_Iterator Evas_Textblock2_Selection_Iterator;
/**
 * @internal
 * @def IS_AT_END(ti, ind)
 * Return true if ind is at the end of the text item, false otherwise.
 */
#define IS_AT_END(ti, ind) (ind == ti->text_props.text_len)

/**
 * @internal
 * @def MOVE_PREV_UNTIL(limit, ind)
 * This decrements ind as long as ind > limit.
 */
#define MOVE_PREV_UNTIL(limit, ind) \
   do \
     { \
        if ((limit) < (ind)) \
           (ind)--; \
     } \
   while (0)

/**
 * @internal
 * @def MOVE_NEXT_UNTIL(limit, ind)
 * This increments ind as long as ind < limit
 */
#define MOVE_NEXT_UNTIL(limit, ind) \
   do \
     { \
        if ((ind) < (limit)) \
           (ind)++; \
     } \
   while (0)

/**
 * @internal
 * @def GET_ITEM_LEN(it)
 * Returns length of item (Format or Text)
 */
#define GET_ITEM_LEN(it) \
   (((it)->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ? \
    _ITEM_TEXT(it)->text_props.text_len : 1)

/**
 * @internal
 * @def GET_ITEM_TEXT(ti)
 * Returns a const reference to the text of the ti (not null terminated).
 */
#define GET_ITEM_TEXT(ti) \
   (((ti)->parent.text_node) ? \
    (eina_ustrbuf_string_get((ti)->parent.text_node->unicode) + \
      (ti)->parent.text_pos) : EINA_UNICODE_EMPTY_STRING)
/**
 * @internal
 * @def _FORMAT_IS_CLOSER_OF(base, closer, closer_len)
 * Returns true if closer is the closer of base.
 */
#define _FORMAT_IS_CLOSER_OF(base, closer, closer_len) \
   (!strncmp(base, closer, closer_len) && \
    (!base[closer_len] || \
     (base[closer_len] == '=') || \
     _is_white(base[closer_len])))

struct _Evas_Object_Style_Tag_Base
{
   char *tag;  /**< Format Identifier: b=Bold, i=Italic etc. */
   char *replace;  /**< Replacement string. "font_weight=Bold", "font_style=Italic" etc. */
   size_t tag_len;  /**< Strlen of tag. */
   size_t replace_len;  /**< Strlen of replace. */
};

struct _Evas_Object_Style_Tag
{
   EINA_INLIST;
   Evas_Object_Style_Tag_Base tag;  /**< Base style object for holding style information. */
};

struct _Evas_Object_Textblock2_Node_Text
{
   EINA_INLIST;
   Eina_UStrbuf                       *unicode;  /**< Actual paragraph text. */
   char                               *utf8;  /**< Text in utf8 format. */
   Evas_Object_Textblock2_Paragraph    *par;  /**< Points to the paragraph node of which this node is a part. */
   Eina_Bool                           dirty : 1;  /**< EINA_TRUE if already handled/format changed, else EINA_FALSE. */
   Eina_Bool                           is_new : 1;  /**< EINA_TRUE if its a new paragraph, else EINA_FALSE. */
};

#define ANCHOR_NONE 0
#define ANCHOR_A 1
#define ANCHOR_ITEM 2

/**
 * @internal
 * @def _NODE_TEXT(x)
 * A convinience macro for casting to a text node.
 */
#define _NODE_TEXT(x)  ((Evas_Object_Textblock2_Node_Text *) (x))
/**
 * @internal
 * @def _ITEM(x)
 * A convinience macro for casting to a generic item.
 */
#define _ITEM(x)  ((Evas_Object_Textblock2_Item *) (x))
/**
 * @internal
 * @def _ITEM_TEXT(x)
 * A convinience macro for casting to a text item.
 */
#define _ITEM_TEXT(x)  ((Evas_Object_Textblock2_Text_Item *) (x))
/**
 * @internal
 * @def _ITEM_FORMAT(x)
 * A convinience macro for casting to a format item.
 */
#define _ITEM_FORMAT(x)  ((Evas_Object_Textblock2_Format_Item *) (x))

struct _Evas_Object_Textblock2_Paragraph
{
   EINA_INLIST;
   Evas_Object_Textblock2_Line        *lines;  /**< Points to the first line of this paragraph. */
   Evas_Object_Textblock2_Node_Text   *text_node;  /**< Points to the first text node of this paragraph. */
   Eina_List                         *logical_items;  /**< Logical items are the properties of this paragraph, like width, height etc. */
   Evas_BiDi_Paragraph_Props         *bidi_props; /**< Only valid during layout. */
   Evas_BiDi_Direction                direction;  /**< Bidi direction enum value. The display direction like right to left.*/
   Evas_Coord                         y, w, h;  /**< Text block co-ordinates. y co-ord, width and height. */
   int                                line_no;  /**< Line no of the text block. */
   Eina_Bool                          is_bidi : 1;  /**< EINA_TRUE if this is BiDi Paragraph, else EINA_FALSE. */
   Eina_Bool                          visible : 1;  /**< EINA_TRUE if paragraph visible, else EINA_FALSE. */
};

struct _Evas_Object_Textblock2_Line
{
   EINA_INLIST;
   Evas_Object_Textblock2_Item        *items;  /**< Pointer to layouting text item. Contains actual text and information about its display. */
   Evas_Object_Textblock2_Paragraph   *par;  /**< Points to the paragraph of which this line is a part. */
   Evas_Coord                         x, y, w, h;  /**< Text block line co-ordinates. */
   int                                baseline;  /**< Baseline of the textblock2. */
   int                                line_no;  /**< Line no of this line. */
};

typedef enum _Evas_Textblock2_Item_Type
{
   EVAS_TEXTBLOCK2_ITEM_TEXT,
   EVAS_TEXTBLOCK2_ITEM_FORMAT,
} Evas_Textblock2_Item_Type;

struct _Evas_Object_Textblock2_Item
{
   EINA_INLIST;
   Evas_Object_Textblock2_Node_Text     *text_node;  /**< Pointer to textblock2 node text. It contains actual text in unicode and utf8 format. */
   Evas_Object_Textblock2_Format        *format;  /**< Pointer to textblock2 format. It contains all the formatting information for this text block. */
   Evas_Object_Textblock2_Line          *ln;  /**< Pointer to textblock2 line. It contains the co-ord, baseline, and line no for this item. */
   size_t                               text_pos;  /**< Position of this item in textblock2 line. */
#ifdef BIDI_SUPPORT
   size_t                               visual_pos;  /**< Visual position of this item. */
#endif
   Evas_Textblock2_Item_Type             type;  /**< EVAS_TEXTBLOCK2_ITEM_TEXT or EVAS_TEXTBLOCK2_ITEM_FORMAT */

   Evas_Coord                           adv, x, w, h;  /**< Item co-ordinates. Advancement to be made, x co-ord, width and height. */
   Evas_Coord                           yoff;  /**< y offset. */
   Eina_Bool                            merge : 1; /**< Indicates whether this item should merge to the previous item or not */
   Eina_Bool                            visually_deleted : 1; /**< Indicates whether this item is used in the visual layout or not. */
};

struct _Evas_Object_Textblock2_Text_Item
{
   Evas_Object_Textblock2_Item       parent;  /**< Textblock2 item. */
   Evas_Text_Props                  text_props;  /**< Props for this item. */
   Evas_Coord                       inset;  /**< Inset of text item. */
   Evas_Coord                       x_adjustment; /**< Used to indicate by how much we adjusted sizes */
};

struct _Evas_Object_Textblock2_Format_Item
{
   Evas_Object_Textblock2_Item           parent;  /**< Textblock2 item. */
   Evas_BiDi_Direction                  bidi_dir;  /**< Bidi text direction. */
   const char                          *item;  /**< Pointer to item contents. */
   int                                  y;  /**< Co-ordinate of item. */
   unsigned char                        vsize : 2;  /**< VSIZE_FULL or VSIZE_ASCENT */
   unsigned char                        size : 2;  /**< SIZE, SIZE_ABS or SIZE_REL*/
   Eina_Bool                            formatme : 1;  /**< EINA_TRUE if format required, else EINA_FALSE */
};

struct _Evas_Object_Textblock2_Format
{
   double               halign;  /**< Horizontal alignment value. */
   double               valign;  /**< Vertical alignment value. */
   struct {
      Evas_Font_Description *fdesc;  /**< Pointer to font description. */
      const char       *source;  /**< Pointer to object from which to search for the font. */
      Evas_Font_Set    *font;  /**< Pointer to font set. */
      Evas_Font_Size    size;  /**< Size of the font. */
   } font;
   struct {
      struct {
	 unsigned char  r, g, b, a;
      } normal, underline, underline2, underline_dash, outline, shadow, glow, glow2, backing,
	strikethrough;
   } color;
   struct {
      int               l, r;
   } margin;  /**< Left and right margin width. */
   int                  ref;  /**< Value of the ref. */
   int                  tabstops;  /**< Value of the size of the tab character. */
   int                  linesize;  /**< Value of the size of the line of the text. */
   int                  linegap;  /**< Value to set the line gap in text. */
   int                  underline_dash_width;  /**< Valule to set the width of the underline dash. */
   int                  underline_dash_gap;  /**< Value to set the gap of the underline dash. */
   double               linerelsize;  /**< Value to set the size of line of text. */
   double               linerelgap;  /**< Value for setting line gap. */
   double               linefill;  /**< The value must be a percentage. */
   double               ellipsis;  /**< The value should be a number. Any value smaller than 0.0 or greater than 1.0 disables ellipsis. A value of 0 means ellipsizing the leftmost portion of the text first, 1 on the other hand the rightmost portion. */
   unsigned char        style;  /**< Value from Evas_Text_Style_Type enum. */
   Eina_Bool            wrap_word : 1;  /**< EINA_TRUE if only wraps lines at word boundaries, else EINA_FALSE. */
   Eina_Bool            wrap_char : 1;  /**< EINA_TRUE if wraps at any character, else EINA_FALSE. */
   Eina_Bool            wrap_mixed : 1;  /**< EINA_TRUE if wrap at words if possible, else EINA_FALSE. */
   Eina_Bool            underline : 1;  /**< EINA_TRUE if a single line under the text, else EINA_FALSE */
   Eina_Bool            underline2 : 1;  /**< EINA_TRUE if two lines under the text, else EINA_FALSE */
   Eina_Bool            underline_dash : 1;  /**< EINA_TRUE if a dashed line under the text, else EINA_FALSE */
   Eina_Bool            strikethrough : 1;  /**< EINA_TRUE if text should be stricked off, else EINA_FALSE */
   Eina_Bool            backing : 1;  /**< EINA_TRUE if enable background color, else EINA_FALSE */
   Eina_Bool            halign_auto : 1;  /**< EINA_TRUE if auto horizontal align, else EINA_FALSE */
};

struct _Evas_Textblock2_Style
{
   const char            *style_text;
   char                  *default_tag;
   Evas_Object_Style_Tag *tags;
   Eina_List             *objects;
   Eina_Bool              delete_me : 1;
};

struct _Evas_Textblock2_Cursor
{
   Evas_Object                     *obj;
   size_t                           pos;
   Evas_Object_Textblock2_Node_Text *node;
};

/* Size of the index array */
#define TEXTBLOCK2_PAR_INDEX_SIZE 10
struct _Evas_Object_Textblock2
{
   DATA32                              magic;
   Evas_Textblock2_Style               *style;
   Evas_Textblock2_Style               *style_user;
   Evas_Textblock2_Cursor              *cursor;
   Eina_List                          *cursors;
   Evas_Object_Textblock2_Node_Text    *text_nodes;

   int                                 num_paragraphs;
   Evas_Object_Textblock2_Paragraph    *paragraphs;
   Evas_Object_Textblock2_Paragraph    *par_index[TEXTBLOCK2_PAR_INDEX_SIZE];

   Evas_Object_Textblock2_Text_Item    *ellip_ti;
   Eina_List                          *ellip_prev_it; /* item that is placed before ellipsis item (0.0 <= ellipsis < 1.0), if required */
   Eina_List                          *anchors_a;
   Eina_List                          *anchors_item;
   int                                 last_w, last_h;
   struct {
      int                              l, r, t, b;
   } style_pad;
   double                              valign;
   char                               *markup_text;
   void                               *engine_data;
   const char                         *bidi_delimiters;
   struct {
      int                              w, h, oneline_h;
      Eina_Bool                        valid : 1;
   } formatted, native;
   Eina_Bool                           redraw : 1;
   Eina_Bool                           changed : 1;
   Eina_Bool                           content_changed : 1;
   Eina_Bool                           format_changed : 1;
   Eina_Bool                           have_ellipsis : 1;
};

struct _Evas_Textblock2_Selection_Iterator
{
   Eina_Iterator                       iterator; /**< Eina Iterator. */
   Eina_List                           *list; /**< Head of list. */
   Eina_List                           *current; /**< Current node in loop. */
};

/* private methods for textblock2 objects */
static void evas_object_textblock2_init(Evas_Object *eo_obj);
static void evas_object_textblock2_render(Evas_Object *eo_obj,
					 Evas_Object_Protected_Data *obj,
					 void *type_private_data,
					 void *output, void *context, void *surface,
					 int x, int y, Eina_Bool do_async);
static void evas_object_textblock2_free(Evas_Object *eo_obj);
static void evas_object_textblock2_render_pre(Evas_Object *eo_obj,
					     Evas_Object_Protected_Data *obj,
					     void *type_private_data);
static void evas_object_textblock2_render_post(Evas_Object *eo_obj,
					      Evas_Object_Protected_Data *obj,
					      void *type_private_data);
static Evas_Object_Textblock2_Node_Text *_evas_textblock2_node_text_new(void);

static unsigned int evas_object_textblock2_id_get(Evas_Object *eo_obj);
static unsigned int evas_object_textblock2_visual_id_get(Evas_Object *eo_obj);
static void *evas_object_textblock2_engine_data_get(Evas_Object *eo_obj);

static int evas_object_textblock2_is_opaque(Evas_Object *eo_obj,
					   Evas_Object_Protected_Data *obj,
					   void *type_private_data);
static int evas_object_textblock2_was_opaque(Evas_Object *eo_obj,
					    Evas_Object_Protected_Data *obj,
					    void *type_private_data);
static void evas_object_textblock2_coords_recalc(Evas_Object *eo_obj,
						Evas_Object_Protected_Data *obj,
						void *type_private_data);
static void evas_object_textblock2_scale_update(Evas_Object *eo_obj,
					       Evas_Object_Protected_Data *obj,
					       void *type_private_data);

static const Evas_Object_Func object_func =
{
   /* methods (compulsory) */
   NULL,
     evas_object_textblock2_render,
     evas_object_textblock2_render_pre,
     evas_object_textblock2_render_post,
     evas_object_textblock2_id_get,
     evas_object_textblock2_visual_id_get,
     evas_object_textblock2_engine_data_get,
   /* these are optional. NULL = nothing */
     NULL,
     NULL,
     NULL,
     NULL,
     evas_object_textblock2_is_opaque,
     evas_object_textblock2_was_opaque,
     NULL,
     NULL,
     NULL, /*evas_object_textblock2_coords_recalc, <- disable - not useful. */
     evas_object_textblock2_scale_update,
     NULL,
     NULL,
     NULL
};

/* the actual api call to add a textblock2 */

#define TB_HEAD() \
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ); \
   return; \
   MAGIC_CHECK_END(); \
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

#define TB_HEAD_RETURN(x) \
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ); \
   return (x); \
   MAGIC_CHECK_END();

static void _evas_textblock2_node_text_remove(Evas_Textblock2_Data *o, Evas_Object_Textblock2_Node_Text *n);
static void _evas_textblock2_node_text_free(Evas_Object_Textblock2_Node_Text *n);
static void _evas_textblock2_changed(Evas_Textblock2_Data *o, Evas_Object *eo_obj);
static void _evas_textblock2_invalidate_all(Evas_Textblock2_Data *o);
static void _evas_textblock2_cursors_update_offset(const Evas_Textblock2_Cursor *cur, const Evas_Object_Textblock2_Node_Text *n, size_t start, int offset);
static void _evas_textblock2_cursors_set_node(Evas_Textblock2_Data *o, const Evas_Object_Textblock2_Node_Text *n, Evas_Object_Textblock2_Node_Text *new_node);

/** selection iterator */
/**
  * @internal
  * Returns the value of the current data of list node,
  * and goes to the next list node.
  *
  * @param it the iterator.
  * @param data the data of the current list node.
  * @return EINA_FALSE if the current list node does not exists.
  * Otherwise, returns EINA_TRUE.
  */
static Eina_Bool
_evas_textblock2_selection_iterator_next(Evas_Textblock2_Selection_Iterator *it, void **data)
{
   if (!it->current)
     return EINA_FALSE;

   *data = eina_list_data_get(it->current);
   it->current = eina_list_next(it->current);

   return EINA_TRUE;
}

/**
  * @internal
  * Gets the iterator container (Eina_List) which created the iterator.
  * @param it the iterator.
  * @return A pointer to Eina_List.
  */
static Eina_List *
_evas_textblock2_selection_iterator_get_container(Evas_Textblock2_Selection_Iterator *it)
{
   return it->list;
}

/**
  * @internal
  * Frees the iterator container (Eina_List).
  * @param it the iterator.
  */
static void
_evas_textblock2_selection_iterator_free(Evas_Textblock2_Selection_Iterator *it)
{
   while (it->list)
     it->list = eina_list_remove_list(it->list, it->list);
   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

/**
  * @internal
  * Creates newly allocated  iterator associated to a list.
  * @param list The list.
  * @return If the memory cannot be allocated, NULL is returned.
  * Otherwise, a valid iterator is returned.
  */
Eina_Iterator *
_evas_textblock2_selection_iterator_new(Eina_List *list)
{
   Evas_Textblock2_Selection_Iterator *it;

   it = calloc(1, sizeof(Evas_Textblock2_Selection_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);
   it->list = list;
   it->current = list;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(
                                     _evas_textblock2_selection_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
                            _evas_textblock2_selection_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(
                                     _evas_textblock2_selection_iterator_free);

   return &it->iterator;
}

/* styles */
/**
 * @internal
 * Clears the textblock2 style passed except for the style_text which is replaced.
 * @param ts The ts to be cleared. Must not be NULL.
 * @param style_text the style's text.
 */
static void
_style_replace(Evas_Textblock2_Style *ts, const char *style_text)
{
   eina_stringshare_replace(&ts->style_text, style_text);
   if (ts->default_tag) free(ts->default_tag);
   while (ts->tags)
     {
	Evas_Object_Style_Tag *tag;

	tag = (Evas_Object_Style_Tag *)ts->tags;
	ts->tags = (Evas_Object_Style_Tag *)eina_inlist_remove(EINA_INLIST_GET(ts->tags), EINA_INLIST_GET(tag));
	free(tag->tag.tag);
	free(tag->tag.replace);
	free(tag);
     }
   ts->default_tag = NULL;
   ts->tags = NULL;
}

/**
 * @internal
 * Clears the textblock2 style passed.
 * @param ts The ts to be cleared. Must not be NULL.
 */
static void
_style_clear(Evas_Textblock2_Style *ts)
{
   _style_replace(ts, NULL);
}

/**
 * @internal
 * Clears all the nodes (text and format) of the textblock2 object.
 * @param obj The evas object, must not be NULL.
 */
static void
_nodes_clear(const Evas_Object *eo_obj)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   while (o->text_nodes)
     {
	Evas_Object_Textblock2_Node_Text *n;

	n = o->text_nodes;
        o->text_nodes = _NODE_TEXT(eina_inlist_remove(
                 EINA_INLIST_GET(o->text_nodes), EINA_INLIST_GET(n)));
        _evas_textblock2_node_text_free(n);
     }
}

/**
 * @internal
 * Unrefs and frees (if needed) a textblock2 format.
 * @param obj The Evas_Object, Must not be NULL.
 * @param fmt the format to be cleaned, must not be NULL.
 */
static void
_format_unref_free(const Evas_Object *eo_obj, Evas_Object_Textblock2_Format *fmt)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   fmt->ref--;
   if (fmt->ref > 0) return;
   if (fmt->font.fdesc) evas_font_desc_unref(fmt->font.fdesc);
   if (fmt->font.source) eina_stringshare_del(fmt->font.source);
   evas_font_free(obj->layer->evas->evas, fmt->font.font);
   free(fmt);
}

/**
 * @internal
 * Free a layout item
 * @param obj The evas object, must not be NULL.
 * @param ln the layout line on which the item is in, must not be NULL.
 * @param it the layout item to be freed
 */
static void
_item_free(const Evas_Object *eo_obj, Evas_Object_Textblock2_Line *ln, Evas_Object_Textblock2_Item *it)
{
   if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
     {
        Evas_Object_Textblock2_Text_Item *ti = _ITEM_TEXT(it);

        evas_common_text_props_content_unref(&ti->text_props);
     }
   else
     {
        Evas_Object_Textblock2_Format_Item *fi = _ITEM_FORMAT(it);

        if (fi->item) eina_stringshare_del(fi->item);
     }
   _format_unref_free(eo_obj, it->format);
   if (ln)
     {
        ln->items = (Evas_Object_Textblock2_Item *) eina_inlist_remove(
              EINA_INLIST_GET(ln->items), EINA_INLIST_GET(ln->items));
     }
   free(it);
}

/**
 * @internal
 * Free a layout line.
 * @param obj The evas object, must not be NULL.
 * @param ln the layout line to be freed, must not be NULL.
 */
static void
_line_free(Evas_Object_Textblock2_Line *ln)
{
   /* Items are freed from the logical list, except for the ellip item */
   if (ln) free(ln);
}

/**
 * @internal
 * Checks if a char is a whitespace.
 * @param c the unicode codepoint.
 * @return @c EINA_TRUE if the unicode codepoint is a whitespace, @c EINA_FALSE
 * otherwise.
 */
static Eina_Bool
_is_white(Eina_Unicode c)
{
   /*
    * unicode list of whitespace chars
    *
    * 0009..000D <control-0009>..<control-000D>
    * 0020 SPACE
    * 0085 <control-0085>
    * 00A0 NO-BREAK SPACE
    * 1680 OGHAM SPACE MARK
    * 180E MONGOLIAN VOWEL SEPARATOR
    * 2000..200A EN QUAD..HAIR SPACE
    * 2028 LINE SEPARATOR
    * 2029 PARAGRAPH SEPARATOR
    * 202F NARROW NO-BREAK SPACE
    * 205F MEDIUM MATHEMATICAL SPACE
    * 3000 IDEOGRAPHIC SPACE
    */
   if (
         (c == 0x20) ||
         ((c >= 0x9) && (c <= 0xd)) ||
         (c == 0x85) ||
         (c == 0xa0) ||
         (c == 0x1680) ||
         (c == 0x180e) ||
         ((c >= 0x2000) && (c <= 0x200a)) ||
         (c == 0x2028) ||
         (c == 0x2029) ||
         (c == 0x202f) ||
         (c == 0x205f) ||
         (c == 0x3000)
      )
     return EINA_TRUE;
   return EINA_FALSE;
}

/* The refcount for the formats. */
static int format_refcount = 0;
/* Holders for the stringshares */
static const char *fontstr = NULL;
static const char *font_fallbacksstr = NULL;
static const char *font_sizestr = NULL;
static const char *font_sourcestr = NULL;
static const char *font_weightstr = NULL;
static const char *font_stylestr = NULL;
static const char *font_widthstr = NULL;
static const char *langstr = NULL;
static const char *colorstr = NULL;
static const char *underline_colorstr = NULL;
static const char *underline2_colorstr = NULL;
static const char *underline_dash_colorstr = NULL;
static const char *outline_colorstr = NULL;
static const char *shadow_colorstr = NULL;
static const char *glow_colorstr = NULL;
static const char *glow2_colorstr = NULL;
static const char *backing_colorstr = NULL;
static const char *strikethrough_colorstr = NULL;
static const char *alignstr = NULL;
static const char *valignstr = NULL;
static const char *wrapstr = NULL;
static const char *left_marginstr = NULL;
static const char *right_marginstr = NULL;
static const char *underlinestr = NULL;
static const char *strikethroughstr = NULL;
static const char *backingstr = NULL;
static const char *stylestr = NULL;
static const char *tabstopsstr = NULL;
static const char *linesizestr = NULL;
static const char *linerelsizestr = NULL;
static const char *linegapstr = NULL;
static const char *linerelgapstr = NULL;
static const char *itemstr = NULL;
static const char *linefillstr = NULL;
static const char *ellipsisstr = NULL;
static const char *underline_dash_widthstr = NULL;
static const char *underline_dash_gapstr = NULL;

/**
 * @page evas_textblock2_style_page Evas Textblock2 Style Options
 *
 * @brief This page describes how to style text in an Evas Text Block.
 */

/**
 * @internal
 * Init the format strings.
 */
static void
_format_command_init(void)
{
   if (format_refcount == 0)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @section evas_textblock2_style_index Index
         *
         * The following styling commands are accepted:
         * @li @ref evas_textblock2_style_font
         * @li @ref evas_textblock2_style_font_fallback
         * @li @ref evas_textblock2_style_font_size
         * @li @ref evas_textblock2_style_font_source
         * @li @ref evas_textblock2_style_font_weight
         * @li @ref evas_textblock2_style_font_style
         * @li @ref evas_textblock2_style_font_width
         * @li @ref evas_textblock2_style_lang
         * @li @ref evas_textblock2_style_color
         * @li @ref evas_textblock2_style_underline_color
         * @li @ref evas_textblock2_style_underline2_color
         * @li @ref evas_textblock2_style_underline_dash_color
         * @li @ref evas_textblock2_style_outline_color
         * @li @ref evas_textblock2_style_shadow_color
         * @li @ref evas_textblock2_style_glow_color
         * @li @ref evas_textblock2_style_glow2_color
         * @li @ref evas_textblock2_style_backing_color
         * @li @ref evas_textblock2_style_strikethrough_color
         * @li @ref evas_textblock2_style_align
         * @li @ref evas_textblock2_style_valign
         * @li @ref evas_textblock2_style_wrap
         * @li @ref evas_textblock2_style_left_margin
         * @li @ref evas_textblock2_style_right_margin
         * @li @ref evas_textblock2_style_underline
         * @li @ref evas_textblock2_style_strikethrough
         * @li @ref evas_textblock2_style_backing
         * @li @ref evas_textblock2_style_style
         * @li @ref evas_textblock2_style_tabstops
         * @li @ref evas_textblock2_style_linesize
         * @li @ref evas_textblock2_style_linerelsize
         * @li @ref evas_textblock2_style_linegap
         * @li @ref evas_textblock2_style_linerelgap
         * @li @ref evas_textblock2_style_item
         * @li @ref evas_textblock2_style_linefill
         * @li @ref evas_textblock2_style_ellipsis
         * @li @ref evas_textblock2_style_password
         * @li @ref evas_textblock2_style_underline_dash_width
         * @li @ref evas_textblock2_style_underline_dash_gap
         *
         * @section evas_textblock2_style_contents Contents
         */
        fontstr = eina_stringshare_add("font");
        font_fallbacksstr = eina_stringshare_add("font_fallbacks");
        font_sizestr = eina_stringshare_add("font_size");
        font_sourcestr = eina_stringshare_add("font_source");
        font_weightstr = eina_stringshare_add("font_weight");
        font_stylestr = eina_stringshare_add("font_style");
        font_widthstr = eina_stringshare_add("font_width");
        langstr = eina_stringshare_add("lang");
        colorstr = eina_stringshare_add("color");
        underline_colorstr = eina_stringshare_add("underline_color");
        underline2_colorstr = eina_stringshare_add("underline2_color");
        underline_dash_colorstr = eina_stringshare_add("underline_dash_color");
        outline_colorstr = eina_stringshare_add("outline_color");
        shadow_colorstr = eina_stringshare_add("shadow_color");
        glow_colorstr = eina_stringshare_add("glow_color");
        glow2_colorstr = eina_stringshare_add("glow2_color");
        backing_colorstr = eina_stringshare_add("backing_color");
        strikethrough_colorstr = eina_stringshare_add("strikethrough_color");
        alignstr = eina_stringshare_add("align");
        valignstr = eina_stringshare_add("valign");
        wrapstr = eina_stringshare_add("wrap");
        left_marginstr = eina_stringshare_add("left_margin");
        right_marginstr = eina_stringshare_add("right_margin");
        underlinestr = eina_stringshare_add("underline");
        strikethroughstr = eina_stringshare_add("strikethrough");
        backingstr = eina_stringshare_add("backing");
        stylestr = eina_stringshare_add("style");
        tabstopsstr = eina_stringshare_add("tabstops");
        linesizestr = eina_stringshare_add("linesize");
        linerelsizestr = eina_stringshare_add("linerelsize");
        linegapstr = eina_stringshare_add("linegap");
        linerelgapstr = eina_stringshare_add("linerelgap");
        itemstr = eina_stringshare_add("item");
        linefillstr = eina_stringshare_add("linefill");
        ellipsisstr = eina_stringshare_add("ellipsis");
        underline_dash_widthstr = eina_stringshare_add("underline_dash_width");
        underline_dash_gapstr = eina_stringshare_add("underline_dash_gap");
     }
   format_refcount++;
}

/**
 * @internal
 * Shutdown the format strings.
 */
static void
_format_command_shutdown(void)
{
   if (--format_refcount > 0) return;

   eina_stringshare_del(fontstr);
   eina_stringshare_del(font_fallbacksstr);
   eina_stringshare_del(font_sizestr);
   eina_stringshare_del(font_sourcestr);
   eina_stringshare_del(font_weightstr);
   eina_stringshare_del(font_stylestr);
   eina_stringshare_del(font_widthstr);
   eina_stringshare_del(langstr);
   eina_stringshare_del(colorstr);
   eina_stringshare_del(underline_colorstr);
   eina_stringshare_del(underline2_colorstr);
   eina_stringshare_del(underline_dash_colorstr);
   eina_stringshare_del(outline_colorstr);
   eina_stringshare_del(shadow_colorstr);
   eina_stringshare_del(glow_colorstr);
   eina_stringshare_del(glow2_colorstr);
   eina_stringshare_del(backing_colorstr);
   eina_stringshare_del(strikethrough_colorstr);
   eina_stringshare_del(alignstr);
   eina_stringshare_del(valignstr);
   eina_stringshare_del(wrapstr);
   eina_stringshare_del(left_marginstr);
   eina_stringshare_del(right_marginstr);
   eina_stringshare_del(underlinestr);
   eina_stringshare_del(strikethroughstr);
   eina_stringshare_del(backingstr);
   eina_stringshare_del(stylestr);
   eina_stringshare_del(tabstopsstr);
   eina_stringshare_del(linesizestr);
   eina_stringshare_del(linerelsizestr);
   eina_stringshare_del(linegapstr);
   eina_stringshare_del(linerelgapstr);
   eina_stringshare_del(itemstr);
   eina_stringshare_del(linefillstr);
   eina_stringshare_del(ellipsisstr);
   eina_stringshare_del(underline_dash_widthstr);
   eina_stringshare_del(underline_dash_gapstr);
}

/**
 * @internal
 * Copies str to dst while removing the \\ char, i.e unescape the escape sequences.
 *
 * @param[out] dst the destination string - Should not be NULL.
 * @param[in] src the source string - Should not be NULL.
 */
static int
_format_clean_param(Eina_Tmpstr *s)
{
   Eina_Tmpstr *ss;
   char *ds;
   int len = 0;

   ds = (char*) s;
   for (ss = s; *ss; ss++, ds++, len++)
     {
        if ((*ss == '\\') && *(ss + 1)) ss++;
        if (ds != ss) *ds = *ss;
     }
   *ds = 0;

   return len;
}

/**
 * @internal
 * Parses the cmd and parameter and adds the parsed format to fmt.
 *
 * @param obj the evas object - should not be NULL.
 * @param fmt The format to populate - should not be NULL.
 * @param[in] cmd the command to process, should be stringshared.
 * @param[in] param the parameter of the command.
 */
static void
_format_command(Evas_Object *eo_obj, Evas_Object_Textblock2_Format *fmt, const char *cmd, Eina_Tmpstr *param)
{
   int len;

   len = _format_clean_param(param);

   /* If we are changing the font, create the fdesc. */
   if ((cmd == font_weightstr) || (cmd == font_widthstr) ||
         (cmd == font_stylestr) || (cmd == langstr) ||
         (cmd == fontstr) || (cmd == font_fallbacksstr))
     {
        if (!fmt->font.fdesc)
          {
             fmt->font.fdesc = evas_font_desc_new();
          }
        else if (!fmt->font.fdesc->is_new)
          {
             Evas_Font_Description *old = fmt->font.fdesc;
             fmt->font.fdesc = evas_font_desc_dup(fmt->font.fdesc);
             if (old) evas_font_desc_unref(old);
          }
     }


   if (cmd == fontstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font Font
         *
         * This sets the name of the font to be used.
         * @code
         * font=<font name>
         * @endcode
         */
        evas_font_name_parse(fmt->font.fdesc, param);
     }
   else if (cmd == font_fallbacksstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font_fallback Font fallback
         *
         * This sets the name of the fallback font to be used. This font will
         * be used if the primary font is not available.
         * @code
         * font_fallbacks=<font name>
         * @endcode
         */
        eina_stringshare_replace(&(fmt->font.fdesc->fallbacks), param);
     }
   else if (cmd == font_sizestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font_size Font size
         *
         * This sets the the size of font in points to be used.
         * @code
         * font_size=<size>
         * @endcode
         */
        int v;

        v = atoi(param);
        if (v != fmt->font.size)
          {
             fmt->font.size = v;
          }
     }
   else if (cmd == font_sourcestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font_source Font source
         *
         * Specify an object from which to search for the font.
         * @code
         * font_source=<source>
         * @endcode
         */
        if ((!fmt->font.source) ||
              ((fmt->font.source) && (strcmp(fmt->font.source, param))))
          {
             eina_stringshare_replace(&(fmt->font.source), param);
          }
     }
   else if (cmd == font_weightstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font_weight Font weight
         *
         * Sets the weight of the font. The value must be one of:
         * @li "normal"
         * @li "thin"
         * @li "ultralight"
         * @li "light"
         * @li "book"
         * @li "medium"
         * @li "semibold"
         * @li "bold"
         * @li "ultrabold"
         * @li "black"
         * @li "extrablack"
         * @code
         * font_weight=<weight>
         * @endcode
         */
        fmt->font.fdesc->weight = evas_font_style_find(param,
                                                       param + len,
                                                       EVAS_FONT_STYLE_WEIGHT);
     }
   else if (cmd == font_stylestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font_style Font style
         *
         * Sets the style of the font. The value must be one of:
         * @li "normal"
         * @li "oblique"
         * @li "italic"
         * @code
         * font_style=<style>
         * @endcode
         */
        fmt->font.fdesc->slant = evas_font_style_find(param,
                                                      param + len,
                                                      EVAS_FONT_STYLE_SLANT);
     }
   else if (cmd == font_widthstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_font_width Font width
         *
         * Sets the width of the font. The value must be one of:
         * @li "normal"
         * @li "ultracondensed"
         * @li "extracondensed"
         * @li "condensed"
         * @li "semicondensed"
         * @li "semiexpanded"
         * @li "expanded"
         * @li "extraexpanded"
         * @li "ultraexpanded"
         * @code
         * font_width=<width>
         * @endcode
         */
        fmt->font.fdesc->width = evas_font_style_find(param,
                                                      param + len,
                                                      EVAS_FONT_STYLE_WIDTH);
     }
   else if (cmd == langstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_lang Language
         *
         * Sets the language of the text for FontConfig.
         * @code
         * lang=<language>
         * @endcode
         */
        eina_stringshare_replace(&(fmt->font.fdesc->lang), param);
     }
   else if (cmd == colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_color Color
      *
      * Sets the color of the text. The following formats are accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.normal.r), &(fmt->color.normal.g),
           &(fmt->color.normal.b), &(fmt->color.normal.a));
   else if (cmd == underline_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_underline_color Underline Color
      *
      * Sets the color of the underline. The following formats are accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * underline_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.underline.r), &(fmt->color.underline.g),
           &(fmt->color.underline.b), &(fmt->color.underline.a));
   else if (cmd == underline2_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_underline2_color Second Underline Color
      *
      * Sets the color of the second line of underline(when using underline
      * mode "double"). The following formats are accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * underline2_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.underline2.r), &(fmt->color.underline2.g),
           &(fmt->color.underline2.b), &(fmt->color.underline2.a));
   else if (cmd == underline_dash_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_underline_dash_color Underline Dash Color
      *
      * Sets the color of dashed underline. The following formats are accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * underline_dash_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.underline_dash.r), &(fmt->color.underline_dash.g),
           &(fmt->color.underline_dash.b), &(fmt->color.underline_dash.a));
   else if (cmd == outline_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_outline_color Outline Color
      *
      * Sets the color of the outline of the text. The following formats are
      * accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * outline_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.outline.r), &(fmt->color.outline.g),
           &(fmt->color.outline.b), &(fmt->color.outline.a));
   else if (cmd == shadow_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_shadow_color Shadow Color
      *
      * Sets the color of the shadow of the text. The following formats are
      * accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * shadow_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.shadow.r), &(fmt->color.shadow.g),
           &(fmt->color.shadow.b), &(fmt->color.shadow.a));
   else if (cmd == glow_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_glow_color First Glow Color
      *
      * Sets the first color of the glow of text. The following formats are
      * accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * glow_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.glow.r), &(fmt->color.glow.g),
           &(fmt->color.glow.b), &(fmt->color.glow.a));
   else if (cmd == glow2_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_glow2_color Second Glow Color
      *
      * Sets the second color of the glow of text. The following formats are
      * accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * glow2_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.glow2.r), &(fmt->color.glow2.g),
           &(fmt->color.glow2.b), &(fmt->color.glow2.a));
   else if (cmd == backing_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_backing_color Backing Color
      *
      * Sets a background color for text. The following formats are
      * accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * backing_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.backing.r), &(fmt->color.backing.g),
           &(fmt->color.backing.b), &(fmt->color.backing.a));
   else if (cmd == strikethrough_colorstr)
     /**
      * @page evas_textblock2_style_page Evas Textblock2 Style Options
      *
      * @subsection evas_textblock2_style_strikethrough_color Strikethrough Color
      *
      * Sets the color of text that is striked through. The following formats
      * are accepted:
      * @li "#RRGGBB"
      * @li "#RRGGBBAA"
      * @li "#RGB"
      * @li "#RGBA"
      * @code
      * strikethrough_color=<color>
      * @endcode
      */
     evas_common_format_color_parse(param, len,
           &(fmt->color.strikethrough.r), &(fmt->color.strikethrough.g),
           &(fmt->color.strikethrough.b), &(fmt->color.strikethrough.a));
   else if (cmd == alignstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_align Horizontal Align
         *
         * Sets the horizontal alignment of the text. The value can either be
         * a number, a percentage or one of several presets:
         * @li "auto" - Respects LTR/RTL settings
         * @li "center" - Centers the text in the line
         * @li "middle" - Alias for "center"
         * @li "left" - Puts the text at the left of the line
         * @li "right" - Puts the text at the right of the line
         * @li <number> - A number between 0.0 and 1.0 where 0.0 represents
         * "left" and 1.0 represents "right"
         * @li <number>% - A percentage between 0% and 100% where 0%
         * represents "left" and 100% represents "right"
         * @code
         * align=<value or preset>
         * @endcode
         */
        if (len == 4 && !strcmp(param, "auto"))
          {
             fmt->halign_auto = EINA_TRUE;
          }
        else
          {
             static const struct {
                const char *param;
                int len;
                double halign;
             } halign_named[] = {
               { "middle", 6, 0.5 },
               { "center", 6, 0.5 },
               { "left", 4, 0.0 },
               { "right", 5, 1.0 },
               { NULL, 0, 0.0 }
             };
             unsigned int i;

             for (i = 0; halign_named[i].param; i++)
               if (len == halign_named[i].len &&
                   !strcmp(param, halign_named[i].param))
                 {
                    fmt->halign = halign_named[i].halign;
                    break;
                 }

             if (halign_named[i].param == NULL)
               {
                  char *endptr = NULL;
                  double val = strtod(param, &endptr);
                  if (endptr)
                    {
                       while (*endptr && _is_white(*endptr))
                         endptr++;
                       if (*endptr == '%')
                         val /= 100.0;
                    }
                  fmt->halign = val;
                  if (fmt->halign < 0.0) fmt->halign = 0.0;
                  else if (fmt->halign > 1.0) fmt->halign = 1.0;
               }
             fmt->halign_auto = EINA_FALSE;
          }
     }
   else if (cmd == valignstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_valign Vertical Align
         *
         * Sets the vertical alignment of the text. The value can either be
         * a number or one of the following presets:
         * @li "top" - Puts text at the top of the line
         * @li "center" - Centers the text in the line
         * @li "middle" - Alias for "center"
         * @li "bottom" - Puts the text at the bottom of the line
         * @li "baseline" - Baseline
         * @li "base" - Alias for "baseline"
         * @li <number> - A number between 0.0 and 1.0 where 0.0 represents
         * "top" and 1.0 represents "bottom"
         * @li <number>% - A percentage between 0% and 100% where 0%
         * represents "top" and 100% represents "bottom"
         * @code
         * valign=<value or preset>
         * @endcode
         *
         * See explanation of baseline at:
         * https://en.wikipedia.org/wiki/Baseline_%28typography%29
         */
        static const struct {
           const char *param;
           int len;
           double valign;
        } valign_named[] = {
          { "top", 3, 0.0 },
          { "middle", 6, 0.5 },
          { "center", 6, 0.5 },
          { "bottom", 6, 1.0 },
          { "baseline", 8, -1.0 },
          { "base", 4, -1.0 },
          { NULL, 0, 0 }
        };
        unsigned int i;

        for (i = 0; valign_named[i].param; i++)
          if (len == valign_named[i].len &&
              !strcmp(valign_named[i].param, param))
            {
               fmt->valign = valign_named[i].valign;
               break;
            }

        if (valign_named[i].param == NULL)
          {
             char *endptr = NULL;
             double val = strtod(param, &endptr);
             if (endptr)
               {
                  while (*endptr && _is_white(*endptr))
                    endptr++;
                  if (*endptr == '%')
                    val /= 100.0;
               }
             fmt->valign = val;
             if (fmt->valign < 0.0) fmt->valign = 0.0;
             else if (fmt->valign > 1.0) fmt->valign = 1.0;
          }
     }
   else if (cmd == wrapstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_wrap Wrap
         *
         * Sets the wrap policy of the text. The value must be one of the
         * following:
         * @li "word" - Only wraps lines at word boundaries
         * @li "char" - Wraps at any character
         * @li "mixed" - Wrap at words if possible, if not at any character
         * @li "" - Don't wrap
         * @code
         * wrap=<value or preset>
         * @endcode
         */
        static const struct {
           const char *param;
           int len;
           Eina_Bool wrap_word;
           Eina_Bool wrap_char;
           Eina_Bool wrap_mixed;
        } wrap_named[] = {
          { "word", 4, 1, 0, 0 },
          { "char", 4, 0, 1, 0 },
          { "mixed", 5, 0, 0, 1 },
          { NULL, 0, 0, 0, 0 }
        };
        unsigned int i;

        fmt->wrap_word = fmt->wrap_mixed = fmt->wrap_char = 0;
        for (i = 0; wrap_named[i].param; i++)
          if (wrap_named[i].len == len &&
              !strcmp(wrap_named[i].param, param))
            {
               fmt->wrap_word = wrap_named[i].wrap_word;
               fmt->wrap_char = wrap_named[i].wrap_char;
               fmt->wrap_mixed = wrap_named[i].wrap_mixed;
               break;
            }
     }
   else if (cmd == left_marginstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_left_margin Left margin
         *
         * Sets the left margin of the text. The value can be a number, an
         * increment, decrement or "reset":
         * @li +<number> - Increments existing left margin by <number>
         * @li -<number> - Decrements existing left margin by <number>
         * @li <number> - Sets left margin to <number>
         * @li "reset" - Sets left margin to 0
         * @code
         * left_margin=<value or reset>
         * @endcode
         */
        if (len == 5 && !strcmp(param, "reset"))
          fmt->margin.l = 0;
        else
          {
             if (param[0] == '+')
               fmt->margin.l += atoi(&(param[1]));
             else if (param[0] == '-')
               fmt->margin.l -= atoi(&(param[1]));
             else
               fmt->margin.l = atoi(param);
             if (fmt->margin.l < 0) fmt->margin.l = 0;
          }
     }
   else if (cmd == right_marginstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_right_margin Right margin
         *
         * Sets the right margin of the text. The value can be a number, an
         * increment, decrement or "reset":
         * @li +<number> - Increments existing right margin by <number>
         * @li -<number> - Decrements existing right margin by <number>
         * @li <number> - Sets left margin to <number>
         * @li "reset" - Sets left margin to 0
         * @code
         * right_margin=<value or reset>
         * @endcode
         */
        if (len == 5 && !strcmp(param, "reset"))
          fmt->margin.r = 0;
        else
          {
             if (param[0] == '+')
               fmt->margin.r += atoi(&(param[1]));
             else if (param[0] == '-')
               fmt->margin.r -= atoi(&(param[1]));
             else
               fmt->margin.r = atoi(param);
             if (fmt->margin.r < 0) fmt->margin.r = 0;
          }
     }
   else if (cmd == underlinestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_underline Underline
         *
         * Sets if and how a text will be underlined. The value must be one of
         * the following:
         * @li "off" - No underlining
         * @li "single" - A single line under the text
         * @li "on" - Alias for "single"
         * @li "double" - Two lines under the text
         * @li "dashed" - A dashed line under the text
         * @code
         * underline=off/single/on/double/dashed
         * @endcode
         */
        static const struct {
           const char *param;
           int len;
           Eina_Bool underline;
           Eina_Bool underline2;
           Eina_Bool underline_dash;
        } underlines_named[] = {
          { "off", 3, 0, 0, 0 },
          { "on", 2, 1, 0, 0 },
          { "single", 6, 1, 0, 0 },
          { "double", 6, 1, 1, 0 },
          { "dashed", 6, 0, 0, 1 },
          { NULL, 0, 0, 0, 0 }
        };
        unsigned int i;

        fmt->underline = fmt->underline2 = fmt->underline_dash = 0;
        for (i = 0; underlines_named[i].param; ++i)
          if (underlines_named[i].len == len &&
              !strcmp(underlines_named[i].param, param))
            {
               fmt->underline = underlines_named[i].underline;
               fmt->underline2 = underlines_named[i].underline2;
               fmt->underline_dash = underlines_named[i].underline_dash;
               break;
            }
     }
   else if (cmd == strikethroughstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_strikethrough Strikethrough
         *
         * Sets if the text will be striked through. The value must be one of
         * the following:
         * @li "off" - No strikethrough
         * @li "on" - Strikethrough
         * @code
         * strikethrough=on/off
         * @endcode
         */
        if (len == 3 && !strcmp(param, "off"))
          fmt->strikethrough = 0;
        else if (len == 2 && !strcmp(param, "on"))
          fmt->strikethrough = 1;
     }
   else if (cmd == backingstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_backing Backing
         *
         * Sets if the text will have backing. The value must be one of
         * the following:
         * @li "off" - No backing
         * @li "on" - Backing
         * @code
         * backing=on/off
         * @endcode
         */
        if (len == 3 && !strcmp(param, "off"))
          fmt->backing = 0;
        else if (len == 2 && !strcmp(param, "on"))
          fmt->backing = 1;
     }
   else if (cmd == stylestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_style Style
         *
         * Sets the style of the text. The value must be a string composed of
         * two comma separated parts. The first part of the value sets the
         * appearance of the text, the second the position.
         *
         * The first part may be any of the following values:
         * @li "plain"
         * @li "off" - Alias for "plain"
         * @li "none" - Alias for "plain"
         * @li "shadow"
         * @li "outline"
         * @li "soft_outline"
         * @li "outline_shadow"
         * @li "outline_soft_shadow"
         * @li "glow"
         * @li "far_shadow"
         * @li "soft_shadow"
         * @li "far_soft_shadow"
         * The second part may be any of the following values:
         * @li "bottom_right"
         * @li "bottom"
         * @li "bottom_left"
         * @li "left"
         * @li "top_left"
         * @li "top"
         * @li "top_right"
         * @li "right"
         * @code
         * style=<appearance>,<position>
         * @endcode
         */
        const char *p;
        char *p1, *p2, *pp;

        p2 = alloca(len + 1);
        *p2 = 0;
        /* no comma */
        if (!strstr(param, ",")) p1 = (char*) param;
        else
          {
             p1 = alloca(len + 1);
             *p1 = 0;

             /* split string "str1,str2" into p1 and p2 (if we have more than
              * 1 str2 eg "str1,str2,str3,str4" then we don't care. p2 just
              * ends up being the last one as right now it's only valid to have
              * 1 comma and 2 strings */
             pp = p1;
             for (p = param; *p; p++)
               {
                  if (*p == ',')
                    {
                       *pp = 0;
                       pp = p2;
                       continue;
                    }
                  *pp = *p;
                  pp++;
               }
             *pp = 0;
          }
        if      (!strcmp(p1, "off"))                 fmt->style = EVAS_TEXT_STYLE_PLAIN;
        else if (!strcmp(p1, "none"))                fmt->style = EVAS_TEXT_STYLE_PLAIN;
        else if (!strcmp(p1, "plain"))               fmt->style = EVAS_TEXT_STYLE_PLAIN;
        else if (!strcmp(p1, "shadow"))              fmt->style = EVAS_TEXT_STYLE_SHADOW;
        else if (!strcmp(p1, "outline"))             fmt->style = EVAS_TEXT_STYLE_OUTLINE;
        else if (!strcmp(p1, "soft_outline"))        fmt->style = EVAS_TEXT_STYLE_SOFT_OUTLINE;
        else if (!strcmp(p1, "outline_shadow"))      fmt->style = EVAS_TEXT_STYLE_OUTLINE_SHADOW;
        else if (!strcmp(p1, "outline_soft_shadow")) fmt->style = EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW;
        else if (!strcmp(p1, "glow"))                fmt->style = EVAS_TEXT_STYLE_GLOW;
        else if (!strcmp(p1, "far_shadow"))          fmt->style = EVAS_TEXT_STYLE_FAR_SHADOW;
        else if (!strcmp(p1, "soft_shadow"))         fmt->style = EVAS_TEXT_STYLE_SOFT_SHADOW;
        else if (!strcmp(p1, "far_soft_shadow"))     fmt->style = EVAS_TEXT_STYLE_FAR_SOFT_SHADOW;
        else                                         fmt->style = EVAS_TEXT_STYLE_PLAIN;

        if (*p2)
          {
             if      (!strcmp(p2, "bottom_right")) EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT);
             else if (!strcmp(p2, "bottom"))       EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM);
             else if (!strcmp(p2, "bottom_left"))  EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_LEFT);
             else if (!strcmp(p2, "left"))         EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_LEFT);
             else if (!strcmp(p2, "top_left"))     EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_LEFT);
             else if (!strcmp(p2, "top"))          EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP);
             else if (!strcmp(p2, "top_right"))    EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_RIGHT);
             else if (!strcmp(p2, "right"))        EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_RIGHT);
             else                                  EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(fmt->style, EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT);
          }
     }
   else if (cmd == tabstopsstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_tabstops Tabstops
         *
         * Sets the size of the tab character. The value must be a number
         * greater than one.
         * @code
         * tabstops=<number>
         * @endcode
         */
        fmt->tabstops = atoi(param);
        if (fmt->tabstops < 1) fmt->tabstops = 1;
     }
   else if (cmd == linesizestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_linesize Line size
         *
         * Sets the size of line of text. The value should be a number.
         * @warning Setting this value sets linerelsize to 0%!
         * @code
         * linesize=<number>
         * @endcode
         */
        fmt->linesize = atoi(param);
        fmt->linerelsize = 0.0;
     }
   else if (cmd == linerelsizestr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_linerelsize Relative line size
         *
         * Sets the relative size of line of text. The value must be a
         * percentage.
         * @warning Setting this value sets linesize to 0!
         * @code
         * linerelsize=<number>%
         * @endcode
         */
        char *endptr = NULL;
        double val = strtod(param, &endptr);
        if (endptr)
          {
             while (*endptr && _is_white(*endptr))
               endptr++;
             if (*endptr == '%')
               {
                  fmt->linerelsize = val / 100.0;
                  fmt->linesize = 0;
                  if (fmt->linerelsize < 0.0) fmt->linerelsize = 0.0;
               }
          }
     }
   else if (cmd == linegapstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_linegap Line gap
         *
         * Sets the size of the line gap in text. The value should be a
         * number.
         * @warning Setting this value sets linerelgap to 0%!
         * @code
         * linegap=<number>
         * @endcode
         */
        fmt->linegap = atoi(param);
        fmt->linerelgap = 0.0;
     }
   else if (cmd == linerelgapstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_linerelgap Relative line gap
         *
         * Sets the relative size of the line gap in text. The value must be
         * a percentage.
         * @warning Setting this value sets linegap to 0!
         * @code
         * linerelgap=<number>%
         * @endcode
         */
        char *endptr = NULL;
        double val = strtod(param, &endptr);
        if (endptr)
          {
             while (*endptr && _is_white(*endptr))
               endptr++;
             if (*endptr == '%')
               {
                  fmt->linerelgap = val / 100.0;
                  fmt->linegap = 0;
                  if (fmt->linerelgap < 0.0) fmt->linerelgap = 0.0;
               }
          }
     }
   else if (cmd == itemstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_item Item
         *
         * Not implemented! Does nothing!
         * @code
         * item=<anything>
         * @endcode
         */
        // itemstr == replacement object items in textblock2 - inline imges
        // for example
     }
   else if (cmd == linefillstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_linefill Line fill
         *
         * Sets the size of the line fill in text. The value must be a
         * percentage.
         * @code
         * linefill=<number>%
         * @endcode
         */
        char *endptr = NULL;
        double val = strtod(param, &endptr);
        if (endptr)
          {
             while (*endptr && _is_white(*endptr))
               endptr++;
             if (*endptr == '%')
               {
                  fmt->linefill = val / 100.0;
                  if (fmt->linefill < 0.0) fmt->linefill = 0.0;
               }
          }
     }
   else if (cmd == ellipsisstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_ellipsis Ellipsis
         *
         * Sets ellipsis mode. The value should be a number. Any value smaller
         * than 0.0 or greater than 1.0 disables ellipsis.
         * A value of 0 means ellipsizing the leftmost portion of the text
         * first, 1 on the other hand the rightmost portion.
         * @code
         * ellipsis=<number>
         * @endcode
         */
        char *endptr = NULL;
        fmt->ellipsis = strtod(param, &endptr);
        if ((fmt->ellipsis < 0.0) || (fmt->ellipsis > 1.0))
          fmt->ellipsis = -1.0;
        else
          {
             Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
             o->have_ellipsis = 1;
          }
     }
   else if (cmd == underline_dash_widthstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_underline_dash_width Underline dash width
         *
         * Sets the width of the underline dash. The value should be a number.
         * @code
         * underline_dash_width=<number>
         * @endcode
         */
        fmt->underline_dash_width = atoi(param);
        if (fmt->underline_dash_width <= 0) fmt->underline_dash_width = 1;
     }
   else if (cmd == underline_dash_gapstr)
     {
        /**
         * @page evas_textblock2_style_page Evas Textblock2 Style Options
         *
         * @subsection evas_textblock2_style_underline_dash_gap Underline dash gap
         *
         * Sets the gap of the underline dash. The value should be a number.
         * @code
         * underline_dash_gap=<number>
         * @endcode
         */
        fmt->underline_dash_gap = atoi(param);
        if (fmt->underline_dash_gap <= 0) fmt->underline_dash_gap = 1;
     }
}

/**
 * @internal
 * Returns @c EINA_TRUE if the item is a format parameter, @c EINA_FALSE
 * otherwise.
 *
 * @param[in] item the item to check - Not NULL.
 */
static Eina_Bool
_format_is_param(const char *item)
{
   if (strchr(item, '=')) return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @internal
 * Parse the format item and populate key and val with the stringshares that
 * corrospond to the formats parsed.
 * It expects item to be of the structure:
 * "key=val"
 *
 * @param[in] item the item to parse - Not NULL.
 * @param[out] key where to store the key at - Not NULL.
 * @param[out] val where to store the value at - Not NULL.
 */
static void
_format_param_parse(const char *item, const char **key, Eina_Tmpstr **val)
{
   const char *start, *end;
   char *tmp, *s, *d;
   size_t len;

   start = strchr(item, '=');
   if (!start) return ;
   *key = eina_stringshare_add_length(item, start - item);
   start++; /* Advance after the '=' */
   /* If we can find a quote as the first non-space char,
    * our new delimiter is a quote, not a space. */
   while (*start == ' ')
      start++;

   if (*start == '\'')
     {
        start++;
        end = strchr(start, '\'');
        while ((end) && (end > start) && (end[-1] == '\\'))
          end = strchr(end + 1, '\'');
     }
   else
     {
        end = strchr(start, ' ');
        while ((end) && (end > start) && (end[-1] == '\\'))
          end = strchr(end + 1, ' ');
     }

   /* Null terminate before the spaces */
   if (end) len = end - start;
   else len = strlen(start);

   tmp = (char*) eina_tmpstr_add_length(start, len);
   if (!tmp) goto end;

   for (d = tmp, s = tmp; *s; s++)
     {
        if (*s != '\\')
          {
             *d = *s;
             d++;
          }
     }
   *d = '\0';

end:
   *val = tmp;
}

/**
 * @internal
 * This function parses the format passed in *s and advances s to point to the
 * next format item, while returning the current one as the return value.
 * @param s The current and returned position in the format string.
 * @return the current item parsed from the string.
 */
static const char *
_format_parse(const char **s)
{
   const char *p;
   const char *s1 = NULL, *s2 = NULL;
   Eina_Bool quote = EINA_FALSE;

   p = *s;
   if (*p == 0) return NULL;
   for (;;)
     {
        if (!s1)
          {
             if (*p != ' ') s1 = p;
             if (*p == 0) break;
          }
        else if (!s2)
          {
             if (*p == '\'')
               {
                  quote = !quote;
               }

             if ((p > *s) && (p[-1] != '\\') && (!quote))
               {
                  if (*p == ' ') s2 = p;
               }
             if (*p == 0) s2 = p;
          }
        p++;
        if (s1 && s2)
          {
             *s = s2;
             return s1;
          }
     }
   *s = p;
   return NULL;
}

/**
 * @internal
 * Parse the format str and populate fmt with the formats found.
 *
 * @param obj The evas object - Not NULL.
 * @param[out] fmt The format to populate - Not NULL.
 * @param[in] str the string to parse.- Not NULL.
 */
static void
_format_fill(Evas_Object *eo_obj, Evas_Object_Textblock2_Format *fmt, const char *str)
{
   const char *s;
   const char *item;

   s = str;

   /* get rid of any spaces at the start of the string */
   while (*s == ' ') s++;

   while ((item = _format_parse(&s)))
     {
        if (_format_is_param(item))
          {
             const char *key = NULL;
             Eina_Tmpstr *val = NULL;

             _format_param_parse(item, &key, &val);
             if ((key) && (val)) _format_command(eo_obj, fmt, key, val);
             eina_stringshare_del(key);
             eina_tmpstr_del(val);
          }
        else
          {
             /* immediate - not handled here */
          }
     }
}

/**
 * @internal
 * Duplicate a format and return the duplicate.
 *
 * @param obj The evas object - Not NULL.
 * @param[in] fmt The format to duplicate - Not NULL.
 * @return the copy of the format.
 */
static Evas_Object_Textblock2_Format *
_format_dup(Evas_Object *eo_obj, const Evas_Object_Textblock2_Format *fmt)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Object_Textblock2_Format *fmt2;

   fmt2 = calloc(1, sizeof(Evas_Object_Textblock2_Format));
   memcpy(fmt2, fmt, sizeof(Evas_Object_Textblock2_Format));
   fmt2->ref = 1;
   if (fmt->font.fdesc) fmt2->font.fdesc = evas_font_desc_ref(fmt->font.fdesc);

   if (fmt->font.source) fmt2->font.source = eina_stringshare_add(fmt->font.source);

   /* FIXME: just ref the font here... */
   fmt2->font.font = evas_font_load(obj->layer->evas->evas, fmt2->font.fdesc,
         fmt2->font.source, (int)(((double) fmt2->font.size) * obj->cur->scale));
   return fmt2;
}




typedef enum
{
   TEXTBLOCK2_POSITION_START,
   TEXTBLOCK2_POSITION_END,
   TEXTBLOCK2_POSITION_ELSE,
   TEXTBLOCK2_POSITION_SINGLE
} Textblock2_Position;

/**
 * @internal
 * @typedef Ctxt
 *
 * A pack of information that needed to be passed around in the layout engine,
 * packed for easier access.
 */
typedef struct _Ctxt Ctxt;

struct _Ctxt
{
   Evas_Object *obj;
   Evas_Textblock2_Data *o;

   Evas_Object_Textblock2_Paragraph *paragraphs;
   Evas_Object_Textblock2_Paragraph *par;
   Evas_Object_Textblock2_Line *ln;


   Eina_List *format_stack;
   Evas_Object_Textblock2_Format *fmt;

   int x, y;
   int w, h;
   int wmax, hmax;
   int ascent, descent;
   int maxascent, maxdescent;
   int marginl, marginr;
   int line_no;
   int underline_extend;
   int have_underline, have_underline2;
   double align, valign;
   Textblock2_Position position;
   Eina_Bool align_auto : 1;
   Eina_Bool width_changed : 1;
};

static void _layout_text_add_logical_item(Ctxt *c, Evas_Object_Textblock2_Text_Item *ti, Eina_List *rel);
static void _text_item_update_sizes(Ctxt *c, Evas_Object_Textblock2_Text_Item *ti);

/**
 * @internal
 * Adjust the ascent/descent of the format and context.
 *
 * @param maxascent The ascent to update - Not NUL.
 * @param maxdescent The descent to update - Not NUL.
 * @param fmt The format to adjust - NOT NULL.
 */
static void
_layout_format_ascent_descent_adjust(const Evas_Object *eo_obj,
      Evas_Coord *maxascent, Evas_Coord *maxdescent,
      Evas_Object_Textblock2_Format *fmt)
{
   int ascent, descent;
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);

   if (fmt->font.font)
     {
        ascent = *maxascent;
        descent = *maxdescent;
        if (fmt->linesize > 0)
          {
             if ((ascent + descent) < fmt->linesize)
               {
                  ascent = ((fmt->linesize * ascent) / (ascent + descent));
                  descent = fmt->linesize - ascent;
               }
          }
        else if (fmt->linerelsize > 0.0)
          {
             descent = descent * fmt->linerelsize;
             ascent = ascent * fmt->linerelsize;
          }
        descent += fmt->linegap;
        descent += ((ascent + descent) * fmt->linerelgap);
        if (*maxascent < ascent) *maxascent = ascent;
        if (*maxdescent < descent) *maxdescent = descent;
        if (fmt->linefill > 0.0)
          {
             int dh;

             dh = obj->cur->geometry.h - (*maxascent + *maxdescent);
             if (dh < 0) dh = 0;
             dh = fmt->linefill * dh;
             *maxdescent += dh / 2;
             *maxascent += dh - (dh / 2);
             // FIXME: set flag that says "if heigh changes - reformat"
          }
     }
}

static void
_layout_item_max_ascent_descent_calc(const Evas_Object *eo_obj,
      Evas_Coord *maxascent, Evas_Coord *maxdescent,
      Evas_Object_Textblock2_Item *it, Textblock2_Position position)
{
   void *fi = NULL;
   *maxascent = *maxdescent = 0;

   if (!it || !it->format || !it->format->font.font)
      return;

   if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
     {
        fi = _ITEM_TEXT(it)->text_props.font_instance;
     }

   if ((position == TEXTBLOCK2_POSITION_START) ||
         (position == TEXTBLOCK2_POSITION_SINGLE))
     {
        Evas_Coord asc = 0;

        if (fi)
          {
             asc = evas_common_font_instance_max_ascent_get(fi);
          }
        else
          {
             Evas_Object_Protected_Data *obj =
                eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
             asc = ENFN->font_max_ascent_get(ENDT,
                   it->format->font.font);
          }

        if (asc > *maxascent)
           *maxascent = asc;
     }

   if ((position == TEXTBLOCK2_POSITION_END) ||
         (position == TEXTBLOCK2_POSITION_SINGLE))
     {
        /* Calculate max descent. */
        Evas_Coord desc = 0;

        if (fi)
          {
             desc = evas_common_font_instance_max_descent_get(fi);
          }
        else
          {
             Evas_Object_Protected_Data *obj =
                eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
             desc = ENFN->font_max_descent_get(ENDT,
                   it->format->font.font);
          }

        if (desc > *maxdescent)
           *maxdescent = desc;
     }
}

/**
 * @internal
 * Adjust the ascent/descent of the item and context.
 *
 * @param ascent The ascent to update - Not NUL.
 * @param descent The descent to update - Not NUL.
 * @param it The format to adjust - NOT NULL.
 * @param position The position inside the textblock2
 */
static void
_layout_item_ascent_descent_adjust(const Evas_Object *eo_obj,
      Evas_Coord *ascent, Evas_Coord *descent,
      Evas_Object_Textblock2_Item *it, Evas_Object_Textblock2_Format *fmt)
{
   void *fi = NULL;
   int asc = 0, desc = 0;

   if ((!it || !it->format || !it->format->font.font) &&
         (!fmt || !fmt->font.font))
     {
        return;
     }

   if (it)
     {
        fmt = it->format;

        if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
          {
             fi = _ITEM_TEXT(it)->text_props.font_instance;
          }
     }

   if (fi)
     {
        asc = evas_common_font_instance_ascent_get(fi);
        desc = evas_common_font_instance_descent_get(fi);
     }
   else
     {
        if (fmt)
          {
             Evas_Object_Protected_Data *obj =
               eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
             asc = ENFN->font_ascent_get(ENDT, fmt->font.font);
             desc = ENFN->font_descent_get(ENDT, fmt->font.font);
          }
     }

   if (asc > *ascent) *ascent = asc;
   if (desc > *descent) *descent = desc;

   if (fmt) _layout_format_ascent_descent_adjust(eo_obj, ascent, descent, fmt);
}

/**
 * @internal
 * Create a new line using the info from the format and update the format
 * and context.
 *
 * @param c The context to work on - Not NULL.
 * @param fmt The format to use info from - NOT NULL.
 */
static void
_layout_line_new(Ctxt *c, Evas_Object_Textblock2_Format *fmt)
{
   c->ln = calloc(1, sizeof(Evas_Object_Textblock2_Line));
   c->align = fmt->halign;
   c->align_auto = fmt->halign_auto;
   c->marginl = fmt->margin.l;
   c->marginr = fmt->margin.r;
   c->par->lines = (Evas_Object_Textblock2_Line *)eina_inlist_append(EINA_INLIST_GET(c->par->lines), EINA_INLIST_GET(c->ln));
   c->x = 0;
   c->ascent = c->descent = 0;
   c->maxascent = c->maxdescent = 0;
   c->ln->line_no = -1;
   c->ln->par = c->par;
}

static inline Evas_Object_Textblock2_Paragraph *
_layout_find_paragraph_by_y(Evas_Textblock2_Data *o, Evas_Coord y)
{
   Evas_Object_Textblock2_Paragraph *start, *par;
   int i;

   start = o->paragraphs;

   for (i = 0 ; i < TEXTBLOCK2_PAR_INDEX_SIZE ; i++)
     {
        if (!o->par_index[i] || (o->par_index[i]->y > y))
          {
             break;
          }
        start = o->par_index[i];
     }

   EINA_INLIST_FOREACH(start, par)
     {
        if ((par->y <= y) && (y < par->y + par->h))
           return par;
     }

   return NULL;
}

static inline Evas_Object_Textblock2_Paragraph *
_layout_find_paragraph_by_line_no(Evas_Textblock2_Data *o, int line_no)
{
   Evas_Object_Textblock2_Paragraph *start, *par;
   int i;

   start = o->paragraphs;

   for (i = 0 ; i < TEXTBLOCK2_PAR_INDEX_SIZE ; i++)
     {
        if (!o->par_index[i] || (o->par_index[i]->line_no > line_no))
          {
             break;
          }
        start = o->par_index[i];
     }

   EINA_INLIST_FOREACH(start, par)
     {
        Evas_Object_Textblock2_Paragraph *npar =
           (Evas_Object_Textblock2_Paragraph *) EINA_INLIST_GET(par)->next;
        if ((par->line_no <= line_no) &&
              (!npar || (line_no < npar->line_no)))
           return par;
     }

   return NULL;
}
/* End of rbtree index functios */

/**
 * @internal
 * Create a new layout paragraph.
 * If c->par is not NULL, the paragraph is appended/prepended according
 * to the append parameter. If it is NULL, the paragraph is appended at
 * the end of the list.
 *
 * @param c The context to work on - Not NULL.
 * @param n the associated text node
 * @param append true to append, false to prpend.
 */
static void
_layout_paragraph_new(Ctxt *c, Evas_Object_Textblock2_Node_Text *n,
      Eina_Bool append)
{
   Evas_Object_Textblock2_Paragraph *rel_par = c->par;
   c->par = calloc(1, sizeof(Evas_Object_Textblock2_Paragraph));
   if (append || !rel_par)
      c->paragraphs = (Evas_Object_Textblock2_Paragraph *)
         eina_inlist_append_relative(EINA_INLIST_GET(c->paragraphs),
               EINA_INLIST_GET(c->par),
               EINA_INLIST_GET(rel_par));
   else
      c->paragraphs = (Evas_Object_Textblock2_Paragraph *)
         eina_inlist_prepend_relative(EINA_INLIST_GET(c->paragraphs),
               EINA_INLIST_GET(c->par),
               EINA_INLIST_GET(rel_par));

   c->ln = NULL;
   c->par->text_node = n;
   if (n)
      n->par = c->par;
   c->par->line_no = -1;
   c->par->visible = 1;
   c->o->num_paragraphs++;
}

#ifdef BIDI_SUPPORT
/**
 * @internal
 * Update bidi paragraph props.
 *
 * @param par The paragraph to update
 */
static inline void
_layout_update_bidi_props(const Evas_Textblock2_Data *o,
      Evas_Object_Textblock2_Paragraph *par)
{
   if (par->text_node)
     {
        const Eina_Unicode *text;
        int *segment_idxs = NULL;
        text = eina_ustrbuf_string_get(par->text_node->unicode);

        if (o->bidi_delimiters)
           segment_idxs = evas_bidi_segment_idxs_get(text, o->bidi_delimiters);

        evas_bidi_paragraph_props_unref(par->bidi_props);
        par->bidi_props = evas_bidi_paragraph_props_get(text,
              eina_ustrbuf_length_get(par->text_node->unicode),
              segment_idxs);
        par->direction = EVAS_BIDI_PARAGRAPH_DIRECTION_IS_RTL(par->bidi_props) ?
           EVAS_BIDI_DIRECTION_RTL : EVAS_BIDI_DIRECTION_LTR;
        par->is_bidi = !!par->bidi_props;
        if (segment_idxs) free(segment_idxs);
     }
}
#endif


/**
 * @internal
 * Free the visual lines in the paragraph (logical items are kept)
 */
static void
_paragraph_clear(const Evas_Object *obj EINA_UNUSED,
      Evas_Object_Textblock2_Paragraph *par)
{
   while (par->lines)
     {
        Evas_Object_Textblock2_Line *ln;

        ln = (Evas_Object_Textblock2_Line *) par->lines;
        par->lines = (Evas_Object_Textblock2_Line *)eina_inlist_remove(EINA_INLIST_GET(par->lines), EINA_INLIST_GET(par->lines));
        _line_free(ln);
     }
}

/**
 * @internal
 * Free the layout paragraph and all of it's lines and logical items.
 */
static void
_paragraph_free(const Evas_Object *eo_obj, Evas_Object_Textblock2_Paragraph *par)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   _paragraph_clear(eo_obj, par);

     {
        Evas_Object_Textblock2_Item *it;
        EINA_LIST_FREE(par->logical_items, it)
          {
             _item_free(eo_obj, NULL, it);
          }
     }
#ifdef BIDI_SUPPORT
   if (par->bidi_props)
      evas_bidi_paragraph_props_unref(par->bidi_props);
#endif
   /* If we are the active par of the text node, set to NULL */
   if (par->text_node && (par->text_node->par == par))
      par->text_node->par = NULL;

   o->num_paragraphs--;

   free(par);
}

/**
 * @internal
 * Clear all the paragraphs from the inlist pars.
 *
 * @param obj the evas object - Not NULL.
 * @param pars the paragraphs to clean - Not NULL.
 */
static void
_paragraphs_clear(const Evas_Object *eo_obj, Evas_Object_Textblock2_Paragraph *pars)
{
   Evas_Object_Textblock2_Paragraph *par;

   EINA_INLIST_FOREACH(EINA_INLIST_GET(pars), par)
     {
        _paragraph_clear(eo_obj, par);
     }
}

/**
 * @internal
 * Free the paragraphs from the inlist pars, the difference between this and
 * _paragraphs_clear is that the latter keeps the logical items and the par
 * items, while the former frees them as well.
 *
 * @param obj the evas object - Not NULL.
 * @param pars the paragraphs to clean - Not NULL.
 */
static void
_paragraphs_free(const Evas_Object *eo_obj, Evas_Object_Textblock2_Paragraph *pars)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

   o->num_paragraphs = 0;

   while (pars)
     {
        Evas_Object_Textblock2_Paragraph *par;

        par = (Evas_Object_Textblock2_Paragraph *) pars;
        pars = (Evas_Object_Textblock2_Paragraph *)eina_inlist_remove(EINA_INLIST_GET(pars), EINA_INLIST_GET(par));
        _paragraph_free(eo_obj, par);
     }
}

/**
 * @internal
 * Push fmt to the format stack, if fmt is NULL, will push a default item.
 *
 * @param c the context to work on - Not NULL.
 * @param fmt the format to push.
 * @see _layout_format_pop()
 */
static Evas_Object_Textblock2_Format *
_layout_format_push(Ctxt *c, Evas_Object_Textblock2_Format *fmt)
{
   if (fmt)
     {
        fmt = _format_dup(c->obj, fmt);
        c->format_stack  = eina_list_prepend(c->format_stack, fmt);
     }
   else
     {
        fmt = calloc(1, sizeof(Evas_Object_Textblock2_Format));
        c->format_stack  = eina_list_prepend(c->format_stack, fmt);
        fmt->ref = 1;
        fmt->halign = 0.0;
        fmt->halign_auto = EINA_TRUE;
        fmt->valign = -1.0;
        fmt->style = EVAS_TEXT_STYLE_PLAIN;
        fmt->tabstops = 32;
        fmt->linesize = 0;
        fmt->linerelsize = 0.0;
        fmt->linegap = 0;
        fmt->underline_dash_width = 6;
        fmt->underline_dash_gap = 2;
        fmt->linerelgap = 0.0;
        fmt->ellipsis = -1;
     }
   return fmt;
}

#define VSIZE_FULL 0
#define VSIZE_ASCENT 1

#define SIZE 0
#define SIZE_ABS 1
#define SIZE_REL 2

/**
 * @internal
 * Get the current line's alignment from the context.
 *
 * @param c the context to work on - Not NULL.
 */
static inline double
_layout_line_align_get(Ctxt *c)
{
#ifdef BIDI_SUPPORT
   if (c->align_auto && c->ln)
     {
        if (c->ln->items && c->ln->items->text_node &&
              (c->ln->par->direction == EVAS_BIDI_DIRECTION_RTL))
          {
             /* Align right*/
             return 1.0;
          }
        else
          {
             /* Align left */
             return 0.0;
          }
     }
#endif
   return c->align;
}

#ifdef BIDI_SUPPORT
/**
 * @internal
 * Reorder the items in visual order
 *
 * @param line the line to reorder
 */
static void
_layout_line_reorder(Evas_Object_Textblock2_Line *line)
{
   /*FIXME: do it a bit more efficient - not very efficient ATM. */
   Evas_Object_Textblock2_Item *it;
   EvasBiDiStrIndex *v_to_l = NULL;
   Evas_Coord x;
   size_t start, end;
   size_t len;

   if (line->items && line->items->text_node &&
         line->par->bidi_props)
     {
        Evas_BiDi_Paragraph_Props *props;
        props = line->par->bidi_props;
        start = end = line->items->text_pos;

        /* Find the first and last positions in the line */

        EINA_INLIST_FOREACH(line->items, it)
          {
             if (it->text_pos < start)
               {
                  start = it->text_pos;
               }
             else
               {
                  int tlen;
                  tlen = (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ?
                     _ITEM_TEXT(it)->text_props.text_len : 1;
                  if (it->text_pos + tlen > end)
                    {
                       end = it->text_pos + tlen;
                    }
               }
          }

        len = end - start;
        evas_bidi_props_reorder_line(NULL, start, len, props, &v_to_l);

        /* Update visual pos */
          {
             Evas_Object_Textblock2_Item *i;
             i = line->items;
             while (i)
               {
                  i->visual_pos = evas_bidi_position_logical_to_visual(
                        v_to_l, len, i->text_pos - start);
                  i = (Evas_Object_Textblock2_Item *) EINA_INLIST_GET(i)->next;
               }
          }

        /*FIXME: not very efficient, sort the items arrays. Anyhow, should only
         * reorder if it's a bidi paragraph */
          {
             Evas_Object_Textblock2_Item *i, *j, *min;
             i = line->items;
             while (i)
               {
                  min = i;
                  EINA_INLIST_FOREACH(i, j)
                    {
                       if (j->visual_pos < min->visual_pos)
                         {
                            min = j;
                         }
                    }
                  if (min != i)
                    {
                       line->items = (Evas_Object_Textblock2_Item *) eina_inlist_remove(EINA_INLIST_GET(line->items), EINA_INLIST_GET(min));
                       line->items = (Evas_Object_Textblock2_Item *) eina_inlist_prepend_relative(EINA_INLIST_GET(line->items), EINA_INLIST_GET(min), EINA_INLIST_GET(i));
                    }

                  i = (Evas_Object_Textblock2_Item *) EINA_INLIST_GET(min)->next;
               }
          }
     }

   if (v_to_l) free(v_to_l);
   x = 0;
   EINA_INLIST_FOREACH(line->items, it)
     {
        it->x = x;
        x += it->adv;
     }
}
#endif

/* FIXME: doc */
static void
_layout_calculate_format_item_size(const Evas_Object *eo_obj,
      const Evas_Object_Textblock2_Format_Item *fi,
      Evas_Coord *maxascent, Evas_Coord *maxdescent,
      Evas_Coord *_y, Evas_Coord *_w, Evas_Coord *_h)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   /* Adjust sizes according to current line height/scale */
   Evas_Coord w, h;
   const char *p, *s;

   s = fi->item;
   w = fi->parent.w;
   h = fi->parent.h;
   if (!s)
     {
        *_w = w;
        *_h = h;
        return;
     }
   switch (fi->size)
     {
      case SIZE:
         p = strstr(s, " size=");
         if (p)
           {
              p += 6;
              if (sscanf(p, "%ix%i", &w, &h) == 2)
                {
                   w = w * obj->cur->scale;
                   h = h * obj->cur->scale;
                }
           }
         break;
      case SIZE_REL:
         p = strstr(s, " relsize=");
         p += 9;
         if (sscanf(p, "%ix%i", &w, &h) == 2)
           {
              int sz = 1;
              if (fi->vsize == VSIZE_FULL)
                {
                   sz = *maxdescent + *maxascent;
                }
              else if (fi->vsize == VSIZE_ASCENT)
                {
                   sz = *maxascent;
                }
              w = (w * sz) / h;
              h = sz;
           }
         break;
      case SIZE_ABS:
         /* Nothing to do */
      default:
         break;
     }

   switch (fi->size)
     {
      case SIZE:
      case SIZE_ABS:
         switch (fi->vsize)
           {
            case VSIZE_FULL:
               if (h > (*maxdescent + *maxascent))
                 {
                    *maxascent += h - (*maxdescent + *maxascent);
                    *_y = -*maxascent;
                 }
               else
                  *_y = -(h - *maxdescent);
               break;
            case VSIZE_ASCENT:
               if (h > *maxascent)
                 {
                    *maxascent = h;
                    *_y = -h;
                 }
               else
                  *_y = -h;
               break;
            default:
               break;
           }
         break;
      case SIZE_REL:
         switch (fi->vsize)
           {
            case VSIZE_FULL:
            case VSIZE_ASCENT:
               *_y = -*maxascent;
               break;
            default:
               break;
           }
         break;
      default:
         break;
     }

   *_w = w;
   *_h = h;
}

static Evas_Coord
_layout_last_line_max_descent_adjust_calc(Ctxt *c, const Evas_Object_Textblock2_Paragraph *last_vis_par)
{
   if (last_vis_par->lines)
     {
        Evas_Object_Textblock2_Line *ln = (Evas_Object_Textblock2_Line *)
           EINA_INLIST_GET(last_vis_par->lines)->last;
        Evas_Object_Textblock2_Item *it;

        EINA_INLIST_FOREACH(ln->items, it)
          {
             if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
               {
                  Evas_Coord asc = 0, desc = 0;
                  Evas_Coord maxasc = 0, maxdesc = 0;
                  _layout_item_ascent_descent_adjust(c->obj, &asc, &desc,
                        it, it->format);
                  _layout_item_max_ascent_descent_calc(c->obj, &maxasc, &maxdesc,
                        it, c->position);

                  if (desc > c->descent)
                     c->descent = desc;
                  if (maxdesc > c->maxdescent)
                     c->maxdescent = maxdesc;
               }
          }

        if (c->maxdescent > c->descent)
          {
             return c->maxdescent - c->descent;
          }
     }

   return 0;
}

/**
 * @internal
 * Order the items in the line, update it's properties and update it's
 * corresponding paragraph.
 *
 * @param c the context to work on - Not NULL.
 * @param fmt the format to use.
 * @param add_line true if we should create a line, false otherwise.
 */
static void
_layout_line_finalize(Ctxt *c, Evas_Object_Textblock2_Format *fmt)
{
   Evas_Object_Textblock2_Item *it;
   Evas_Coord x = 0;

   /* If there are no text items yet, calc ascent/descent
    * according to the current format. */
   if (c->ascent + c->descent == 0)
      _layout_item_ascent_descent_adjust(c->obj, &c->ascent, &c->descent,
            NULL, fmt);

#ifdef BIDI_SUPPORT
   _layout_line_reorder(c->ln);
#endif

   /* Adjust all the item sizes according to the final line size,
    * and update the x positions of all the items of the line. */
   EINA_INLIST_FOREACH(c->ln->items, it)
     {
        if (it->type == EVAS_TEXTBLOCK2_ITEM_FORMAT)
          {
             Evas_Object_Textblock2_Format_Item *fi = _ITEM_FORMAT(it);
             if (!fi->formatme) goto loop_advance;
             _layout_calculate_format_item_size(c->obj, fi, &c->ascent,
                   &c->descent, &fi->y, &fi->parent.w, &fi->parent.h);
             fi->parent.adv = fi->parent.w;
          }
        else
          {
             Evas_Coord asc = 0, desc = 0;
             Evas_Coord maxasc = 0, maxdesc = 0;
             _layout_item_ascent_descent_adjust(c->obj, &asc, &desc,
                   it, it->format);
             _layout_item_max_ascent_descent_calc(c->obj, &maxasc, &maxdesc,
                   it, c->position);

             if (asc > c->ascent)
                c->ascent = asc;
             if (desc > c->descent)
                c->descent = desc;
             if (maxasc > c->maxascent)
                c->maxascent = maxasc;
             if (maxdesc > c->maxdescent)
                c->maxdescent = maxdesc;
          }

loop_advance:
        it->x = x;
        x += it->adv;

        if ((it->w > 0) && ((it->x + it->w) > c->ln->w)) c->ln->w = it->x + it->w;
     }

   c->ln->y = c->y - c->par->y;
   c->ln->h = c->ascent + c->descent;

   /* Handle max ascent and descent if at the edges */
     {
        /* If it's the start, offset the line according to the max ascent. */
        if (((c->position == TEXTBLOCK2_POSITION_START) ||
                 (c->position == TEXTBLOCK2_POSITION_SINGLE))
              && (c->maxascent > c->ascent))
          {
             Evas_Coord ascdiff;

             ascdiff = c->maxascent - c->ascent;
             c->ln->y += ascdiff;
             c->y += ascdiff;
             c->ln->y += c->o->style_pad.t;
             c->y += c->o->style_pad.t;
          }
     }

   c->ln->baseline = c->ascent;
   /* FIXME: Actually needs to be adjusted using the actual font value.
    * Also, underline_extend is actually not being used. */
   if (c->have_underline2)
     {
        if (c->descent < 4) c->underline_extend = 4 - c->descent;
     }
   else if (c->have_underline)
     {
        if (c->descent < 2) c->underline_extend = 2 - c->descent;
     }
   c->ln->line_no = c->line_no - c->ln->par->line_no;
   c->line_no++;
   c->y += c->ascent + c->descent;
   if (c->w >= 0)
     {
        /* c->o->style_pad.r is already included in the line width, so it's
         * not used in this calculation. . */
        c->ln->x = c->marginl + c->o->style_pad.l +
           ((c->w - c->ln->w - c->o->style_pad.l -
             c->marginl - c->marginr) * _layout_line_align_get(c));
     }
   else
     {
        c->ln->x = c->marginl + c->o->style_pad.l;
     }

   c->par->h = c->ln->y + c->ln->h;
   if (c->ln->w > c->par->w)
     c->par->w = c->ln->w;

     {
        Evas_Coord new_wmax = c->ln->w +
           c->marginl + c->marginr - (c->o->style_pad.l + c->o->style_pad.r);
        if (new_wmax > c->wmax)
           c->wmax = new_wmax;
     }

   if (c->position == TEXTBLOCK2_POSITION_START)
      c->position = TEXTBLOCK2_POSITION_ELSE;
}

/**
 * @internal
 * Create a new line and append it to the lines in the context.
 *
 * @param c the context to work on - Not NULL.
 * @param fmt the format to use.
 * @param add_line true if we should create a line, false otherwise.
 */
static void
_layout_line_advance(Ctxt *c, Evas_Object_Textblock2_Format *fmt)
{
   _layout_line_finalize(c, fmt);
   _layout_line_new(c, fmt);
}

/**
 * @internal
 * Create a new text layout item from the string and the format.
 *
 * @param c the context to work on - Not NULL.
 * @param fmt the format to use.
 * @param str the string to use.
 * @param len the length of the string.
 */
static Evas_Object_Textblock2_Text_Item *
_layout_text_item_new(Ctxt *c EINA_UNUSED, Evas_Object_Textblock2_Format *fmt)
{
   Evas_Object_Textblock2_Text_Item *ti;

   ti = calloc(1, sizeof(Evas_Object_Textblock2_Text_Item));
   ti->parent.format = fmt;
   ti->parent.format->ref++;
   ti->parent.type = EVAS_TEXTBLOCK2_ITEM_TEXT;
   return ti;
}

/**
 * @internal
 * Return the cutoff of the text in the text item.
 *
 * @param c the context to work on - Not NULL.
 * @param fmt the format to use. - Not NULL.
 * @param it the item to check - Not null.
 * @return -1 if there is no cutoff (either because there is really none,
 * or because of an error), cutoff index on success.
 */
static int
_layout_text_cutoff_get(Ctxt *c, Evas_Object_Textblock2_Format *fmt,
      const Evas_Object_Textblock2_Text_Item *ti)
{
   if (fmt->font.font)
     {
        Evas_Coord x;
        x = c->w - c->o->style_pad.l - c->o->style_pad.r - c->marginl -
           c->marginr - c->x - ti->x_adjustment;
        if (x < 0)
          x = 0;
        Evas_Object_Protected_Data *obj = eo_data_scope_get(c->obj, EVAS_OBJECT_CLASS);
        return ENFN->font_last_up_to_pos(ENDT, fmt->font.font,
              &ti->text_props, x, 0);
     }
   return -1;
}

/**
 * @internal
 * Split before cut, and strip if str[cut - 1] is a whitespace.
 *
 * @param c the context to work on - Not NULL.
 * @param ti the item to cut - not null.
 * @param lti the logical list item of the item.
 * @param cut the cut index.
 * @return the second (newly created) item.
 */
static Evas_Object_Textblock2_Text_Item *
_layout_item_text_split_strip_white(Ctxt *c,
      Evas_Object_Textblock2_Text_Item *ti, Eina_List *lti, size_t cut)
{
   const Eina_Unicode *ts;
   Evas_Object_Textblock2_Text_Item *new_ti = NULL, *white_ti = NULL;

   ts = GET_ITEM_TEXT(ti);

   if (!IS_AT_END(ti, cut) && (ti->text_props.text_len > 0))
     {
        new_ti = _layout_text_item_new(c, ti->parent.format);
        new_ti->parent.text_node = ti->parent.text_node;
        new_ti->parent.text_pos = ti->parent.text_pos + cut;
        new_ti->parent.merge = EINA_TRUE;

        evas_common_text_props_split(&ti->text_props,
                                     &new_ti->text_props, cut);
        _layout_text_add_logical_item(c, new_ti, lti);
     }

   /* Strip the previous white if needed */
   if ((cut >= 1) && _is_white(ts[cut - 1]) && (ti->text_props.text_len > 0))
     {
        if (cut - 1 > 0)
          {
             size_t white_cut = cut - 1;
             white_ti = _layout_text_item_new(c, ti->parent.format);
             white_ti->parent.text_node = ti->parent.text_node;
             white_ti->parent.text_pos = ti->parent.text_pos + white_cut;
             white_ti->parent.merge = EINA_TRUE;
             white_ti->parent.visually_deleted = EINA_TRUE;

             evas_common_text_props_split(&ti->text_props,
                   &white_ti->text_props, white_cut);
             _layout_text_add_logical_item(c, white_ti, lti);
          }
        else
          {
             /* Mark this one as the visually deleted. */
             ti->parent.visually_deleted = EINA_TRUE;
          }
     }

   if (new_ti || white_ti)
     {
        _text_item_update_sizes(c, ti);
     }
   return new_ti;
}

/**
 * @internal
 * Merge item2 into item1 and free item2.
 *
 * @param c the context to work on - Not NULL.
 * @param item1 the item to copy to
 * @param item2 the item to copy from
 */
static void
_layout_item_merge_and_free(Ctxt *c,
      Evas_Object_Textblock2_Text_Item *item1,
      Evas_Object_Textblock2_Text_Item *item2)
{
   evas_common_text_props_merge(&item1->text_props,
         &item2->text_props);

   _text_item_update_sizes(c, item1);

   item1->parent.merge = EINA_FALSE;
   item1->parent.visually_deleted = EINA_FALSE;

   _item_free(c->obj, NULL, _ITEM(item2));
}

/**
 * @internal
 * Calculates an item's size.
 *
 * @param c the context
 * @param it the item itself.
 */
static void
_text_item_update_sizes(Ctxt *c, Evas_Object_Textblock2_Text_Item *ti)
{
   int tw, th, inset, advw;
   const Evas_Object_Textblock2_Format *fmt = ti->parent.format;
   int shad_sz = 0, shad_dst = 0, out_sz = 0;
   int dx = 0, minx = 0, maxx = 0, shx1, shx2;

   tw = th = 0;
   Evas_Object_Protected_Data *obj = eo_data_scope_get(c->obj, EVAS_OBJECT_CLASS);
   if (fmt->font.font)
     ENFN->font_string_size_get(ENDT, fmt->font.font,
           &ti->text_props, &tw, &th);
   inset = 0;
   if (fmt->font.font)
     inset = ENFN->font_inset_get(ENDT, fmt->font.font,
           &ti->text_props);
   advw = 0;
   if (fmt->font.font)
      advw = ENFN->font_h_advance_get(ENDT, fmt->font.font,
           &ti->text_props);

   /* These adjustments are calculated and thus heavily linked to those in
    * textblock2_render!!! Don't change one without the other. */

   switch (ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC)
     {
      case EVAS_TEXT_STYLE_SHADOW:
        shad_dst = 1;
        break;
      case EVAS_TEXT_STYLE_OUTLINE_SHADOW:
      case EVAS_TEXT_STYLE_FAR_SHADOW:
        shad_dst = 2;
        out_sz = 1;
        break;
      case EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW:
        shad_dst = 1;
        shad_sz = 2;
        out_sz = 1;
        break;
      case EVAS_TEXT_STYLE_FAR_SOFT_SHADOW:
        shad_dst = 2;
        shad_sz = 2;
        break;
      case EVAS_TEXT_STYLE_SOFT_SHADOW:
        shad_dst = 1;
        shad_sz = 2;
        break;
      case EVAS_TEXT_STYLE_GLOW:
      case EVAS_TEXT_STYLE_SOFT_OUTLINE:
        out_sz = 2;
        break;
      case EVAS_TEXT_STYLE_OUTLINE:
        out_sz = 1;
        break;
      default:
        break;
     }
   switch (ti->parent.format->style & EVAS_TEXT_STYLE_MASK_SHADOW_DIRECTION)
     {
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_LEFT:
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_LEFT:
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_LEFT:
        dx = -1;
        break;
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT:
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_RIGHT:
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_RIGHT:
        dx = 1;
        break;
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP:
      case EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM:
      default:
        dx = 0;
        break;
     }
   minx = -out_sz;
   maxx = out_sz;
   shx1 = dx * shad_dst;
   shx1 -= shad_sz;
   shx2 = dx * shad_dst;
   shx2 += shad_sz;
   if (shx1 < minx) minx = shx1;
   if (shx2 > maxx) maxx = shx2;
   inset += -minx;
   ti->x_adjustment = maxx - minx;
   
   ti->inset = inset;
   ti->parent.w = tw + ti->x_adjustment;
   ti->parent.h = th;
   ti->parent.adv = advw;
   ti->parent.x = 0;
}

/**
 * @internal
 * Adds the item to the list, updates the item's properties (e.g, x,w,h)
 *
 * @param c the context
 * @param it the item itself.
 * @param rel item ti will be appened after, NULL = last.
 */
static void
_layout_text_add_logical_item(Ctxt *c, Evas_Object_Textblock2_Text_Item *ti,
      Eina_List *rel)
{
   _text_item_update_sizes(c, ti);

   c->par->logical_items = eina_list_append_relative_list(
         c->par->logical_items, ti, rel);
}

static void
_layout_text_append_add_logical_item(Ctxt *c, Evas_Object_Textblock2_Text_Item *ti,
      Eina_List *rel)
{
   _text_item_update_sizes(c, ti);

   if (rel)
     {
        c->par->logical_items = eina_list_prepend_relative_list(
              c->par->logical_items, ti, rel);
     }
   else
     {
        c->par->logical_items = eina_list_append(
              c->par->logical_items, ti);
     }
}

typedef struct {
     EINA_INLIST;
     Evas_Object_Textblock2_Format *format;
     size_t start;
     int off;
} Layout_Text_Append_Queue;

/**
 * @internal
 * Appends the text from node n starting at start ending at off to the layout.
 * It uses the fmt for the formatting.
 *
 * @param c the current context- NOT NULL.
 * @param fmt the format to use.
 * @param n the text node. - Not null.
 * @param start the start position. - in range.
 * @param off the offset - start + offset in range. if offset is -1, it'll add everything to the end of the string if offset = 0 it'll return with doing nothing.
 */
static void
_layout_text_append(Ctxt *c, Layout_Text_Append_Queue *queue, Evas_Object_Textblock2_Node_Text *n, int start, int off, Eina_List *rel)
{
   const Eina_Unicode *str = EINA_UNICODE_EMPTY_STRING;
   const Eina_Unicode *tbase;
   Evas_Object_Textblock2_Text_Item *ti;
   size_t cur_len = 0;

   /* prepare a working copy of the string, either filled by the repch or
    * filled with the true values */
   if (n)
     {
        int len;
        int orig_off = off;

        /* Figure out if we want to bail, work with an empty string,
         * or continue with a slice of the passed string */
        len = eina_ustrbuf_length_get(n->unicode);
        if (off == 0) return;
        else if (off < 0) off = len - start;

        if (start < 0)
          {
             start = 0;
          }
        else if ((start == 0) && (off == 0) && (orig_off == -1))
          {
             /* Special case that means that we need to add an empty
              * item */
             str = EINA_UNICODE_EMPTY_STRING;
             goto skip;
          }
        else if ((start >= len) || (start + off > len))
          {
             return;
          }

        str = eina_ustrbuf_string_get(n->unicode) + start;

        cur_len = off;
     }

skip:
   tbase = str;

   /* If there's no parent text node, only create an empty item */
   if (!n)
     {
        ti = _layout_text_item_new(c, queue->format);
        ti->parent.text_node = NULL;
        ti->parent.text_pos = 0;
        _layout_text_append_add_logical_item(c, ti, rel);

        return;
     }

   while (cur_len > 0)
     {
        Evas_Font_Instance *script_fi = NULL;
        int script_len, tmp_cut;
        Evas_Script_Type script;
        size_t str_start = start + str - tbase;

        script_len = cur_len;

        tmp_cut = evas_common_language_script_end_of_run_get(str,
              c->par->bidi_props, str_start, script_len);

        if (tmp_cut > 0)
          {
             script_len = tmp_cut;
          }
        cur_len -= script_len;

        script = evas_common_language_script_type_get(str, script_len);

        Evas_Object_Protected_Data *obj = eo_data_scope_get(c->obj, EVAS_OBJECT_CLASS);
        while (script_len > 0)
          {
             Evas_Font_Instance *cur_fi = NULL;
             size_t run_start;
             int run_len = script_len;
             ti = _layout_text_item_new(c, queue->format);
             ti->parent.text_node = n;
             ti->parent.text_pos = run_start = start + str - tbase;

             if (ti->parent.format->font.font)
               {
                  run_len = ENFN->font_run_end_get(ENDT,
                        ti->parent.format->font.font, &script_fi, &cur_fi,
                        script, str, script_len);
               }

             evas_common_text_props_bidi_set(&ti->text_props,
                   c->par->bidi_props, ti->parent.text_pos);
             evas_common_text_props_script_set(&ti->text_props, script);

             if (cur_fi)
               {
                  ENFN->font_text_props_info_create(ENDT,
                        cur_fi, str, &ti->text_props, c->par->bidi_props,
                        ti->parent.text_pos, run_len, EVAS_TEXT_PROPS_MODE_SHAPE);
               }

             while ((queue->start + queue->off) < (run_start + run_len))
               {
                  Evas_Object_Textblock2_Text_Item *new_ti;

                  /* There must be a next because of the test in the while. */
                  queue = (Layout_Text_Append_Queue *) EINA_INLIST_GET(queue)->next;

                  new_ti = _layout_text_item_new(c, queue->format);
                  new_ti->parent.text_node = ti->parent.text_node;
                  new_ti->parent.text_pos = queue->start;

                  evas_common_text_props_split(&ti->text_props, &new_ti->text_props,
                        new_ti->parent.text_pos - ti->parent.text_pos);

                  _layout_text_append_add_logical_item(c, ti, rel);
                  ti = new_ti;
               }

             _layout_text_append_add_logical_item(c, ti, rel);

             str += run_len;
             script_len -= run_len;
          }
     }
}

/**
 * @internal
 * Should be call after we finish filling a format.
 * FIXME: doc.
 */
static void
_format_finalize(Evas_Object *eo_obj, Evas_Object_Textblock2_Format *fmt)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   void *of;

   of = fmt->font.font;

   fmt->font.font = evas_font_load(obj->layer->evas->evas, fmt->font.fdesc,
         fmt->font.source, (int)(((double) fmt->font.size) * obj->cur->scale));
   if (of) evas_font_free(obj->layer->evas->evas, of);
}

/**
 * @internal
 * Returns true if the item is a tab
 * @def _IS_TAB(item)
 */
#define _IS_TAB(item) 0
/**
 * @internal
 * Returns true if the item is a line spearator, false otherwise
 * @def _IS_LINE_SEPARATOR(item)
 */
#define _IS_LINE_SEPARATOR(item) 0
/**
 * @internal
 * Returns true if the item is a paragraph separator, false otherwise
 * @def _IS_PARAGRAPH_SEPARATOR(item)
 */
#define _IS_PARAGRAPH_SEPARATOR_SIMPLE(item) 0
/**
 * @internal
 * Returns true if the item is a paragraph separator, false otherwise
 * takes legacy mode into account.
 * @def _IS_PARAGRAPH_SEPARATOR(item)
 */
#define _IS_PARAGRAPH_SEPARATOR(o, item) 0

static void
_layout_update_par(Ctxt *c)
{
   Evas_Object_Textblock2_Paragraph *last_par;
   last_par = (Evas_Object_Textblock2_Paragraph *)
      EINA_INLIST_GET(c->par)->prev;
   if (last_par)
     {
        c->par->y = last_par->y + last_par->h;
     }
   else
     {
        c->par->y = 0;
     }
}

/* -1 means no wrap */
static int
_layout_get_charwrap(Ctxt *c, Evas_Object_Textblock2_Format *fmt,
      const Evas_Object_Textblock2_Item *it, size_t line_start,
      const char *breaks)
{
   int wrap;
   size_t uwrap;
   size_t len = eina_ustrbuf_length_get(it->text_node->unicode);
   /* Currently not being used, because it doesn't contain relevant
    * information */
   (void) breaks;

     {
        if (it->type == EVAS_TEXTBLOCK2_ITEM_FORMAT)
           wrap = 0;
        else
           wrap = _layout_text_cutoff_get(c, fmt, _ITEM_TEXT(it));

        if (wrap < 0)
           return -1;
        uwrap = (size_t) wrap + it->text_pos;
     }


   if ((uwrap == line_start) && (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT))
     {
        uwrap = it->text_pos +
           (size_t) evas_common_text_props_cluster_next(
                 &_ITEM_TEXT(it)->text_props, wrap);
     }
   if ((uwrap <= line_start) || (uwrap > len))
      return -1;

   return uwrap;
}

/* -1 means no wrap */
/* Allow break means: if we can break after the current char */
#define ALLOW_BREAK(i) \
   (breaks[i] <= LINEBREAK_ALLOWBREAK)

static int
_layout_get_word_mixwrap_common(Ctxt *c, Evas_Object_Textblock2_Format *fmt,
      const Evas_Object_Textblock2_Item *it, Eina_Bool mixed_wrap,
      size_t line_start, const char *breaks)
{
   Eina_Bool wrap_after = EINA_FALSE;
   size_t wrap;
   size_t orig_wrap;
   const Eina_Unicode *str = eina_ustrbuf_string_get(
         it->text_node->unicode);
   int item_start = it->text_pos;
   size_t len = eina_ustrbuf_length_get(it->text_node->unicode);

     {
        int swrap = -1;
        if (it->type == EVAS_TEXTBLOCK2_ITEM_FORMAT)
           swrap = 0;
        else
           swrap = _layout_text_cutoff_get(c, fmt, _ITEM_TEXT(it));
        /* Avoiding too small textblock2s to even contain one char.
         * FIXME: This can cause breaking inside ligatures. */

        if (swrap < 0)
           return -1;

        orig_wrap = wrap = swrap + item_start;
     }

   if (wrap > line_start)
     {
        /* The wrapping point found is the first char of the next string
           the rest works on the last char of the previous string.
           If it's a whitespace, then it's ok, and no need to go back
           because we'll remove it anyway. */
        if (!_is_white(str[wrap]) || (wrap + 1 == len))
           MOVE_PREV_UNTIL(line_start, wrap);
        /* If there's a breakable point inside the text, scan backwards until
         * we find it */
        while (wrap > line_start)
          {
             if (ALLOW_BREAK(wrap))
                break;
             wrap--;
          }

        if ((wrap > line_start) ||
              ((wrap == line_start) && (ALLOW_BREAK(wrap)) && (wrap < len)))
          {
             /* We found a suitable wrapping point, break here. */
             MOVE_NEXT_UNTIL(len, wrap);
             return wrap;
          }
        else
          {
             if (mixed_wrap)
               {
                  return ((orig_wrap >= line_start) && (orig_wrap < len)) ?
                     ((int) orig_wrap) : -1;
               }
             else
               {
                  /* Scan forward to find the next wrapping point */
                  wrap = orig_wrap;
                  wrap_after = EINA_TRUE;
               }
          }
     }

   /* If we need to find the position after the cutting point */
   if ((wrap == line_start) || (wrap_after))
     {
        if (mixed_wrap)
          {
             return _layout_get_charwrap(c, fmt, it,
                   line_start, breaks);
          }
        else
          {
             while (wrap < len)
               {
                  if (ALLOW_BREAK(wrap))
                     break;
                  wrap++;
               }


             if ((wrap < len) && (wrap >= line_start))
               {
                  MOVE_NEXT_UNTIL(len, wrap);
                  return wrap;
               }
             else
               {
                  return -1;
               }
          }
     }

   return -1;
}

/* -1 means no wrap */
static int
_layout_get_wordwrap(Ctxt *c, Evas_Object_Textblock2_Format *fmt,
      const Evas_Object_Textblock2_Item *it, size_t line_start,
      const char *breaks)
{
   return _layout_get_word_mixwrap_common(c, fmt, it, EINA_FALSE, line_start,
         breaks);
}

/* -1 means no wrap */
static int
_layout_get_mixedwrap(Ctxt *c, Evas_Object_Textblock2_Format *fmt,
      const Evas_Object_Textblock2_Item *it, size_t line_start,
      const char *breaks)
{
   return _layout_get_word_mixwrap_common(c, fmt, it, EINA_TRUE, line_start,
         breaks);
}

static int
_it_break_position_get(Evas_Object_Textblock2_Item *it, const char *breaks)
{
   if (it->type != EVAS_TEXTBLOCK2_ITEM_TEXT)
      return -1;

   Evas_Object_Textblock2_Text_Item *ti = _ITEM_TEXT(it);
   breaks += it->text_pos;
   size_t i;
   for (i = 0 ; i < ti->text_props.text_len ; i++, breaks++)
     {
        if (*breaks == LINEBREAK_MUSTBREAK)
          {
             return i + it->text_pos;
          }
     }

   return -1;
}

static int
_layout_par_wrap_find(Ctxt *c, Evas_Object_Textblock2_Format *fmt, Evas_Object_Textblock2_Item *it, const char *line_breaks)
{
   int wrap = -1;

   if ((c->w >= 0) &&
         ((fmt->wrap_word) ||
          (fmt->wrap_char) ||
          (fmt->wrap_mixed)))
     {
        int line_start = it->text_pos;
        if (it->format->wrap_word)
           wrap = _layout_get_wordwrap(c, it->format, it,
                 line_start, line_breaks);
        else if (it->format->wrap_char)
           wrap = _layout_get_charwrap(c, it->format, it,
                 line_start, line_breaks);
        else if (it->format->wrap_mixed)
           wrap = _layout_get_mixedwrap(c, it->format, it,
                 line_start, line_breaks);
        else
           wrap = -1;
     }

   return wrap;
}

static void
_layout_par_line_item_add(Ctxt *c, Evas_Object_Textblock2_Item *it)
{
   c->ln->items = _ITEM(eina_inlist_append(EINA_INLIST_GET(c->ln->items),
            EINA_INLIST_GET(it)));
   it->ln = c->ln;
   c->x += it->adv;
}

/* 0 means go ahead, 1 means break without an error, 2 means
 * break with an error, should probably clean this a bit (enum/macro)
 * FIXME ^ */
static int
_layout_par(Ctxt *c)
{
   Evas_Object_Textblock2_Item *it;
   Eina_List *i;
   int ret = 0;
   char *line_breaks = NULL;

   if (!c->par->logical_items)
     return 2;

   /* We want to show it. */
   c->par->visible = 1;

   /* Check if we need to skip this paragraph because it's already layouted
    * correctly, and mark handled nodes as dirty. */
   c->par->line_no = c->line_no;

   if (c->par->text_node)
     {
        /* Skip this paragraph if width is the same, there is no ellipsis
         * and we aren't just calculating. */
        if (!c->par->text_node->is_new && !c->par->text_node->dirty &&
              !c->width_changed && c->par->lines &&
              !c->o->have_ellipsis)
          {
             Evas_Object_Textblock2_Line *ln;
             /* Update c->line_no */
             ln = (Evas_Object_Textblock2_Line *)
                EINA_INLIST_GET(c->par->lines)->last;
             if (ln)
                c->line_no = c->par->line_no + ln->line_no + 1;

             /* After this par we are no longer at the beginning, as there
              * must be some text in the par. */
             if (c->position == TEXTBLOCK2_POSITION_START)
                c->position = TEXTBLOCK2_POSITION_ELSE;

             return 0;
          }
        c->par->text_node->dirty = EINA_FALSE;
        c->par->text_node->is_new = EINA_FALSE;

        /* Merge back and clear the paragraph */
          {
             Eina_List *itr, *itr_next;
             Evas_Object_Textblock2_Item *ititr, *prev_it = NULL;
             _paragraph_clear(c->obj, c->par);
             EINA_LIST_FOREACH_SAFE(c->par->logical_items, itr, itr_next, ititr)
               {
                  if (ititr->merge && prev_it &&
                        (prev_it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) &&
                        (ititr->type == EVAS_TEXTBLOCK2_ITEM_TEXT))
                    {
                       _layout_item_merge_and_free(c, _ITEM_TEXT(prev_it),
                             _ITEM_TEXT(ititr));
                       c->par->logical_items =
                          eina_list_remove_list(c->par->logical_items, itr);
                    }
                  else
                    {
                       ititr->visually_deleted = EINA_FALSE;
                       prev_it = ititr;
                    }
               }
          }
     }

   c->y = c->par->y;


#ifdef BIDI_SUPPORT
   if (c->par->is_bidi)
     {
        _layout_update_bidi_props(c->o, c->par);
     }
#endif

   it = _ITEM(eina_list_data_get(c->par->logical_items));
   _layout_line_new(c, it->format);
   /* We walk on our own because we want to be able to add items from
    * inside the list and then walk them on the next iteration. */

     {
        const char *lang = "";
        size_t len = eina_ustrbuf_length_get(c->par->text_node->unicode);
        line_breaks = malloc(len);
        set_linebreaks_utf32((const utf32_t *)
              eina_ustrbuf_string_get(c->par->text_node->unicode),
              len, lang, line_breaks);
     }

   /* XXX: We assume wrap type doesn't change between items. */

   /* This loop walks on lines, we do per item inside. */
   for (i = c->par->logical_items ; i ; )
     {
        it = _ITEM(eina_list_data_get(i));
        /* Skip visually deleted items */
        if (it->visually_deleted)
          {
             i = eina_list_next(i);
             continue;
          }

        if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
          {
             _layout_item_ascent_descent_adjust(c->obj, &c->ascent,
                   &c->descent, it, it->format);
          }

        while (i)
          {
             int break_position = 0;
             it = _ITEM(eina_list_data_get(i));

             if ((break_position = _it_break_position_get(it, line_breaks)) > 0)
               {
                  if (eina_ustrbuf_string_get(
                           c->par->text_node->unicode)[break_position] == _PARAGRAPH_SEPARATOR)
                    {
                       break_position = -1;
                    }
                  else
                    {
                       break_position++;
                    }
               }

               {
                  int wrap = _layout_par_wrap_find(c, it->format, it, line_breaks);
                  if (((0 < wrap) && (wrap < break_position)) || (break_position < 0))
                    {
                       break_position = wrap;
                    }
               }

             if (break_position > 0)
               {
                  /* Add all the items that don't need breaking. */
                  for ( ; i ; i = eina_list_next(i), it = _ITEM(eina_list_data_get(i)))
                    {
                       if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
                         {
                            Evas_Object_Textblock2_Text_Item *ti = _ITEM_TEXT(it);
                            if ((it->text_pos < (unsigned int) break_position) &&
                               ((unsigned int) break_position <= it->text_pos + ti->text_props.text_len))
                              {
                                 break;
                              }

                            _layout_par_line_item_add(c, it);
                         }
                       else
                         {
                            /* FIXME: Do something. */
                            break;
                         }
                    }

                  _layout_item_text_split_strip_white(c, _ITEM_TEXT(it), i, break_position - it->text_pos);
               }

             _layout_par_line_item_add(c, it);

             i = eina_list_next(i);

             if (break_position > 0)
               {
                  _layout_line_advance(c, it->format);
                  break;
               }
          }
     }

   if (c->ln->items)
     {
        if (c->par && !EINA_INLIST_GET(c->par)->next)
          {
             c->position = (c->position == TEXTBLOCK2_POSITION_START) ?
                TEXTBLOCK2_POSITION_SINGLE : TEXTBLOCK2_POSITION_END;
          }

        /* Here 'it' is the last format used */
        _layout_line_finalize(c, it->format);
     }

   if (line_breaks)
      free(line_breaks);

#ifdef BIDI_SUPPORT
   if (c->par->bidi_props)
     {
        evas_bidi_paragraph_props_unref(c->par->bidi_props);
        c->par->bidi_props = NULL;
     }
#endif

   return ret;
}

static Layout_Text_Append_Queue *
_layout_text_append_queue_item_append(Layout_Text_Append_Queue *queue,
      Evas_Object_Textblock2_Format *format, size_t start, int off)
{
   /* Don't add empty items. */
   if (off == 0)
      return (Layout_Text_Append_Queue *) queue;

   Layout_Text_Append_Queue *item = calloc(1, sizeof(*item));
   item->format = format;
   item->start = start;
   item->off = off;
   item->format->ref++;

   return (Layout_Text_Append_Queue *) eina_inlist_append(EINA_INLIST_GET(queue), EINA_INLIST_GET(item));
}

static void
_layout_text_append_item_free(Ctxt *c, Layout_Text_Append_Queue *item)
{
   if (item->format)
      _format_unref_free(c->obj, item->format);
   free(item);
}

static void
_layout_text_append_commit(Ctxt *c, Layout_Text_Append_Queue **_queue, Evas_Object_Textblock2_Node_Text *n, Eina_List *rel)
{
   Layout_Text_Append_Queue *item, *queue = *_queue;

   if (!queue)
      return;

     {
        item = (Layout_Text_Append_Queue *) EINA_INLIST_GET(queue)->last;
        int off = item->start - queue->start + item->off;
        _layout_text_append(c, queue, n, queue->start, off, rel);
     }

   while (queue)
     {
        item = queue;
        queue = (Layout_Text_Append_Queue *) EINA_INLIST_GET(queue)->next;
        _layout_text_append_item_free(c, item);
     }

   *_queue = NULL;
}

/** FIXME: Document */
static void
_layout_pre(Ctxt *c, int *style_pad_l, int *style_pad_r, int *style_pad_t,
      int *style_pad_b)
{
   Evas_Object *eo_obj = c->obj;
   Evas_Textblock2_Data *o = c->o;

   if (o->content_changed)
     {
        Evas_Object_Textblock2_Node_Text *n;
        c->o->have_ellipsis = 0;
        c->par = c->paragraphs = o->paragraphs;
        /* Go through all the text nodes to create the logical layout */
        EINA_INLIST_FOREACH(c->o->text_nodes, n)
          {
             size_t start;
             int off;

             /* If it's not a new paragraph, either update it or skip it.
              * Remove all the paragraphs that were deleted */
             if (!n->is_new)
               {
                  /* Remove all the deleted paragraphs at this point */
                  while (c->par->text_node != n)
                    {
                       Evas_Object_Textblock2_Paragraph *tmp_par =
                          (Evas_Object_Textblock2_Paragraph *)
                          EINA_INLIST_GET(c->par)->next;

                       c->paragraphs = (Evas_Object_Textblock2_Paragraph *)
                          eina_inlist_remove(EINA_INLIST_GET(c->paragraphs),
                                EINA_INLIST_GET(c->par));
                       _paragraph_free(eo_obj, c->par);

                       c->par = tmp_par;
                    }

                  /* If it's dirty, remove and recreate, if it's clean,
                   * skip to the next. */
                  if (n->dirty)
                    {
                       Evas_Object_Textblock2_Paragraph *prev_par = c->par;

                       _layout_paragraph_new(c, n, EINA_TRUE);

                       c->paragraphs = (Evas_Object_Textblock2_Paragraph *)
                          eina_inlist_remove(EINA_INLIST_GET(c->paragraphs),
                                EINA_INLIST_GET(prev_par));
                       _paragraph_free(eo_obj, prev_par);
                    }
                  else
                    {
                       c->par = (Evas_Object_Textblock2_Paragraph *)
                          EINA_INLIST_GET(c->par)->next;
                       continue;
                    }
               }
             else
               {
                  /* If it's a new paragraph, just add it. */
                  _layout_paragraph_new(c, n, EINA_FALSE);
               }

#ifdef BIDI_SUPPORT
             _layout_update_bidi_props(c->o, c->par);
#endif

             Layout_Text_Append_Queue *queue = NULL;
             start = off = 0;
             queue = _layout_text_append_queue_item_append(queue, c->fmt, start,
                   eina_ustrbuf_length_get(n->unicode) - start);
             _layout_text_append_commit(c, &queue, n, NULL);
#ifdef BIDI_SUPPORT
             /* Clear the bidi props because we don't need them anymore. */
             if (c->par->bidi_props)
               {
                  evas_bidi_paragraph_props_unref(c->par->bidi_props);
                  c->par->bidi_props = NULL;
               }
#endif
             c->par = (Evas_Object_Textblock2_Paragraph *)
                EINA_INLIST_GET(c->par)->next;
          }

        /* Delete the rest of the layout paragraphs */
        while (c->par)
          {
             Evas_Object_Textblock2_Paragraph *tmp_par =
                (Evas_Object_Textblock2_Paragraph *)
                EINA_INLIST_GET(c->par)->next;

             c->paragraphs = (Evas_Object_Textblock2_Paragraph *)
                eina_inlist_remove(EINA_INLIST_GET(c->paragraphs),
                      EINA_INLIST_GET(c->par));
             _paragraph_free(eo_obj, c->par);

             c->par = tmp_par;
          }
        o->paragraphs = c->paragraphs;
        c->par = NULL;
     }
   else
     {
        if (o->style_pad.l > *style_pad_l) *style_pad_l = o->style_pad.l;
        if (o->style_pad.r > *style_pad_r) *style_pad_r = o->style_pad.r;
        if (o->style_pad.t > *style_pad_t) *style_pad_t = o->style_pad.t;
        if (o->style_pad.b > *style_pad_b) *style_pad_b = o->style_pad.b;
     }
}

/**
 * @internal
 * Create the layout from the nodes.
 *
 * @param obj the evas object - NOT NULL.
 * @param calc_only true if should only calc sizes false if should also create the layout.. It assumes native size is being calculated, doesn't support formatted size atm.
 * @param w the object's w, -1 means no wrapping (i.e infinite size)
 * @param h the object's h, -1 means inifinte size.
 * @param w_ret the object's calculated w.
 * @param h_ret the object's calculated h.
 */
static void
_layout(const Evas_Object *eo_obj, int w, int h, int *w_ret, int *h_ret)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Textblock2_Data *o = eo_data_ref(eo_obj, MY_CLASS);
   Ctxt ctxt, *c;
   int style_pad_l = 0, style_pad_r = 0, style_pad_t = 0, style_pad_b = 0;

   LYDBG("ZZ: layout %p %4ix%4i | w=%4i | last_w=%4i --- '%s'\n", eo_obj, w, h, obj->cur->geometry.w, o->last_w, o->markup_text);
   /* setup context */
   c = &ctxt;
   c->obj = (Evas_Object *)eo_obj;
   c->o = o;
   c->paragraphs = c->par = NULL;
   c->format_stack = NULL;
   c->fmt = NULL;
   c->x = c->y = 0;
   c->w = w;
   c->h = h;
   c->wmax = c->hmax = 0;
   c->ascent = c->descent = 0;
   c->maxascent = c->maxdescent = 0;
   c->marginl = c->marginr = 0;
   c->have_underline = 0;
   c->have_underline2 = 0;
   c->underline_extend = 0;
   c->line_no = 0;
   c->align = 0.0;
   c->align_auto = EINA_TRUE;
   c->ln = NULL;
   c->width_changed = (obj->cur->geometry.w != o->last_w);

   /* Start of logical layout creation */
   /* setup default base style */
     {
        Eina_Bool finalize = EINA_FALSE;
        if ((c->o->style) && (c->o->style->default_tag))
          {
             c->fmt = _layout_format_push(c, NULL);
             _format_fill(c->obj, c->fmt, c->o->style->default_tag);
             finalize = EINA_TRUE;
          }

        if ((c->o->style_user) && (c->o->style_user->default_tag))
          {
             if (!c->fmt)
               {
                  c->fmt = _layout_format_push(c, NULL);
               }
             _format_fill(c->obj, c->fmt, c->o->style_user->default_tag);
             finalize = EINA_TRUE;
          }

        if (finalize)
           _format_finalize(c->obj, c->fmt);
     }
   if (!c->fmt)
     {
        if (w_ret) *w_ret = 0;
        if (h_ret) *h_ret = 0;
        return;
     }

   _layout_pre(c, &style_pad_l, &style_pad_r, &style_pad_t, &style_pad_b);
   c->paragraphs = o->paragraphs;

   /* If there are no paragraphs, create the minimum needed,
    * if the last paragraph has no lines/text, create that as well */
   if (!c->paragraphs)
     {
        _layout_paragraph_new(c, NULL, EINA_TRUE);
        o->paragraphs = c->paragraphs;
     }
   c->par = (Evas_Object_Textblock2_Paragraph *)
      EINA_INLIST_GET(c->paragraphs)->last;
   if (!c->par->logical_items)
     {
        Evas_Object_Textblock2_Text_Item *ti;
        ti = _layout_text_item_new(c, c->fmt);
        ti->parent.text_node = c->par->text_node;
        ti->parent.text_pos = 0;
        _layout_text_add_logical_item(c, ti, NULL);
     }

   /* End of logical layout creation */

   /* Start of visual layout creation */
   {
      Evas_Object_Textblock2_Paragraph *last_vis_par = NULL;
      int par_index_step = o->num_paragraphs / TEXTBLOCK2_PAR_INDEX_SIZE;
      int par_count = 1; /* Force it to take the first one */
      int par_index_pos = 0;

      c->position = TEXTBLOCK2_POSITION_START;

      if (par_index_step == 0) par_index_step = 1;

      /* Clear all of the index */
      memset(o->par_index, 0, sizeof(o->par_index));

      EINA_INLIST_FOREACH(c->paragraphs, c->par)
        {
           _layout_update_par(c);

           /* Break if we should stop here. */
           if (_layout_par(c))
             {
                last_vis_par = c->par;
                break;
             }

           if ((par_index_pos < TEXTBLOCK2_PAR_INDEX_SIZE) && (--par_count == 0))
             {
                par_count = par_index_step;

                o->par_index[par_index_pos++] = c->par;
             }
        }

      /* Mark all the rest of the paragraphs as invisible */
      if (c->par)
        {
           c->par = (Evas_Object_Textblock2_Paragraph *)
              EINA_INLIST_GET(c->par)->next;
           while (c->par)
             {
                c->par->visible = 0;
                c->par = (Evas_Object_Textblock2_Paragraph *)
                   EINA_INLIST_GET(c->par)->next;
             }
        }

      /* Get the last visible paragraph in the layout */
      if (!last_vis_par && c->paragraphs)
         last_vis_par = (Evas_Object_Textblock2_Paragraph *)
            EINA_INLIST_GET(c->paragraphs)->last;

      if (last_vis_par)
        {
           c->hmax = last_vis_par->y + last_vis_par->h +
              _layout_last_line_max_descent_adjust_calc(c, last_vis_par);
        }
   }

   /* Clean the rest of the format stack */
   while (c->format_stack)
     {
        c->fmt = c->format_stack->data;
        c->format_stack = eina_list_remove_list(c->format_stack, c->format_stack);
        _format_unref_free(c->obj, c->fmt);
     }

   if (w_ret) *w_ret = c->wmax;
   if (h_ret) *h_ret = c->hmax;

   /* Vertically align the textblock2 */
   if ((o->valign > 0.0) && (c->h > c->hmax))
     {
        Evas_Coord adjustment = (c->h - c->hmax) * o->valign;
        Evas_Object_Textblock2_Paragraph *par;
        EINA_INLIST_FOREACH(c->paragraphs, par)
          {
             par->y += adjustment;
          }
     }

   if ((o->style_pad.l != style_pad_l) || (o->style_pad.r != style_pad_r) ||
       (o->style_pad.t != style_pad_t) || (o->style_pad.b != style_pad_b))
     {
        o->style_pad.l = style_pad_l;
        o->style_pad.r = style_pad_r;
        o->style_pad.t = style_pad_t;
        o->style_pad.b = style_pad_b;
        _paragraphs_clear(eo_obj, c->paragraphs);
        LYDBG("ZZ: ... layout #2\n");
        _layout(eo_obj, w, h, w_ret, h_ret);
     }
}

/*
 * @internal
 * Relayout the object according to current object size.
 *
 * @param obj the evas object - NOT NULL.
 */
static void
_relayout(const Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   _layout(eo_obj, obj->cur->geometry.w, obj->cur->geometry.h,
         &o->formatted.w, &o->formatted.h);
   o->formatted.valid = 1;
   o->formatted.oneline_h = 0;
   o->last_w = obj->cur->geometry.w;
   LYDBG("ZZ: --------- layout %p @ %ix%i = %ix%i\n", eo_obj, obj->cur->geometry.w, obj->cur->geometry.h, o->formatted.w, o->formatted.h);
   o->last_h = obj->cur->geometry.h;
   if ((o->paragraphs) && (!EINA_INLIST_GET(o->paragraphs)->next) &&
       (o->paragraphs->lines) && (!EINA_INLIST_GET(o->paragraphs->lines)->next))
     {
        if (obj->cur->geometry.h < o->formatted.h)
          {
             LYDBG("ZZ: 1 line only... lasth == formatted h (%i)\n", o->formatted.h);
             o->formatted.oneline_h = o->formatted.h;
          }
     }
   o->changed = 0;
   o->content_changed = 0;
   o->format_changed = EINA_FALSE;
   o->redraw = 1;
}

/*
 * @internal
 * Check if the object needs a relayout, and if so, execute it.
 */
static inline void
_relayout_if_needed(Evas_Object *eo_obj, const Evas_Textblock2_Data *o)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);

   evas_object_textblock2_coords_recalc(eo_obj, obj, obj->private_data);
   if (!o->formatted.valid)
     {
        LYDBG("ZZ: relayout\n");
        _relayout(eo_obj);
     }
}

/**
 * @internal
 * Find the layout item and line that match the text node and position passed.
 *
 * @param obj the evas object - NOT NULL.
 * @param n the text node - Not null.
 * @param pos the position to look for - valid.
 * @param[out] lnr the line found - not null.
 * @param[out] tir the item found - not null.
 * @see _find_layout_format_item_line_match()
 */
static void
_find_layout_item_line_match(Evas_Object *eo_obj, Evas_Object_Textblock2_Node_Text *n, size_t pos, Evas_Object_Textblock2_Line **lnr, Evas_Object_Textblock2_Item **itr)
{
   Evas_Object_Textblock2_Paragraph *found_par;
   Evas_Object_Textblock2_Line *ln;
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

   _relayout_if_needed(eo_obj, o);

   found_par = n->par;
   if (found_par)
     {
        EINA_INLIST_FOREACH(found_par->lines, ln)
          {
             Evas_Object_Textblock2_Item *it;

             EINA_INLIST_FOREACH(ln->items, it)
               {
                  size_t p = it->text_pos;

                  if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
                    {
                       Evas_Object_Textblock2_Text_Item *ti =
                          _ITEM_TEXT(it);

                       p += ti->text_props.text_len;
                    }
                  else
                    {
                       p++;
                    }

                  if (((pos >= it->text_pos) && (pos < p)))
                    {
                       *lnr = ln;
                       *itr = it;
                       return;
                    }
                  else if (p == pos)
                    {
                       *lnr = ln;
                       *itr = it;
                    }
               }
          }
     }
}

/**
 * @internal
 * Return the line number 'line'.
 *
 * @param obj the evas object - NOT NULL.
 * @param line the line to find
 * @return the line of line number or NULL if no line found.
 */
static Evas_Object_Textblock2_Line *
_find_layout_line_num(const Evas_Object *eo_obj, int line)
{
   Evas_Object_Textblock2_Paragraph *par;
   Evas_Object_Textblock2_Line *ln;
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

   par = _layout_find_paragraph_by_line_no(o, line);
   if (par)
     {
        EINA_INLIST_FOREACH(par->lines, ln)
          {
             if (par->line_no + ln->line_no == line) return ln;
          }
     }
   return NULL;
}

EAPI Evas_Object *
evas_object_textblock2_add(Evas *e)
{
   Evas_Object *eo_obj = eo_add(EVAS_TEXTBLOCK2_CLASS, e);
   return eo_obj;
}

EOLIAN static void
_evas_textblock2_eo_base_constructor(Eo *eo_obj, Evas_Textblock2_Data *class_data EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Textblock2_Data *o;
   Eo *eo_parent = NULL;

   eo_do_super(eo_obj, MY_CLASS, eo_constructor());

   /* set up methods (compulsory) */
   obj->func = &object_func;
   obj->private_data = eo_data_ref(eo_obj, MY_CLASS);
   obj->type = o_type;

   o = obj->private_data;
   o->cursor = calloc(1, sizeof(Evas_Textblock2_Cursor));
   _format_command_init();
   evas_object_textblock2_init(eo_obj);

   eo_do(eo_obj, eo_parent = eo_parent_get());
   evas_object_inject(eo_obj, obj, evas_object_evas_get(eo_parent));
}

EAPI Evas_Textblock2_Style *
evas_textblock2_style_new(void)
{
   Evas_Textblock2_Style *ts;

   ts = calloc(1, sizeof(Evas_Textblock2_Style));
   return ts;
}

EAPI void
evas_textblock2_style_free(Evas_Textblock2_Style *ts)
{
   if (!ts) return;
   if (ts->objects)
     {
        ts->delete_me = 1;
        return;
     }
   _style_clear(ts);
   free(ts);
}

EAPI void
evas_textblock2_style_set(Evas_Textblock2_Style *ts, const char *text)
{
   Eina_List *l;
   Evas_Object *eo_obj;

   if (!ts) return;
   /* If the style wasn't really changed, abort. */
   if ((!ts->style_text && !text) ||
       (ts->style_text && text && !strcmp(text, ts->style_text)))
      return;

   EINA_LIST_FOREACH(ts->objects, l, eo_obj)
     {
        Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
        _evas_textblock2_invalidate_all(o);
        _evas_textblock2_changed(o, eo_obj);
     }

   _style_replace(ts, text);

   if (ts->style_text)
     {
        // format MUST be KEY='VALUE'[KEY='VALUE']...
        const char *p;
        const char *key_start, *key_stop, *val_start;

        key_start = key_stop = val_start = NULL;
        p = ts->style_text;
        while (*p)
          {
             if (!key_start)
               {
		 if (!isspace((unsigned char)(*p)))
                    key_start = p;
               }
             else if (!key_stop)
               {
		 if ((*p == '=') || (isspace((unsigned char)(*p))))
                    key_stop = p;
               }
             else if (!val_start)
               {
                  if (((*p) == '\'') && (*(p + 1)))
                    {
                       val_start = ++p;
                    }
               }
             if ((key_start) && (key_stop) && (val_start))
               {
                  char *tags, *replaces = NULL;
                  Evas_Object_Style_Tag *tag;
                  const char *val_stop = NULL;
                  size_t tag_len;
                  size_t replace_len;

                    {
                       Eina_Strbuf *buf = eina_strbuf_new();
                       val_stop = val_start;
                       while(*p)
                         {
                            if (*p == '\'')
                              {
                                 /* Break if we found the tag end */
                                 if (p[-1] != '\\')
                                   {
                                      eina_strbuf_append_length(buf, val_stop,
                                            p - val_stop);
                                      break;
                                   }
                                 else
                                   {
                                      eina_strbuf_append_length(buf, val_stop,
                                            p - val_stop - 1);
                                      eina_strbuf_append_char(buf, '\'');
                                      val_stop = p + 1;
                                   }
                              }
                            p++;
                         }
                       replaces = eina_strbuf_string_steal(buf);
                       eina_strbuf_free(buf);
                    }
                  /* If we didn't find an end, just aboart. */
                  if (!*p)
                    {
                       if (replaces) free(replaces);
                       break;
                    }

                  tag_len = key_stop - key_start;
                  replace_len = val_stop - val_start;

                  tags = malloc(tag_len + 1);
                  if (tags)
                    {
                       memcpy(tags, key_start, tag_len);
                       tags[tag_len] = 0;
                    }

                  if ((tags) && (replaces))
                    {
                       if (!strcmp(tags, "DEFAULT"))
                         {
                            ts->default_tag = replaces;
                            free(tags);
                         }
                       else
                         {
                            tag = calloc(1, sizeof(Evas_Object_Style_Tag));
                            if (tag)
                              {
                                 tag->tag.tag = tags;
                                 tag->tag.replace = replaces;
                                 tag->tag.tag_len = tag_len;
                                 tag->tag.replace_len = replace_len;
                                 ts->tags = (Evas_Object_Style_Tag *)eina_inlist_append(EINA_INLIST_GET(ts->tags), EINA_INLIST_GET(tag));
                              }
                            else
                              {
                                 free(tags);
                                 free(replaces);
                              }
                         }
                    }
                  else
                    {
                       if (tags) free(tags);
                       if (replaces) free(replaces);
                    }
                  key_start = key_stop = val_start = NULL;
               }
             p++;
          }
     }
}

EAPI const char *
evas_textblock2_style_get(const Evas_Textblock2_Style *ts)
{
   if (!ts) return NULL;
   return ts->style_text;
}

/* textblock2 styles */

static void
_textblock2_style_generic_set(Evas_Object *eo_obj, Evas_Textblock2_Style *ts,
      Evas_Textblock2_Style **obj_ts)
{
   TB_HEAD();
   if (ts == *obj_ts) return;
   if ((ts) && (ts->delete_me)) return;
   if (*obj_ts)
     {
        Evas_Textblock2_Style *old_ts;
        if (o->markup_text)
          {
             free(o->markup_text);
             o->markup_text = NULL;
          }

        old_ts = *obj_ts;
        old_ts->objects = eina_list_remove(old_ts->objects, eo_obj);
        if ((old_ts->delete_me) && (!old_ts->objects))
          evas_textblock2_style_free(old_ts);
     }
   if (ts)
     {
        ts->objects = eina_list_append(ts->objects, eo_obj);
     }
   *obj_ts = ts;

   o->format_changed = EINA_TRUE;
   _evas_textblock2_invalidate_all(o);
   _evas_textblock2_changed(o, eo_obj);
}

EOLIAN static void
_evas_textblock2_style_set(Eo *eo_obj, Evas_Textblock2_Data *o, const Evas_Textblock2_Style *ts)
{
   _textblock2_style_generic_set(eo_obj, (Evas_Textblock2_Style *) ts, &(o->style));
}

EOLIAN static const Evas_Textblock2_Style*
_evas_textblock2_style_get(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o)
{
   return o->style;
}

EOLIAN static void
_evas_textblock2_style_user_push(Eo *eo_obj, Evas_Textblock2_Data *o, Evas_Textblock2_Style *ts)
{
   _textblock2_style_generic_set(eo_obj, ts, &(o->style_user));
}

EOLIAN static const Evas_Textblock2_Style*
_evas_textblock2_style_user_peek(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o)
{
   return o->style_user;
}

EOLIAN static void
_evas_textblock2_style_user_pop(Eo *eo_obj, Evas_Textblock2_Data *o)
{
   _textblock2_style_generic_set(eo_obj, NULL,  &(o->style_user));
}

EOLIAN static void
_evas_textblock2_valign_set(Eo *eo_obj, Evas_Textblock2_Data *o, double align)
{
   if (align < 0.0) align = 0.0;
   else if (align > 1.0) align = 1.0;
   if (o->valign == align) return;
   o->valign = align;
   _evas_textblock2_changed(o, eo_obj);
}

EOLIAN static double
_evas_textblock2_valign_get(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o)
{
   return o->valign;
}

EOLIAN static void
_evas_textblock2_bidi_delimiters_set(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o, const char *delim)
{
   eina_stringshare_replace(&o->bidi_delimiters, delim);
}

EOLIAN static const char*
_evas_textblock2_bidi_delimiters_get(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o)
{
   return o->bidi_delimiters;
}

/* cursors */

/**
 * @internal
 * Merge the current node with the next, no need to remove PS, already
 * not there.
 *
 * @param o the text block object.
 * @param to merge into to.
 */
static void
_evas_textblock2_nodes_merge(Evas_Textblock2_Data *o, Evas_Object_Textblock2_Node_Text *to)
{
   Evas_Object_Textblock2_Node_Text *from;
   const Eina_Unicode *text;
   int len;

   if (!to) return;
   from = _NODE_TEXT(EINA_INLIST_GET(to)->next);

   text = eina_ustrbuf_string_get(from->unicode);
   len = eina_ustrbuf_length_get(from->unicode);
   eina_ustrbuf_append_length(to->unicode, text, len);

   /* When it comes to how we handle it, merging is like removing both nodes
    * and creating a new one, so we need to do the needed cleanups. */
   if (to->par)
      to->par->text_node = NULL;
   to->par = NULL;

   to->is_new = EINA_TRUE;

   _evas_textblock2_cursors_set_node(o, from, to);
   _evas_textblock2_node_text_remove(o, from);
}

/**
 * @internal
 * Merge the current node with the next, no need to remove PS, already
 * not there.
 *
 * @param cur the cursor that points to the current node
 */
static void
_evas_textblock2_cursor_nodes_merge(Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Node_Text *nnode;
   int len;
   if (!cur) return;

   len = eina_ustrbuf_length_get(cur->node->unicode);

   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);
   nnode = _NODE_TEXT(EINA_INLIST_GET(cur->node)->next);
   _evas_textblock2_nodes_merge(o, cur->node);
   _evas_textblock2_cursors_update_offset(cur, nnode, 0, len);
   _evas_textblock2_cursors_set_node(o, nnode, cur->node);
   if (nnode == o->cursor->node)
     {
        o->cursor->node = cur->node;
        o->cursor->pos += len;
     }
}

/**
 * @internal
 * Find the layout item and line that match the cursor.
 *
 * @param cur the cursor we are currently at. - NOT NULL.
 * @param[out] lnr the line found - not null.
 * @param[out] itr the item found - not null.
 * @return @c EINA_TRUE if we matched the previous format, @c EINA_FALSE
 * otherwise.
 */
static Eina_Bool
_find_layout_item_match(const Evas_Textblock2_Cursor *cur, Evas_Object_Textblock2_Line **lnr, Evas_Object_Textblock2_Item **itr)
{
   Evas_Textblock2_Cursor cur2;
   Eina_Bool previous_format = EINA_FALSE;

   cur2.obj = cur->obj;
   evas_textblock2_cursor_copy(cur, &cur2);
   if (cur2.pos > 0)
     {
        cur2.pos--;
     }

     {
        _find_layout_item_line_match(cur->obj, cur->node, cur->pos, lnr, itr);
     }
   return previous_format;
}

EOLIAN static Evas_Textblock2_Cursor*
_evas_textblock2_cursor_get(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o)
{
   return o->cursor;
}

EOLIAN static Evas_Textblock2_Cursor*
_evas_textblock2_cursor_new(Eo *eo_obj, Evas_Textblock2_Data *o)
{
   Evas_Textblock2_Cursor *cur;
     {
        cur = calloc(1, sizeof(Evas_Textblock2_Cursor));
        (cur)->obj = (Evas_Object *) eo_obj;
        (cur)->node = o->text_nodes;
        (cur)->pos = 0;
        o->cursors = eina_list_append(o->cursors, cur);
     }

   return cur;
}

EAPI void
evas_textblock2_cursor_free(Evas_Textblock2_Cursor *cur)
{
   if (!cur) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);
   if (cur == o->cursor) return;
   o->cursors = eina_list_remove(o->cursors, cur);
   free(cur);
}

EOLIAN static const Eina_List *
_evas_textblock2_node_format_list_get(Eo *eo_obj EINA_UNUSED, Evas_Textblock2_Data *o, const char *anchor)
{
   if (!strcmp(anchor, "a"))
      return o->anchors_a;
   else if (!strcmp(anchor, "item"))
      return o->anchors_item;
   return NULL;
}

EAPI void
evas_textblock2_cursor_paragraph_first(Evas_Textblock2_Cursor *cur)
{
   if (!cur) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);
   cur->node = o->text_nodes;
   cur->pos = 0;

}

EAPI void
evas_textblock2_cursor_paragraph_last(Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Node_Text *node;

   if (!cur) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);
   node = o->text_nodes;
   if (node)
     {
        node = _NODE_TEXT(EINA_INLIST_GET(node)->last);
        cur->node = node;
        cur->pos = 0;

        evas_textblock2_cursor_paragraph_char_last(cur);
     }
   else
     {
        cur->node = NULL;
        cur->pos = 0;

     }
}

EAPI Eina_Bool
evas_textblock2_cursor_paragraph_next(Evas_Textblock2_Cursor *cur)
{
   if (!cur) return EINA_FALSE;
   TB_NULL_CHECK(cur->node, EINA_FALSE);
   /* If there is a current text node, return the next text node (if exists)
    * otherwise, just return False. */
   if (cur->node)
     {
        Evas_Object_Textblock2_Node_Text *nnode;
        nnode = _NODE_TEXT(EINA_INLIST_GET(cur->node)->next);
        if (nnode)
          {
             cur->node = nnode;
             cur->pos = 0;

             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

/* BREAK_AFTER: true if we can break after the current char.
 * Both macros assume str[i] is not the terminating nul */
#define BREAK_AFTER(i) \
   (breaks[i] == WORDBREAK_BREAK)

EAPI Eina_Bool
evas_textblock2_cursor_word_start(Evas_Textblock2_Cursor *cur)
{
   const Eina_Unicode *text;
   size_t i;
   char *breaks;

   if (!cur) return EINA_FALSE;
   TB_NULL_CHECK(cur->node, EINA_FALSE);

   size_t len = eina_ustrbuf_length_get(cur->node->unicode);

   text = eina_ustrbuf_string_get(cur->node->unicode);

     {
        const char *lang = ""; /* FIXME: get lang */
        breaks = malloc(len);
        set_wordbreaks_utf32((const utf32_t *) text, len, lang, breaks);
     }

   if ((cur->pos > 0) && (cur->pos == len))
      cur->pos--;

   for (i = cur->pos ; _is_white(text[i]) && BREAK_AFTER(i) ; i--)
     {
        if (i == 0)
          {
             Evas_Object_Textblock2_Node_Text *pnode;
             pnode = _NODE_TEXT(EINA_INLIST_GET(cur->node)->prev);
             if (pnode)
               {
                  cur->node = pnode;
                  len = eina_ustrbuf_length_get(cur->node->unicode);
                  cur->pos = len - 1;
                  free(breaks);
                  return evas_textblock2_cursor_word_start(cur);
               }
             else
               {
                  break;
               }
          }
     }

   for ( ; i > 0 ; i--)
     {
        if (BREAK_AFTER(i - 1))
          {
             break;
          }
     }

   cur->pos = i;

   free(breaks);
   return EINA_TRUE;
}

EAPI Eina_Bool
evas_textblock2_cursor_word_end(Evas_Textblock2_Cursor *cur)
{
   const Eina_Unicode *text;
   size_t i;
   char *breaks;

   if (!cur) return EINA_FALSE;
   TB_NULL_CHECK(cur->node, EINA_FALSE);

   size_t len = eina_ustrbuf_length_get(cur->node->unicode);

   if (cur->pos == len)
      return EINA_TRUE;

   text = eina_ustrbuf_string_get(cur->node->unicode);

     {
        const char *lang = ""; /* FIXME: get lang */
        breaks = malloc(len);
        set_wordbreaks_utf32((const utf32_t *) text, len, lang, breaks);
     }

   for (i = cur->pos; text[i] && _is_white(text[i]) && (BREAK_AFTER(i)) ; i++);
   if (i == len)
     {
        Evas_Object_Textblock2_Node_Text *nnode;
        nnode = _NODE_TEXT(EINA_INLIST_GET(cur->node)->next);
        if (nnode)
          {
             cur->node = nnode;
             cur->pos = 0;
             free(breaks);
             return evas_textblock2_cursor_word_end(cur);
          }
     }

   for ( ; text[i] ; i++)
     {
        if (BREAK_AFTER(i))
          {
             /* This is the one to break after. */
             break;
          }
     }

   cur->pos = i;

   free(breaks);
   return EINA_TRUE;
}

EAPI Eina_Bool
evas_textblock2_cursor_char_next(Evas_Textblock2_Cursor *cur)
{
   int ind;
   const Eina_Unicode *text;

   if (!cur) return EINA_FALSE;
   TB_NULL_CHECK(cur->node, EINA_FALSE);

   ind = cur->pos;
   text = eina_ustrbuf_string_get(cur->node->unicode);
   if (text[ind]) ind++;
   /* Only allow pointing a null if it's the last paragraph.
    * because we don't have a PS there. */
   if (text[ind])
     {
        cur->pos = ind;
        return EINA_TRUE;
     }
   else
     {
        if (!evas_textblock2_cursor_paragraph_next(cur))
          {
             /* If we already were at the end, that means we don't have
              * where to go next we should return FALSE */
             if (cur->pos == (size_t) ind)
                return EINA_FALSE;

             cur->pos = ind;
             return EINA_TRUE;
          }
        else
          {
             return EINA_TRUE;
          }
     }
}

EAPI void
evas_textblock2_cursor_paragraph_char_last(Evas_Textblock2_Cursor *cur)
{
   int ind;

   if (!cur) return;
   TB_NULL_CHECK(cur->node);
   ind = eina_ustrbuf_length_get(cur->node->unicode);
   /* If it's not the last paragraph, go back one, because we want to point
    * to the PS, not the NULL */
   if (EINA_INLIST_GET(cur->node)->next)
      ind--;

   if (ind >= 0)
      cur->pos = ind;
   else
      cur->pos = 0;

}

EAPI void
evas_textblock2_cursor_line_char_first(Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Line *ln = NULL;
   Evas_Object_Textblock2_Item *it = NULL;

   if (!cur) return;
   TB_NULL_CHECK(cur->node);
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   _find_layout_item_match(cur, &ln, &it);

   if (!ln) return;
   if (ln->items)
     {
        Evas_Object_Textblock2_Item *i;
        it = ln->items;
        EINA_INLIST_FOREACH(ln->items, i)
          {
             if (it->text_pos > i->text_pos)
               {
                  it = i;
               }
          }
     }
   if (it)
     {
        cur->pos = it->text_pos;
        cur->node = it->text_node;
     }
}

EAPI void
evas_textblock2_cursor_line_char_last(Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Line *ln = NULL;
   Evas_Object_Textblock2_Item *it = NULL;

   if (!cur) return;
   TB_NULL_CHECK(cur->node);
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   _find_layout_item_match(cur, &ln, &it);

   if (!ln) return;
   if (ln->items)
     {
        Evas_Object_Textblock2_Item *i;
        it = ln->items;
        EINA_INLIST_FOREACH(ln->items, i)
          {
             if (it->text_pos < i->text_pos)
               {
                  it = i;
               }
          }
     }
   if (it)
     {
        size_t ind;

        cur->node = it->text_node;
        cur->pos = it->text_pos;
        if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
          {
             ind = _ITEM_TEXT(it)->text_props.text_len - 1;
             if (!IS_AT_END(_ITEM_TEXT(it), ind)) ind++;
             cur->pos += ind;
          }
        else if (!EINA_INLIST_GET(ln)->next && !EINA_INLIST_GET(ln->par)->next)
          {
             cur->pos++;
          }
     }
}

/**
 * Removes a text node and the corresponding format nodes.
 *
 * @param o the textblock2 objec.t
 * @param n the node to remove.
 */
static void
_evas_textblock2_node_text_remove(Evas_Textblock2_Data *o, Evas_Object_Textblock2_Node_Text *n)
{
   o->text_nodes = _NODE_TEXT(eina_inlist_remove(
            EINA_INLIST_GET(o->text_nodes), EINA_INLIST_GET(n)));
   _evas_textblock2_node_text_free(n);
}

EAPI int
evas_textblock2_cursor_pos_get(const Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Node_Text *n;
   size_t npos = 0;

   if (!cur) return -1;
   TB_NULL_CHECK(cur->node, 0);
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);
   n = o->text_nodes;
   while (n != cur->node)
     {
        npos += eina_ustrbuf_length_get(n->unicode);
        n = _NODE_TEXT(EINA_INLIST_GET(n)->next);
     }
   return npos + cur->pos;
}

EAPI void
evas_textblock2_cursor_pos_set(Evas_Textblock2_Cursor *cur, int _pos)
{
   Evas_Object_Textblock2_Node_Text *n;
   size_t pos;

   if (!cur) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   if (_pos < 0)
     {
        pos = 0;
     }
   else
     {
        pos = (size_t) _pos;
     }

   n = o->text_nodes;
   while (n && (pos >= eina_ustrbuf_length_get(n->unicode)))
     {
        pos -= eina_ustrbuf_length_get(n->unicode);
        n = _NODE_TEXT(EINA_INLIST_GET(n)->next);
     }

   if (n)
     {
        cur->node = n;
        cur->pos = pos;
     }
   else if (o->text_nodes)
     {
        /* In case we went pass the last node, we need to put the cursor
         * at the absolute end. */
        Evas_Object_Textblock2_Node_Text *last_n;

        last_n = _NODE_TEXT(EINA_INLIST_GET(o->text_nodes)->last);
        pos = eina_ustrbuf_length_get(last_n->unicode);

        cur->node = last_n;
        cur->pos = pos;
     }

}

EAPI Eina_Bool
evas_textblock2_cursor_line_set(Evas_Textblock2_Cursor *cur, int line)
{
   Evas_Object_Textblock2_Line *ln;
   Evas_Object_Textblock2_Item *it;

   if (!cur) return EINA_FALSE;

   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   ln = _find_layout_line_num(cur->obj, line);
   if (!ln) return EINA_FALSE;
   it = (Evas_Object_Textblock2_Item *)ln->items;
   if (it)
     {
        cur->pos = it->text_pos;
        cur->node = it->text_node;
     }
   else
     {
        cur->pos = 0;

        cur->node = o->text_nodes;
     }
   return EINA_TRUE;
}

EAPI int
evas_textblock2_cursor_compare(const Evas_Textblock2_Cursor *cur1, const Evas_Textblock2_Cursor *cur2)
{
   Eina_Inlist *l1, *l2;

   if (!cur1) return 0;
   if (!cur2) return 0;
   if (cur1->obj != cur2->obj) return 0;
   if ((!cur1->node) || (!cur2->node)) return 0;
   if (cur1->node == cur2->node)
     {
        if (cur1->pos < cur2->pos) return -1; /* cur1 < cur2 */
        else if (cur1->pos > cur2->pos) return 1; /* cur2 < cur1 */
        return 0;
     }
   for (l1 = EINA_INLIST_GET(cur1->node),
         l2 = EINA_INLIST_GET(cur1->node); (l1) || (l2);)
     {
        if (l1 == EINA_INLIST_GET(cur2->node)) return 1; /* cur2 < cur 1 */
        else if (l2 == EINA_INLIST_GET(cur2->node)) return -1; /* cur1 < cur 2 */
        else if (!l1) return -1; /* cur1 < cur 2 */
        else if (!l2) return 1; /* cur2 < cur 1 */
        l1 = l1->prev;
        l2 = l2->next;
     }
   return 0;
}

EAPI void
evas_textblock2_cursor_copy(const Evas_Textblock2_Cursor *cur, Evas_Textblock2_Cursor *cur_dest)
{
   if (!cur) return;
   if (!cur_dest) return;
   if (cur->obj != cur_dest->obj) return;
   cur_dest->pos = cur->pos;
   cur_dest->node = cur->node;

}

/* text controls */
/**
 * @internal
 * Free a text node. Shouldn't be used usually, it's better to use
 * @ref _evas_textblock2_node_text_remove for most cases .
 *
 * @param n the text node to free
 * @see _evas_textblock2_node_text_remove
 */
static void
_evas_textblock2_node_text_free(Evas_Object_Textblock2_Node_Text *n)
{
   if (!n) return;
   eina_ustrbuf_free(n->unicode);
   if (n->utf8)
      free(n->utf8);
   if (n->par)
      n->par->text_node = NULL;
   free(n);
}

/**
 * @internal
 * Create a new text node
 *
 * @return the new text node.
 */
static Evas_Object_Textblock2_Node_Text *
_evas_textblock2_node_text_new(void)
{
   Evas_Object_Textblock2_Node_Text *n;

   n = calloc(1, sizeof(Evas_Object_Textblock2_Node_Text));
   n->unicode = eina_ustrbuf_new();
   /* We want to layout each paragraph at least once. */
   n->dirty = EINA_TRUE;
   n->is_new = EINA_TRUE;

   return n;
}

/**
 * @internal
 * Break a paragraph. This does not add a PS but only splits the paragraph
 * where a ps was just added!
 *
 * @param cur the cursor to break at.
 * @param fnode the format node of the PS just added.
 * @return Returns no value.
 */
static void
_evas_textblock2_cursor_break_paragraph(Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Node_Text *n;

   if (!cur) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   n = _evas_textblock2_node_text_new();
   o->text_nodes = _NODE_TEXT(eina_inlist_append_relative(
            EINA_INLIST_GET(o->text_nodes),
            EINA_INLIST_GET(n),
            EINA_INLIST_GET(cur->node)));
   /* Handle text and format changes. */
   if (cur->node)
     {
        size_t len, start;
        const Eina_Unicode *text;

        /* cur->pos now points to the PS, move after. */
        start = cur->pos + 1;
        len = eina_ustrbuf_length_get(cur->node->unicode) - start;
        if (len > 0)
          {
             text = eina_ustrbuf_string_get(cur->node->unicode);
             eina_ustrbuf_append_length(n->unicode, text + start, len);
             eina_ustrbuf_remove(cur->node->unicode, start, start + len);
             cur->node->dirty = EINA_TRUE;
          }
     }
}

/**
 * @internal
 * Set the node and offset of all the curs after cur.
 *
 * @param cur the cursor.
 * @param n the current textblock2 node.
 * @param new_node the new node to set.
 */
static void
_evas_textblock2_cursors_set_node(Evas_Textblock2_Data *o,
      const Evas_Object_Textblock2_Node_Text *n,
      Evas_Object_Textblock2_Node_Text *new_node)
{
   Eina_List *l;
   Evas_Textblock2_Cursor *data;

   if (n == o->cursor->node)
     {
        o->cursor->pos = 0;
        o->cursor->node = new_node;
     }
   EINA_LIST_FOREACH(o->cursors, l, data)
     {
        if (n == data->node)
          {
             data->pos = 0;
             data->node = new_node;
          }
     }
}

/**
 * @internal
 * Update the offset of all the cursors after cur.
 *
 * @param cur the cursor.
 * @param n the current textblock2 node.
 * @param start the starting pos.
 * @param offset how much to adjust (can be negative).
 */
static void
_evas_textblock2_cursors_update_offset(const Evas_Textblock2_Cursor *cur,
      const Evas_Object_Textblock2_Node_Text *n,
      size_t start, int offset)
{
   Eina_List *l;
   Evas_Textblock2_Cursor *data;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   if (cur != o->cursor)
     {
        if ((n == o->cursor->node) &&
              (o->cursor->pos > start))
          {
             if ((offset < 0) && (o->cursor->pos <= (size_t) (-1 * offset)))
               {
                  o->cursor->pos = 0;
               }
             else
               {
                  o->cursor->pos += offset;
               }
          }
     }
   EINA_LIST_FOREACH(o->cursors, l, data)
     {
        if (data != cur)
          {
             if ((n == data->node) &&
                   (data->pos > start))
               {
                  if ((offset < 0) && (data->pos <= (size_t) (-1 * offset)))
                    {
                       data->pos = 0;
                    }
                  else
                    {
                       data->pos += offset;
                    }
               }
             else if (!data->node)
               {
                  data->node = o->text_nodes;
                  data->pos = 0;
               }
          }
     }
}

/**
 * @internal
 * Mark that the textblock2 has changed.
 *
 * @param o the textblock2 object.
 * @param obj the evas object.
 */
static void
_evas_textblock2_changed(Evas_Textblock2_Data *o, Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   LYDBG("ZZ: invalidate 1 %p\n", eo_obj);
   o->formatted.valid = 0;
   o->native.valid = 0;
   o->content_changed = 1;
   if (o->markup_text)
     {
        free(o->markup_text);
        o->markup_text = NULL;
     }

   evas_object_change(eo_obj, obj);
}

static void
_evas_textblock2_invalidate_all(Evas_Textblock2_Data *o)
{
   Evas_Object_Textblock2_Node_Text *n;

   EINA_INLIST_FOREACH(o->text_nodes, n)
     {
        n->dirty = EINA_TRUE;
     }
}

static int
_evas_textblock2_cursor_text_append(Evas_Textblock2_Cursor *cur, const char *_text)
{
   Evas_Object_Textblock2_Node_Text *n;
   Eina_Unicode *text;
   int len = 0;

   if (!cur) return 0;
   text = eina_unicode_utf8_to_unicode(_text, &len);
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   n = cur->node;
   if (n)
     {
     }
   else if (o->text_nodes)
     {
        n = cur->node = o->text_nodes;
        cur->pos = 0;
     }
   else
     {
        n = _evas_textblock2_node_text_new();
        o->text_nodes = _NODE_TEXT(eina_inlist_append(
                 EINA_INLIST_GET(o->text_nodes),
                 EINA_INLIST_GET(n)));
        cur->node = n;
     }

   eina_ustrbuf_insert_length(n->unicode, text, len, cur->pos);

   int i;
   for (i = 0 ; i < len ; i++)
     {
        if (text[i] == _PARAGRAPH_SEPARATOR)
          {
             _evas_textblock2_cursor_break_paragraph(cur);
          }
        evas_textblock2_cursor_char_next(cur);
     }

   /* Update all the cursors after our position. */
   _evas_textblock2_cursors_update_offset(cur, cur->node, cur->pos, len);

   _evas_textblock2_changed(o, cur->obj);
   n->dirty = EINA_TRUE;
   free(text);

   if (!o->cursor->node)
      o->cursor->node = o->text_nodes;
   return len;
}

EAPI int
evas_textblock2_cursor_text_prepend(Evas_Textblock2_Cursor *cur, const char *_text)
{
   int len;
   /*append is essentially prepend without advancing */
   len = _evas_textblock2_cursor_text_append(cur, _text);
   if (len == 0) return 0;
   cur->pos += len; /*Advance */
   return len;
}

EOLIAN static void
_evas_textblock2_efl_text_text_set(Eo *obj, Evas_Textblock2_Data *pd EINA_UNUSED, const char *text)
{
   /* FIXME: This is not even slightly correct. */
   Evas_Textblock2_Cursor *main_cur = evas_object_textblock2_cursor_get(obj);
   evas_textblock2_cursor_text_prepend(main_cur, text);
}


EOLIAN static const char *
_evas_textblock2_efl_text_text_get(Eo *obj, Evas_Textblock2_Data *pd)
{
   (void) obj;
   (void) pd;
   /* FIXME: Do something. */
   return "";
}

EAPI void
evas_textblock2_cursor_char_delete(Evas_Textblock2_Cursor *cur)
{
   Evas_Object_Textblock2_Node_Text *n, *n2;
   const Eina_Unicode *text;
   int chr, ind, ppos;

   if (!cur || !cur->node) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);
   n = cur->node;

   text = eina_ustrbuf_string_get(n->unicode);
   ind = cur->pos;
   if (text[ind])
      chr = text[ind++];
   else
      chr = 0;

   if (chr == 0) return;
   ppos = cur->pos;
   eina_ustrbuf_remove(n->unicode, cur->pos, ind);

   if (chr == _PARAGRAPH_SEPARATOR)
     {
        _evas_textblock2_cursor_nodes_merge(cur);
     }

   if (cur->pos == eina_ustrbuf_length_get(n->unicode))
     {
	n2 = _NODE_TEXT(EINA_INLIST_GET(n)->next);
	if (n2)
	  {
	     cur->node = n2;
	     cur->pos = 0;
	  }
     }

   _evas_textblock2_cursors_update_offset(cur, n, ppos, -(ind - ppos));
   _evas_textblock2_changed(o, cur->obj);
   cur->node->dirty = EINA_TRUE;
}

EAPI void
evas_textblock2_cursor_range_delete(Evas_Textblock2_Cursor *cur1, Evas_Textblock2_Cursor *cur2)
{
   Evas_Object_Textblock2_Node_Text *n1, *n2;
   Eina_Bool should_merge = EINA_FALSE, reset_cursor = EINA_FALSE;

   if (!cur1 || !cur1->node) return;
   if (!cur2 || !cur2->node) return;
   if (cur1->obj != cur2->obj) return;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur1->obj, MY_CLASS);
   if (evas_textblock2_cursor_compare(cur1, cur2) > 0)
     {
	Evas_Textblock2_Cursor *tc;

	tc = cur1;
	cur1 = cur2;
	cur2 = tc;
     }
   n1 = cur1->node;
   n2 = cur2->node;
   if ((evas_textblock2_cursor_compare(o->cursor, cur1) >= 0) &&
         (evas_textblock2_cursor_compare(cur2, o->cursor) >= 0))
     {
        reset_cursor = EINA_TRUE;
     }


   if (n1 == n2)
     {
        if ((cur1->pos == 0) &&
              (cur2->pos == eina_ustrbuf_length_get(n1->unicode)))
          {
             /* Remove the whole node. */
             Evas_Object_Textblock2_Node_Text *n =
                _NODE_TEXT(EINA_INLIST_GET(n1)->next);
             if (n)
               {
                  should_merge = EINA_TRUE;
               }
          }
        else
          {
             /* FIXME: Handle the case we are deleting a ps. */
          }
        eina_ustrbuf_remove(n1->unicode, cur1->pos, cur2->pos);
        _evas_textblock2_cursors_update_offset(cur1, cur1->node, cur1->pos, - (cur2->pos - cur1->pos));
     }
   else
     {
        Evas_Object_Textblock2_Node_Text *n;
        int len;
        n = _NODE_TEXT(EINA_INLIST_GET(n1)->next);
        /* Remove all the text nodes between */
        while (n && (n != n2))
          {
             Evas_Object_Textblock2_Node_Text *nnode;

             nnode = _NODE_TEXT(EINA_INLIST_GET(n)->next);
             _evas_textblock2_nodes_merge(o, n1);
             n = nnode;
          }
        /* After we merged all the nodes, move the formats to the start of
         * the range. */

        /* FIXME: Handle the case we are deleting a ps. */

        /* Remove the formats and the strings in the first and last nodes */
        len = eina_ustrbuf_length_get(n1->unicode);
        eina_ustrbuf_remove(n1->unicode, cur1->pos, len);
        eina_ustrbuf_remove(n2->unicode, 0, cur2->pos);
        /* Merge the nodes because we removed the PS */
        _evas_textblock2_cursors_update_offset(cur1, cur1->node, cur1->pos,
                                              -cur1->pos);
        _evas_textblock2_cursors_update_offset(cur2, cur2->node, 0, -cur2->pos);
        cur2->pos = 0;
        _evas_textblock2_nodes_merge(o, n1);
     }

   n1 = cur1->node;
   n2 = cur2->node;
   n1->dirty = n2->dirty = EINA_TRUE;

   if (should_merge)
     {
        /* We call this function instead of the cursor one because we already
         * updated the cursors */
        _evas_textblock2_nodes_merge(o, n1);
     }

   evas_textblock2_cursor_copy(cur1, cur2);
   if (reset_cursor)
     evas_textblock2_cursor_copy(cur1, o->cursor);

   _evas_textblock2_changed(o, cur1->obj);
}


EAPI char *
evas_textblock2_cursor_content_get(const Evas_Textblock2_Cursor *cur)
{
   if (!cur || !cur->node)
      return NULL;

     {
        const Eina_Unicode *ustr;
        Eina_Unicode buf[2];
        char *s;

        ustr = eina_ustrbuf_string_get(cur->node->unicode);
        buf[0] = ustr[cur->pos];
        buf[1] = 0;
        s = eina_unicode_unicode_to_utf8(buf, NULL);

        return s;
     }
}

EAPI char *
evas_textblock2_cursor_range_text_get(const Evas_Textblock2_Cursor *cur1, const Evas_Textblock2_Cursor *_cur2)
{
   Eina_UStrbuf *buf;
   Evas_Object_Textblock2_Node_Text *n1, *n2;
   Evas_Textblock2_Cursor *cur2;

   if (!cur1 || !cur1->node) return NULL;
   if (!_cur2 || !_cur2->node) return NULL;
   if (cur1->obj != _cur2->obj) return NULL;
   buf = eina_ustrbuf_new();

   if (evas_textblock2_cursor_compare(cur1, _cur2) > 0)
     {
	const Evas_Textblock2_Cursor *tc;

	tc = cur1;
	cur1 = _cur2;
	_cur2 = tc;
     }
   n1 = cur1->node;
   n2 = _cur2->node;
   /* Work on a local copy of the cur */
   cur2 = alloca(sizeof(Evas_Textblock2_Cursor));
   cur2->obj = _cur2->obj;
   evas_textblock2_cursor_copy(_cur2, cur2);


   if (n1 == n2)
     {
        const Eina_Unicode *tmp;
        tmp = eina_ustrbuf_string_get(n1->unicode);
        eina_ustrbuf_append_length(buf, tmp + cur1->pos, cur2->pos - cur1->pos);
     }
   else
     {
        const Eina_Unicode *tmp;
        tmp = eina_ustrbuf_string_get(n1->unicode);
        eina_ustrbuf_append(buf, tmp + cur1->pos);
        n1 = _NODE_TEXT(EINA_INLIST_GET(n1)->next);
        while (n1 != n2)
          {
             tmp = eina_ustrbuf_string_get(n1->unicode);
             eina_ustrbuf_append_length(buf, tmp,
                   eina_ustrbuf_length_get(n1->unicode));
             n1 = _NODE_TEXT(EINA_INLIST_GET(n1)->next);
          }
        tmp = eina_ustrbuf_string_get(n2->unicode);
        eina_ustrbuf_append_length(buf, tmp, cur2->pos);
     }

   /* Free and return */
     {
        char *ret;
        ret = eina_unicode_unicode_to_utf8(eina_ustrbuf_string_get(buf), NULL);
        eina_ustrbuf_free(buf);
        return ret;
     }
}

#ifdef BIDI_SUPPORT
static Evas_Object_Textblock2_Line*
_find_layout_line_by_item(Evas_Object_Textblock2_Paragraph *par, Evas_Object_Textblock2_Item *_it)
{
   Evas_Object_Textblock2_Line *ln;

   EINA_INLIST_FOREACH(par->lines, ln)
     {
        Evas_Object_Textblock2_Item *it;

        EINA_INLIST_FOREACH(ln->items, it)
          {
             if (_it == it)
               return ln;
          }
     }
   return NULL;
}
#endif

EAPI Eina_Bool
evas_textblock2_cursor_geometry_bidi_get(const Evas_Textblock2_Cursor *cur, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch, Evas_Coord *cx2, Evas_Coord *cy2, Evas_Coord *cw2, Evas_Coord *ch2, Evas_Textblock2_Cursor_Type ctype)
{
   if (!cur) return EINA_FALSE;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   if (ctype == EVAS_TEXTBLOCK2_CURSOR_UNDER)
     {
        evas_textblock2_cursor_pen_geometry_get(cur, cx, cy, cw, ch);
        return EINA_FALSE;
     }

#ifdef BIDI_SUPPORT
#define IS_RTL(par) ((par) % 2)
#define IS_DIFFERENT_DIR(l1, l2) (IS_RTL(l1) != IS_RTL(l2))
   else
     {
        Evas_Object_Textblock2_Line *ln = NULL;
        Evas_Object_Textblock2_Item *it = NULL;
        _find_layout_item_match(cur, &ln, &it);
        if (ln && it)
          {
             if (ln->par->is_bidi)
               {
                  if (cw) *cw = 0;
                  if (cw2) *cw2 = 0;

                  /* If we are at the start or the end of the item there's a chance
                   * we'll want a split cursor.  */
                  Evas_Object_Textblock2_Item *previt = NULL;
                  Evas_Object_Textblock2_Item *it1 = NULL, *it2 = NULL;
                  Evas_Coord adv1 = 0, adv2 = 0;

                  if (cur->pos == it->text_pos)
                    {
                       EvasBiDiLevel par_level, it_level, previt_level;

                       _layout_update_bidi_props(o, ln->par);
                       par_level = *(ln->par->bidi_props->embedding_levels);
                       it_level = ln->par->bidi_props->embedding_levels[it->text_pos];
                       /* Get the logically previous item. */
                         {
                            Eina_List *itr;
                            Evas_Object_Textblock2_Item *ititr;

                            EINA_LIST_FOREACH(ln->par->logical_items, itr, ititr)
                              {
                                 if (ititr == it)
                                   break;
                                 previt = ititr;
                              }

                            if (previt)
                              {
                                 previt_level = ln->par->bidi_props->embedding_levels[previt->text_pos];
                              }
                         }

                       if (previt && (it_level != previt_level))
                         {
                            Evas_Object_Textblock2_Item *curit = NULL, *curit_opp = NULL;
                            EvasBiDiLevel cur_level;

                            if (it_level > previt_level)
                              {
                                 curit = it;
                                 curit_opp = previt;
                                 cur_level = it_level;
                              }
                            else
                              {
                                 curit = previt;
                                 curit_opp = it;
                                 cur_level = previt_level;
                              }

                            if (((curit == it) && (!IS_RTL(par_level))) ||
                                ((curit == previt) && (IS_RTL(par_level))))
                              {
                                 adv1 = (IS_DIFFERENT_DIR(cur_level, par_level)) ?
                                                          curit_opp->adv : 0;
                                 adv2 = curit->adv;
                              }
                            else if (((curit == previt) && (!IS_RTL(par_level))) ||
                                     ((curit == it) && (IS_RTL(par_level))))
                              {
                                 adv1 = (IS_DIFFERENT_DIR(cur_level, par_level)) ?
                                                          0 : curit->adv;
                                 adv2 = 0;
                              }

                            if (!IS_DIFFERENT_DIR(cur_level, par_level))
                              curit_opp = curit;

                            it1 = curit_opp;
                            it2 = curit;
                         }
                       /* Clear the bidi props because we don't need them anymore. */
                       evas_bidi_paragraph_props_unref(ln->par->bidi_props);
                       ln->par->bidi_props = NULL;
                    }
                  /* Handling last char in line (or in paragraph).
                   * T.e. prev condition didn't work, so we are not standing in the beginning of item,
                   * but in the end of line or paragraph. */
                  else if (evas_textblock2_cursor_eol_get(cur))
                    {
                       EvasBiDiLevel par_level, it_level;

                       _layout_update_bidi_props(o, ln->par);
                       par_level = *(ln->par->bidi_props->embedding_levels);
                       it_level = ln->par->bidi_props->embedding_levels[it->text_pos];

                       if (it_level > par_level)
                         {
                            Evas_Object_Textblock2_Item *lastit = it;

                            if (IS_RTL(par_level)) /* RTL par*/
                              {
                                 /*  We know, that all the items before current are of the same or bigger embedding level.
                                  *  So search backwards for the first one. */
                                 while (EINA_INLIST_GET(lastit)->prev)
                                   {
                                      lastit = _EINA_INLIST_CONTAINER(it, EINA_INLIST_GET(lastit)->prev);
                                   }

                                 adv1 = 0;
                                 adv2 = it->adv;
                              }
                            else /* LTR par */
                              {
                                 /*  We know, that all the items after current are of bigger or same embedding level.
                                  *  So search forward for the last one. */
                                 while (EINA_INLIST_GET(lastit)->next)
                                   {
                                      lastit = _EINA_INLIST_CONTAINER(it, EINA_INLIST_GET(lastit)->next);
                                   }

                                 adv1 = lastit->adv;
                                 adv2 = 0;
                              }

                            it1 = lastit;
                            it2 = it;
                         }
                       /* Clear the bidi props because we don't need them anymore. */
                       evas_bidi_paragraph_props_unref(ln->par->bidi_props);
                       ln->par->bidi_props = NULL;
                    }

                  if (it1 && it2)
                    {
                       Evas_Object_Textblock2_Line *ln1 = NULL, *ln2 = NULL;
                       ln1 = _find_layout_line_by_item(ln->par, it1);
                       if (cx) *cx = ln1->x + it1->x + adv1;
                       if (cy) *cy = ln1->par->y + ln1->y;
                       if (ch) *ch = ln1->h;

                       ln2 = _find_layout_line_by_item(ln->par, it2);
                       if (cx2) *cx2 = ln2->x + it2->x + adv2;
                       if (cy2) *cy2 = ln2->par->y + ln2->y;
                       if (ch2) *ch2 = ln2->h;

                       return EINA_TRUE;
                    }
               }
          }
     }
#undef IS_DIFFERENT_DIR
#undef IS_RTL
#else
   (void) cx2;
   (void) cy2;
   (void) cw2;
   (void) ch2;
#endif
   evas_textblock2_cursor_geometry_get(cur, cx, cy, cw, ch, NULL, ctype);
   return EINA_FALSE;
}

EAPI int
evas_textblock2_cursor_geometry_get(const Evas_Textblock2_Cursor *cur, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch, Evas_BiDi_Direction *dir, Evas_Textblock2_Cursor_Type ctype)
{
   int ret = -1;
   if (!cur) return -1;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   if (ctype == EVAS_TEXTBLOCK2_CURSOR_UNDER)
     {
        Evas_Object_Textblock2_Line *ln;
        Evas_Object_Textblock2_Item *it;

        ret = evas_textblock2_cursor_pen_geometry_get(cur, cx, cy, cw, ch);
        _find_layout_item_match(cur, &ln, &it);
        if (ret >= 0)
          {
             Evas_BiDi_Direction itdir =
                (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ?
                _ITEM_TEXT(it)->text_props.bidi_dir :
                _ITEM_FORMAT(it)->bidi_dir;
             if (dir) *dir = itdir;
          }
     }
   else if (ctype == EVAS_TEXTBLOCK2_CURSOR_BEFORE)
     {
        /* In the case of a "before cursor", we should get the coordinates
         * of just after the previous char (which in bidi text may not be
         * just before the current char). */
        Evas_Coord x, y, w, h;
        Evas_Object_Textblock2_Line *ln;
        Evas_Object_Textblock2_Item *it;

        ret = evas_textblock2_cursor_pen_geometry_get(cur, &x, &y, &w, &h);
        _find_layout_item_match(cur, &ln, &it);
        if (ret >= 0)
          {
             Evas_BiDi_Direction itdir =
                (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ?
                _ITEM_TEXT(it)->text_props.bidi_dir :
                _ITEM_FORMAT(it)->bidi_dir;
             if (itdir == EVAS_BIDI_DIRECTION_RTL)
               {
                  if (cx) *cx = x + w;
               }
             else
               {
                  if (cx) *cx = x;
               }
             if (cy) *cy = y;
             if (cw) *cw = 0;
             if (ch) *ch = h;
             if (dir) *dir = itdir;
          }
     }
   return ret;
}

/**
 * @internal
 * Returns the geometry/pen position (depending on query_func) of the char
 * at pos.
 *
 * @param cur the position of the char.
 * @param query_func the query function to use.
 * @param cx the x of the char (or pen_x in the case of pen position).
 * @param cy the y of the char.
 * @param cw the w of the char (or advance in the case pen position).
 * @param ch the h of the char.
 * @return line number of the char on success, -1 on error.
 */
static int
_evas_textblock2_cursor_char_pen_geometry_common_get(int (*query_func) (void *data, Evas_Font_Set *font, const Evas_Text_Props *intl_props, int pos, int *cx, int *cy, int *cw, int *ch), const Evas_Textblock2_Cursor *cur, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   Evas_Object_Textblock2_Line *ln = NULL;
   Evas_Object_Textblock2_Item *it = NULL;
   Evas_Object_Textblock2_Text_Item *ti = NULL;
   Evas_Object_Textblock2_Format_Item *fi = NULL;
   int x = 0, y = 0, w = 0, h = 0;
   int pos;
   Eina_Bool previous_format;

   if (!cur) return -1;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   if (!cur->node)
     {
        if (!o->text_nodes)
          {
             if (!o->paragraphs) return -1;
             ln = o->paragraphs->lines;
             if (!ln) return -1;
             if (cx) *cx = ln->x;
             if (cy) *cy = ln->par->y + ln->y;
             if (cw) *cw = ln->w;
             if (ch) *ch = ln->h;
             return ln->par->line_no + ln->line_no;
          }
        else
          return -1;
     }

   previous_format = _find_layout_item_match(cur, &ln, &it);
   if (!it)
     {
        return -1;
     }
   if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
     {
        ti = _ITEM_TEXT(it);
     }
   else
     {
        fi = _ITEM_FORMAT(it);
     }

   if (ln && ti)
     {
        pos = cur->pos - ti->parent.text_pos;

        if (pos < 0) pos = 0;
        if (ti->parent.format->font.font)
          {
             Evas_Object_Protected_Data *obj = eo_data_scope_get(cur->obj, EVAS_OBJECT_CLASS);
             query_func(ENDT,
                   ti->parent.format->font.font,
                   &ti->text_props,
                   pos,
                   &x, &y, &w, &h);
          }

        x += ln->x + _ITEM(ti)->x;

        if (x < ln->x)
          {
             x = ln->x;
          }
	y = ln->par->y + ln->y;
	h = ln->h;
     }
   else if (ln && fi)
     {
        if (previous_format)
          {
             if (_IS_LINE_SEPARATOR(fi->item))
               {
                  x = 0;
                  y = ln->par->y + ln->y + ln->h;
               }
             else
               {
#ifdef BIDI_SUPPORT
                  if (ln->par->direction == EVAS_BIDI_DIRECTION_RTL)
                    {
                       x = ln->x;
                    }
                  else
#endif
                    {
                       x = ln->x + ln->w;
                    }
                  y = ln->par->y + ln->y;
               }
             w = 0;
             h = ln->h;
          }
        else
          {
             x = ln->x + _ITEM(fi)->x;
             y = ln->par->y + ln->y;
             w = _ITEM(fi)->w;
             h = ln->h;
          }
     }
   else
     {
	return -1;
     }
   if (cx) *cx = x;
   if (cy) *cy = y;
   if (cw) *cw = w;
   if (ch) *ch = h;
   return ln->par->line_no + ln->line_no;
}

EAPI int
evas_textblock2_cursor_char_geometry_get(const Evas_Textblock2_Cursor *cur, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   if (!cur) return -1;
   Evas_Object_Protected_Data *obj = eo_data_scope_get(cur->obj, EVAS_OBJECT_CLASS);
   return _evas_textblock2_cursor_char_pen_geometry_common_get(
         ENFN->font_char_coords_get, cur, cx, cy, cw, ch);
}

EAPI int
evas_textblock2_cursor_pen_geometry_get(const Evas_Textblock2_Cursor *cur, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   if (!cur) return -1;
   Evas_Object_Protected_Data *obj = eo_data_scope_get(cur->obj, EVAS_OBJECT_CLASS);
   return _evas_textblock2_cursor_char_pen_geometry_common_get(
         ENFN->font_pen_coords_get, cur, cx, cy, cw, ch);
}

EAPI int
evas_textblock2_cursor_line_geometry_get(const Evas_Textblock2_Cursor *cur, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   Evas_Object_Textblock2_Line *ln = NULL;
   Evas_Object_Textblock2_Item *it = NULL;
   int x, y, w, h;

   if (!cur) return -1;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   if (!cur->node)
     {
        ln = o->paragraphs->lines;
     }
   else
     {
        _find_layout_item_match(cur, &ln, &it);
     }
   if (!ln) return -1;
   x = ln->x;
   y = ln->par->y + ln->y;
   w = ln->w;
   h = ln->h;
   if (cx) *cx = x;
   if (cy) *cy = y;
   if (cw) *cw = w;
   if (ch) *ch = h;
   return ln->par->line_no + ln->line_no;
}

EAPI Eina_Bool
evas_textblock2_cursor_char_coord_set(Evas_Textblock2_Cursor *cur, Evas_Coord x, Evas_Coord y)
{
   Evas_Object_Textblock2_Paragraph *found_par;
   Evas_Object_Textblock2_Line *ln;
   Evas_Object_Textblock2_Item *it = NULL;

   if (!cur) return EINA_FALSE;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   x += o->style_pad.l;
   y += o->style_pad.t;

   found_par = _layout_find_paragraph_by_y(o, y);
   if (found_par)
     {
        EINA_INLIST_FOREACH(found_par->lines, ln)
          {
             if (ln->par->y + ln->y > y) break;
             if ((ln->par->y + ln->y <= y) && ((ln->par->y + ln->y + ln->h) > y))
               {
                  /* If before or after the line, go to start/end according
                   * to paragraph direction. */
                  if (x < ln->x)
                    {
                       cur->pos = ln->items->text_pos;
                       cur->node = found_par->text_node;
                       if (found_par->direction == EVAS_BIDI_DIRECTION_RTL)
                         {
                            evas_textblock2_cursor_line_char_last(cur);
                         }
                       else
                         {
                            evas_textblock2_cursor_line_char_first(cur);
                         }
                       return EINA_TRUE;
                    }
                  else if (x >= ln->x + ln->w)
                    {
                       cur->pos = ln->items->text_pos;
                       cur->node = found_par->text_node;
                       if (found_par->direction == EVAS_BIDI_DIRECTION_RTL)
                         {
                            evas_textblock2_cursor_line_char_first(cur);
                         }
                       else
                         {
                            evas_textblock2_cursor_line_char_last(cur);
                         }
                       return EINA_TRUE;
                    }

                  Evas_Object_Protected_Data *obj = eo_data_scope_get(cur->obj, EVAS_OBJECT_CLASS);
                  EINA_INLIST_FOREACH(ln->items, it)
                    {
                       if (((it->x + ln->x) <= x) && (((it->x + ln->x) + it->adv) > x))
                         {
                            if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
                              {
                                 int pos;
                                 int cx, cy, cw, ch;
                                 Evas_Object_Textblock2_Text_Item *ti;
                                 ti = _ITEM_TEXT(it);

                                 pos = -1;
                                 if (ti->parent.format->font.font)
                                   pos = ENFN->font_char_at_coords_get(
                                         ENDT,
                                         ti->parent.format->font.font,
                                         &ti->text_props,
                                         x - it->x - ln->x, 0,
                                         &cx, &cy, &cw, &ch);
                                 if (pos < 0)
                                   return EINA_FALSE;
                                 cur->pos = pos + it->text_pos;
                                 cur->node = it->text_node;
                                 return EINA_TRUE;
                              }
                            else
                              {
                                 Evas_Object_Textblock2_Format_Item *fi;
                                 fi = _ITEM_FORMAT(it);
                                 cur->pos = fi->parent.text_pos;
                                 cur->node = found_par->text_node;
                                 return EINA_TRUE;
                              }
                         }
                    }
               }
          }
     }

   if (o->paragraphs)
     {
        Evas_Object_Textblock2_Line *first_line = o->paragraphs->lines;
        if (y >= o->paragraphs->y + o->formatted.h)
          {
             /* If we are after the last paragraph, use the last position in the
              * text. */
             evas_textblock2_cursor_paragraph_last(cur);
             return EINA_TRUE;
          }
        else if (o->paragraphs && (y < (o->paragraphs->y + first_line->y)))
          {
             evas_textblock2_cursor_paragraph_first(cur);
             return EINA_TRUE;
          }
     }

   return EINA_FALSE;
}

EAPI int
evas_textblock2_cursor_line_coord_set(Evas_Textblock2_Cursor *cur, Evas_Coord y)
{
   Evas_Object_Textblock2_Paragraph *found_par;
   Evas_Object_Textblock2_Line *ln;

   if (!cur) return -1;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur->obj, MY_CLASS);

   _relayout_if_needed(cur->obj, o);

   y += o->style_pad.t;

   found_par = _layout_find_paragraph_by_y(o, y);

   if (found_par)
     {
        EINA_INLIST_FOREACH(found_par->lines, ln)
          {
             if (ln->par->y + ln->y > y) break;
             if ((ln->par->y + ln->y <= y) && ((ln->par->y + ln->y + ln->h) > y))
               {
                  evas_textblock2_cursor_line_set(cur, ln->par->line_no +
                        ln->line_no);
                  return ln->par->line_no + ln->line_no;
               }
          }
     }
   else if (o->paragraphs && (y >= o->paragraphs->y + o->formatted.h))
     {
        int line_no = 0;
        /* If we are after the last paragraph, use the last position in the
         * text. */
        evas_textblock2_cursor_paragraph_last(cur);
        if (cur->node && cur->node->par)
          {
             line_no = cur->node->par->line_no;
             if (cur->node->par->lines)
               {
                  line_no += ((Evas_Object_Textblock2_Line *)
                        EINA_INLIST_GET(cur->node->par->lines)->last)->line_no;
               }
          }
        return line_no;
     }
   else if (o->paragraphs && (y < o->paragraphs->y))
     {
        int line_no = 0;
        evas_textblock2_cursor_paragraph_first(cur);
        if (cur->node && cur->node->par)
          {
             line_no = cur->node->par->line_no;
          }
        return line_no;
     }
   return -1;
}

/**
 * @internal
 * Updates x and w according to the text direction, position in text and
 * if it's a special case switch
 *
 * @param ti the text item we are working on
 * @param x the current x (we get) and the x we return
 * @param w the current w (we get) and the w we return
 * @param start if this is the first item or not
 * @param switch_items toogles item switching (rtl cases)
 */
static void
_evas_textblock2_range_calc_x_w(const Evas_Object_Textblock2_Item *it,
      Evas_Coord *x, Evas_Coord *w, Eina_Bool start, Eina_Bool switch_items)
{
   if ((start && !switch_items) || (!start && switch_items))
     {
#ifdef BIDI_SUPPORT
        if (((it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) &&
            _ITEM_TEXT(it)->text_props.bidi_dir == EVAS_BIDI_DIRECTION_RTL)
            ||
            ((it->type == EVAS_TEXTBLOCK2_ITEM_FORMAT) &&
             _ITEM_FORMAT(it)->bidi_dir == EVAS_BIDI_DIRECTION_RTL))
          {
             *w = *x + *w;
             *x = 0;
          }
        else
#endif
          {
             *w = it->adv - *x;
          }
     }
   else
     {
#ifdef BIDI_SUPPORT
        if (((it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) &&
            _ITEM_TEXT(it)->text_props.bidi_dir == EVAS_BIDI_DIRECTION_RTL)
            ||
            ((it->type == EVAS_TEXTBLOCK2_ITEM_FORMAT) &&
             _ITEM_FORMAT(it)->bidi_dir == EVAS_BIDI_DIRECTION_RTL))
          {
             *x = *x + *w;
             *w = it->adv - *x;
          }
        else
#endif
          {
             *w = *x;
             *x = 0;
          }
     }

}

/**
 * @internal
 * Returns the geometry of the range in line ln. Cur1 is the start cursor,
 * cur2 is the end cursor, NULL means from the start or to the end accordingly.
 * Assumes that ln is valid, and that at least one of cur1 and cur2 is not NULL.
 *
 * @param ln the line to work on.
 * @param cur1 the start cursor
 * @param cur2 the end cursor
 * @return Returns the geometry of the range
 */
static Eina_List *
_evas_textblock2_cursor_range_in_line_geometry_get(
      const Evas_Object_Textblock2_Line *ln, const Evas_Textblock2_Cursor *cur1,
      const Evas_Textblock2_Cursor *cur2)
{
   Evas_Object_Textblock2_Item *it;
   Evas_Object_Textblock2_Item *it1, *it2;
   Eina_List *rects = NULL;
   Evas_Textblock2_Rectangle *tr;
   size_t start, end;
   Eina_Bool switch_items;
   const Evas_Textblock2_Cursor *cur;

   cur = (cur1) ? cur1 : cur2;

   if (!cur) return NULL;
   Evas_Object_Protected_Data *obj = eo_data_scope_get(cur->obj, EVAS_OBJECT_CLASS);

   /* Find the first and last items */
   it1 = it2 = NULL;
   start = end = 0;
   EINA_INLIST_FOREACH(ln->items, it)
     {
        size_t item_len;
        item_len = (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ?
           _ITEM_TEXT(it)->text_props.text_len
           : 1;
        if ((!cur1 || (cur1->pos < it->text_pos + item_len)) &&
              (!cur2 || (cur2->pos >= it->text_pos)))
          {
             if (!it1)
               {
                  it1 = it;
                  start = item_len; /* start stores the first item_len */
               }
             it2 = it;
             end = item_len; /* end stores the last item_len */
          }
     }

   /* If we couldn't find even one item, return */
   if (!it1) return NULL;

   /* If the first item is logically before or equal the second item
    * we have to set start and end differently than in the other case */
   if (it1->text_pos <= it2->text_pos)
     {
        start = (cur1) ? (cur1->pos - it1->text_pos) : 0;
        end = (cur2) ? (cur2->pos - it2->text_pos) : end;
        switch_items = EINA_FALSE;
     }
   else
     {
        start = (cur2) ? (cur2->pos - it1->text_pos) : start;
        end = (cur1) ? (cur1->pos - it2->text_pos) : 0;
        switch_items = EINA_TRUE;
     }

   /* IMPORTANT: Don't use cur1/cur2 past this point (because they probably
    * don't make sense anymore. That's why there are start and end),
    * unless you know what you are doing */

   /* Special case when they share the same item and it's a text item */
   if ((it1 == it2) && (it1->type == EVAS_TEXTBLOCK2_ITEM_TEXT))
     {
        Evas_Coord x1, w1, x2, w2;
        Evas_Coord x, w, y, h;
        Evas_Object_Textblock2_Text_Item *ti;
        int ret = 0;

        ti = _ITEM_TEXT(it1);
        if (ti->parent.format->font.font)
          {
             ret = ENFN->font_pen_coords_get(ENDT,
                   ti->parent.format->font.font,
                   &ti->text_props,
                   start,
                   &x1, &y, &w1, &h);
          }
        if (!ret)
          {
             return NULL;
          }
        ret = ENFN->font_pen_coords_get(ENDT,
              ti->parent.format->font.font,
              &ti->text_props,
              end,
              &x2, &y, &w2, &h);
        if (!ret)
          {
             return NULL;
          }

        /* Make x2 the one on the right */
        if (x2 < x1)
          {
             Evas_Coord tmp;
             tmp = x1;
             x1 = x2;
             x2 = tmp;

             tmp = w1;
             w1 = w2;
             w2 = tmp;
          }

#ifdef BIDI_SUPPORT
        if (ti->text_props.bidi_dir == EVAS_BIDI_DIRECTION_RTL)
          {
             x = x1 + w1;
             w = x2 + w2 - x;
          }
        else
#endif
          {
             x = x1;
             w = x2 - x1;
          }
        if (w > 0)
          {
             tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
             rects = eina_list_append(rects, tr);
             tr->x = ln->x + it1->x + x;
             tr->y = ln->par->y + ln->y;
             tr->h = ln->h;
             tr->w = w;
          }
     }
   else if ((it1 == it2) && (it1->type != EVAS_TEXTBLOCK2_ITEM_TEXT))
     {
        Evas_Coord x, w;
        x = 0;
        w = it1->w;
        _evas_textblock2_range_calc_x_w(it1, &x, &w, EINA_TRUE,
                                       switch_items);
        if (w > 0)
          {
             tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
             rects = eina_list_append(rects, tr);
             tr->x = ln->x + it1->x + x;
             tr->y = ln->par->y + ln->y;
             tr->h = ln->h;
             tr->w = w;
          }
     }
   else if (it1 != it2)
     {
        /* Get the middle items */
        Evas_Coord min_x, max_x;
        Evas_Coord x, w;
        it = _ITEM(EINA_INLIST_GET(it1)->next);
        min_x = max_x = it->x;

        if (it1->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
          {
             Evas_Coord y, h;
             Evas_Object_Textblock2_Text_Item *ti;
             int ret;
             ti = _ITEM_TEXT(it1);

             ret = ENFN->font_pen_coords_get(ENDT,
                   ti->parent.format->font.font,
                   &ti->text_props,
                   start,
                   &x, &y, &w, &h);
             if (!ret)
               {
                  /* BUG! Skip the first item */
                  x = w = 0;
               }
             else
               {
                  _evas_textblock2_range_calc_x_w(it1, &x, &w, EINA_TRUE,
                        switch_items);
               }
          }
        else
          {
             x = 0;
             w = it1->w;
             _evas_textblock2_range_calc_x_w(it1, &x, &w, EINA_TRUE,
                   switch_items);
          }
        if (w > 0)
          {
             tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
             rects = eina_list_append(rects, tr);
             tr->x = ln->x + it1->x + x;
             tr->y = ln->par->y + ln->y;
             tr->h = ln->h;
             tr->w = w;
          }

        while (it && (it != it2))
          {
             if (((it1->text_pos <= it->text_pos) && (it->text_pos <= it2->text_pos)) ||
                   ((it2->text_pos <= it->text_pos) && (it->text_pos <= it1->text_pos)))
               {
                  max_x = it->x + it->adv;
               }
             it = (Evas_Object_Textblock2_Item *) EINA_INLIST_GET(it)->next;
          }
        if (min_x != max_x)
          {
             tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
             rects = eina_list_append(rects, tr);
             tr->x = ln->x + min_x;
             tr->y = ln->par->y + ln->y;
             tr->h = ln->h;
             tr->w = max_x - min_x;
          }
        if (it2->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
          {
             Evas_Coord y, h;
             Evas_Object_Textblock2_Text_Item *ti;
             int ret;
             ti = _ITEM_TEXT(it2);

             ret = ENFN->font_pen_coords_get(ENDT,
                   ti->parent.format->font.font,
                   &ti->text_props,
                   end,
                   &x, &y, &w, &h);
             if (!ret)
               {
                  /* BUG! skip the last item */
                  x = w = 0;
               }
             else
               {
                  _evas_textblock2_range_calc_x_w(it2, &x, &w, EINA_FALSE,
                        switch_items);
               }
          }
        else
          {
             if (end > 0)
               {
                  x = it2->adv;
                  w = 0;
               }
             else
               {
                  x = 0;
                  w = it2->adv;
               }
             _evas_textblock2_range_calc_x_w(it2, &x, &w, EINA_FALSE,
                        switch_items);
          }
        if (w > 0)
          {
             tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
             rects = eina_list_append(rects, tr);
             tr->x = ln->x + it2->x + x;
             tr->y = ln->par->y + ln->y;
             tr->h = ln->h;
             tr->w = w;
          }
     }
   return rects;
}

EAPI Eina_Iterator *
evas_textblock2_cursor_range_simple_geometry_get(const Evas_Textblock2_Cursor *cur1, const Evas_Textblock2_Cursor *cur2)
{
   Evas_Object_Textblock2_Line *ln1, *ln2;
   Evas_Object_Textblock2_Item *it1, *it2;
   Eina_List *rects = NULL;
   Eina_Iterator *itr = NULL;

   if (!cur1 || !cur1->node) return NULL;
   if (!cur2 || !cur2->node) return NULL;
   if (cur1->obj != cur2->obj) return NULL;
   Evas_Textblock2_Data *o = eo_data_scope_get(cur1->obj, MY_CLASS);

   _relayout_if_needed(cur1->obj, o);

   if (evas_textblock2_cursor_compare(cur1, cur2) > 0)
     {
        const Evas_Textblock2_Cursor *tc;

        tc = cur1;
        cur1 = cur2;
        cur2 = tc;
     }

   ln1 = ln2 = NULL;
   it1 = it2 = NULL;
   _find_layout_item_match(cur1, &ln1, &it1);
   if (!ln1 || !it1) return NULL;
   _find_layout_item_match(cur2, &ln2, &it2);
   if (!ln2 || !it2) return NULL;

   if (ln1 == ln2)
     {
        rects = _evas_textblock2_cursor_range_in_line_geometry_get(ln1, cur1, cur2);
     }
   else
     {
        int lm = 0, rm = 0;
        Eina_List *rects2 = NULL;
        Evas_Coord w;
        Evas_Textblock2_Cursor *tc;
        Evas_Textblock2_Rectangle *tr;

        if (ln1->items)
          {
             Evas_Object_Textblock2_Format *fm = ln1->items->format;
             if (fm)
               {
                  lm = fm->margin.l;
                  rm = fm->margin.r;
               }
          }

        evas_object_geometry_get(cur1->obj, NULL, NULL, &w, NULL);
        rects = _evas_textblock2_cursor_range_in_line_geometry_get(ln1, cur1, NULL);

        /* Extend selection rectangle in first line */
        tc = evas_object_textblock2_cursor_new(cur1->obj);
        evas_textblock2_cursor_copy(cur1, tc);
        evas_textblock2_cursor_line_char_last(tc);
        tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
        evas_textblock2_cursor_pen_geometry_get(tc, &tr->x, &tr->y, &tr->w, &tr->h);
        if (ln1->par->direction == EVAS_BIDI_DIRECTION_RTL)
          {
             tr->w = tr->x + tr->w - rm;
             tr->x = lm;
          }
        else
          {
             tr->w = w - tr->x - rm;
          }
        rects = eina_list_append(rects, tr);
        evas_textblock2_cursor_free(tc);

        rects2 = _evas_textblock2_cursor_range_in_line_geometry_get(ln2, NULL, cur2);

        /* Add middle rect */
        if ((ln1->par->y + ln1->y + ln1->h) != (ln2->par->y + ln2->y))
          {
             tr = calloc(1, sizeof(Evas_Textblock2_Rectangle));
             tr->x = lm;
             tr->y = ln1->par->y + ln1->y + ln1->h;
             tr->w = w - tr->x - rm;
             tr->h = ln2->par->y + ln2->y - tr->y;
             rects = eina_list_append(rects, tr);
          }
        rects = eina_list_merge(rects, rects2);
     }
   itr = _evas_textblock2_selection_iterator_new(rects);

   return itr;
}

EAPI Eina_Bool
evas_textblock2_cursor_eol_get(const Evas_Textblock2_Cursor *cur)
{
   Eina_Bool ret = EINA_FALSE;
   Evas_Textblock2_Cursor cur2;
   if (!cur) return EINA_FALSE;

   cur2.obj = cur->obj;
   evas_textblock2_cursor_copy(cur, &cur2);
   evas_textblock2_cursor_line_char_last(&cur2);
   if (cur2.pos == cur->pos)
     {
        ret = EINA_TRUE;
     }
   return ret;
}

/* general controls */
EOLIAN static Eina_Bool
_evas_textblock2_line_number_geometry_get(Eo *eo_obj, Evas_Textblock2_Data *o, int line, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{

   Evas_Object_Textblock2_Line *ln;

   _relayout_if_needed(eo_obj, o);

   ln = _find_layout_line_num(eo_obj, line);
   if (!ln) return EINA_FALSE;
   if (cx) *cx = ln->x;
   if (cy) *cy = ln->par->y + ln->y;
   if (cw) *cw = ln->w;
   if (ch) *ch = ln->h;
   return EINA_TRUE;
}

static void
_evas_object_textblock2_clear_all(Evas_Object *eo_obj)
{
   eo_do(eo_obj, evas_obj_textblock2_clear());
}

EOLIAN static void
_evas_textblock2_clear(Eo *eo_obj, Evas_Textblock2_Data *o)
{
   Eina_List *l;
   Evas_Textblock2_Cursor *cur;

   if (o->paragraphs)
     {
	_paragraphs_free(eo_obj, o->paragraphs);
	o->paragraphs = NULL;
     }

   _nodes_clear(eo_obj);
   o->cursor->node = NULL;
   o->cursor->pos = 0;
   EINA_LIST_FOREACH(o->cursors, l, cur)
     {
	cur->node = NULL;
	cur->pos = 0;

     }

   _evas_textblock2_changed(o, eo_obj);
}

EAPI void
evas_object_textblock2_clear(Evas_Object *eo_obj)
{
   TB_HEAD();
   _evas_object_textblock2_clear_all(eo_obj);

   /* Force recreation of everything for textblock2.
    * FIXME: We have the same thing in other places, merge it... */
   evas_textblock2_cursor_paragraph_first(o->cursor);
   evas_textblock2_cursor_text_prepend(o->cursor, "");
}

EOLIAN static void
_evas_textblock2_size_formatted_get(Eo *eo_obj, Evas_Textblock2_Data *o, Evas_Coord *w, Evas_Coord *h)
{
   _relayout_if_needed(eo_obj, o);

   if (w) *w = o->formatted.w;
   if (h) *h = o->formatted.h;
}

EOLIAN static void
_evas_textblock2_style_insets_get(Eo *eo_obj, Evas_Textblock2_Data *o, Evas_Coord *l, Evas_Coord *r, Evas_Coord *t, Evas_Coord *b)
{
   _relayout_if_needed(eo_obj, o);

   if (l) *l = o->style_pad.l;
   if (r) *r = o->style_pad.r;
   if (t) *t = o->style_pad.t;
   if (b) *b = o->style_pad.b;
}

EOLIAN static void
_evas_textblock2_eo_base_dbg_info_get(Eo *eo_obj, Evas_Textblock2_Data *o EINA_UNUSED, Eo_Dbg_Info *root)
{
   eo_do_super(eo_obj, MY_CLASS, eo_dbg_info_get(root));
   if (!root) return;
   Eo_Dbg_Info *group = EO_DBG_INFO_LIST_APPEND(root, MY_CLASS_NAME);
   Eo_Dbg_Info *node;

   const char *style;
   const char *text = NULL;
   char shorttext[48];
   const Evas_Textblock2_Style *ts = NULL;

   eo_do(eo_obj, ts = evas_obj_textblock2_style_get());
   style = evas_textblock2_style_get(ts);
   eo_do(eo_obj, text = efl_text_get());
   strncpy(shorttext, text, 38);
   if (shorttext[37])
     strcpy(shorttext + 37, "\xe2\x80\xa6"); /* HORIZONTAL ELLIPSIS */

   EO_DBG_INFO_APPEND(group, "Style", EINA_VALUE_TYPE_STRING, style);
   EO_DBG_INFO_APPEND(group, "Text", EINA_VALUE_TYPE_STRING, shorttext);

     {
        int w, h;
        eo_do(eo_obj, evas_obj_textblock2_size_formatted_get(&w, &h));
        node = EO_DBG_INFO_LIST_APPEND(group, "Formatted size");
        EO_DBG_INFO_APPEND(node, "w", EINA_VALUE_TYPE_INT, w);
        EO_DBG_INFO_APPEND(node, "h", EINA_VALUE_TYPE_INT, h);
     }
}

/* all nice and private */
static void
evas_object_textblock2_init(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Textblock2_Data *o;
   static Eina_Bool linebreak_init = EINA_FALSE;

   if (!linebreak_init)
     {
        linebreak_init = EINA_TRUE;
        init_linebreak();
        init_wordbreak();
     }

   o = obj->private_data;
   o->cursor->obj = eo_obj;
   eo_do(eo_obj, efl_text_set(""));
}

EOLIAN static void
_evas_textblock2_eo_base_destructor(Eo *eo_obj, Evas_Textblock2_Data *o EINA_UNUSED)
{
   evas_object_textblock2_free(eo_obj);
   eo_do_super(eo_obj, MY_CLASS, eo_destructor());
}

static void
evas_object_textblock2_free(Evas_Object *eo_obj)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

   _evas_object_textblock2_clear_all(eo_obj);
   evas_object_textblock2_style_set(eo_obj, NULL);
   while (evas_object_textblock2_style_user_peek(eo_obj))
     {
        evas_object_textblock2_style_user_pop(eo_obj);
     }
   free(o->cursor);
   while (o->cursors)
     {
	Evas_Textblock2_Cursor *cur;

	cur = (Evas_Textblock2_Cursor *)o->cursors->data;
	o->cursors = eina_list_remove_list(o->cursors, o->cursors);
	free(cur);
     }
   if (o->ellip_ti) _item_free(eo_obj, NULL, _ITEM(o->ellip_ti));
   o->magic = 0;
  _format_command_shutdown();
}


static void
evas_object_textblock2_render(Evas_Object *eo_obj EINA_UNUSED,
			     Evas_Object_Protected_Data *obj,
			     void *type_private_data,
			     void *output, void *context, void *surface,
			     int x, int y, Eina_Bool do_async)
{
   Evas_Object_Textblock2_Paragraph *par, *start = NULL;
   Evas_Object_Textblock2_Item *itr;
   Evas_Object_Textblock2_Line *ln;
   Evas_Textblock2_Data *o = type_private_data;
   Eina_List *shadows = NULL;
   Eina_List *glows = NULL;
   Eina_List *outlines = NULL;
   int i, j;
   int cx, cy, cw, ch, clip;
   int ca, cr, cg, cb;
   int na, nr, ng, nb;
   const char vals[5][5] =
     {
	  {0, 1, 2, 1, 0},
	  {1, 3, 4, 3, 1},
	  {2, 4, 5, 4, 2},
	  {1, 3, 4, 3, 1},
	  {0, 1, 2, 1, 0}
     };

   /* render object to surface with context, and offxet by x,y */
   obj->layer->evas->engine.func->context_multiplier_unset(output,
							   context);
   ENFN->context_render_op_set(output, context, obj->cur->render_op);
   /* FIXME: This clipping is just until we fix inset handling correctly. */
   ENFN->context_clip_clip(output, context,
                              obj->cur->geometry.x + x,
                              obj->cur->geometry.y + y,
                              obj->cur->geometry.w,
                              obj->cur->geometry.h);
   clip = ENFN->context_clip_get(output, context, &cx, &cy, &cw, &ch);
   /* If there are no paragraphs and thus there are no lines,
    * there's nothing left to do. */
   if (!o->paragraphs) return;

   ENFN->context_color_set(output, context, 0, 0, 0, 0);
   ca = cr = cg = cb = 0;

#define ITEM_WALK() \
   EINA_INLIST_FOREACH(start, par) \
     { \
        if (!par->visible) continue; \
        if (clip) \
          { \
             if ((obj->cur->geometry.y + y + par->y + par->h) < (cy - 20)) \
             continue; \
             if ((obj->cur->geometry.y + y + par->y) > (cy + ch + 20)) \
             break; \
          } \
        EINA_INLIST_FOREACH(par->lines, ln) \
          { \
             if (clip) \
               { \
                  if ((obj->cur->geometry.y + y + par->y + ln->y + ln->h) < (cy - 20)) \
                  continue; \
                  if ((obj->cur->geometry.y + y + par->y + ln->y) > (cy + ch + 20)) \
                  break; \
               } \
             EINA_INLIST_FOREACH(ln->items, itr) \
               { \
                  Evas_Coord yoff; \
                  yoff = ln->baseline; \
                  if (itr->format->valign != -1.0) \
                    { \
                       if (itr->type == EVAS_TEXTBLOCK2_ITEM_TEXT) \
                         { \
                            Evas_Object_Textblock2_Text_Item *titr = \
                              (Evas_Object_Textblock2_Text_Item *)itr; \
                            int ascent = 0; \
                            if (titr->text_props.font_instance) \
                              ascent = evas_common_font_instance_max_ascent_get(titr->text_props.font_instance); \
                            yoff = ascent + \
                              (itr->format->valign * (ln->h - itr->h)); \
                         } \
                       else yoff = itr->format->valign * (ln->h - itr->h); \
                    } \
                  itr->yoff = yoff;             \
                  if (clip) \
                    { \
                       if ((obj->cur->geometry.x + x + ln->x + itr->x + itr->w) < (cx - 20)) \
                       continue; \
                       if ((obj->cur->geometry.x + x + ln->x + itr->x) > (cx + cw + 20)) \
                       break; \
                    } \
                  if ((ln->x + itr->x + itr->w) <= 0) continue; \
                  if (ln->x + itr->x > obj->cur->geometry.w) break; \
                  do

#define ITEM_WALK_END() \
                  while (0); \
               } \
          } \
     } \
   do {} while(0)
#define COLOR_SET(col)                                                  \
   nr = obj->cur->cache.clip.r * ti->parent.format->color.col.r;        \
   ng = obj->cur->cache.clip.g * ti->parent.format->color.col.g;        \
   nb = obj->cur->cache.clip.b * ti->parent.format->color.col.b;        \
   na = obj->cur->cache.clip.a * ti->parent.format->color.col.a;        \
   if (na != ca || nb != cb || ng != cg || nr != cr)                    \
     {                                                                  \
        ENFN->context_color_set(output, context,                        \
                                nr / 255, ng / 255, nb / 255, na / 255); \
        cr = nr; cg = ng; cb = nb; ca = na;                             \
     }
#define COLOR_SET_AMUL(col, amul)                                       \
   nr = obj->cur->cache.clip.r * ti->parent.format->color.col.r * (amul); \
   ng = obj->cur->cache.clip.g * ti->parent.format->color.col.g * (amul); \
   nb = obj->cur->cache.clip.b * ti->parent.format->color.col.b * (amul); \
   na = obj->cur->cache.clip.a * ti->parent.format->color.col.a * (amul); \
   if (na != ca || nb != cb || ng != cg || nr != cr)                    \
     {                                                                  \
        ENFN->context_color_set(output, context,                        \
                                nr / 65025, ng / 65025, nb / 65025, na / 65025); \
        cr = nr; cg = ng; cb = nb; ca = na;                             \
     }
#define DRAW_TEXT(ox, oy)                                               \
   if (ti->parent.format->font.font)                                    \
     evas_font_draw_async_check(obj, output, context, surface,          \
        ti->parent.format->font.font,                                   \
        obj->cur->geometry.x + ln->x + ti->parent.x + x + (ox),          \
        obj->cur->geometry.y + ln->par->y + ln->y + yoff + y + (oy),     \
        ti->parent.w, ti->parent.h, ti->parent.w, ti->parent.h,         \
        &ti->text_props, do_async);

   /* backing */
#define DRAW_RECT(ox, oy, ow, oh, or, og, ob, oa)                       \
   do                                                                   \
     {                                                                  \
        nr = obj->cur->cache.clip.r * or;                               \
        ng = obj->cur->cache.clip.g * og;                               \
        nb = obj->cur->cache.clip.b * ob;                               \
        na = obj->cur->cache.clip.a * oa;                               \
        if (na != ca || nb != cb || ng != cg || nr != cr)               \
          {                                                             \
             ENFN->context_color_set(output, context,                   \
                                     nr / 255, ng / 255, nb / 255, na / 255); \
             cr = nr; cg = ng; cb = nb; ca = na;                        \
          }                                                             \
        ENFN->rectangle_draw(output,                                    \
                             context,                                   \
                             surface,                                   \
                             obj->cur->geometry.x + ln->x + x + (ox),   \
                             obj->cur->geometry.y + ln->par->y + ln->y + y + (oy), \
                             (ow),                                      \
                             (oh),                                      \
                             do_async);                                 \
     }                                                                  \
   while (0)

#define DRAW_FORMAT_DASHED(oname, oy, oh, dw, dp) \
   do \
     { \
        if (itr->format->oname) \
          { \
             unsigned char _or, _og, _ob, _oa; \
             int _ind, _dx = 0, _dn, _dr; \
             _or = itr->format->color.oname.r; \
             _og = itr->format->color.oname.g; \
             _ob = itr->format->color.oname.b; \
             _oa = itr->format->color.oname.a; \
             if (!EINA_INLIST_GET(itr)->next) \
               { \
                  _dn = itr->w / (dw + dp); \
                  _dr = itr->w % (dw + dp); \
               } \
             else \
               { \
                  _dn = itr->adv / (dw + dp); \
                  _dr = itr->adv % (dw + dp); \
               } \
             if (_dr > dw) _dr = dw; \
             for (_ind = 0 ; _ind < _dn ; _ind++) \
               { \
                  DRAW_RECT(itr->x + _dx, oy, dw, oh, _or, _og, _ob, _oa); \
                  _dx += dw + dp; \
               } \
             DRAW_RECT(itr->x + _dx, oy, _dr, oh, _or, _og, _ob, _oa); \
          } \
     } \
   while (0)

#define DRAW_FORMAT(oname, oy, oh) \
   do \
     { \
        if (itr->format->oname) \
          { \
             unsigned char _or, _og, _ob, _oa; \
             _or = itr->format->color.oname.r; \
             _og = itr->format->color.oname.g; \
             _ob = itr->format->color.oname.b; \
             _oa = itr->format->color.oname.a; \
             DRAW_RECT(itr->x, oy, itr->adv, oh, _or, _og, _ob, _oa); \
          } \
     } \
   while (0)

     {
        Evas_Coord look_for_y = 0 - (obj->cur->geometry.y + y);
        if (clip)
          {
             Evas_Coord tmp_lfy = cy - (obj->cur->geometry.y + y);
             if (tmp_lfy > look_for_y)
                look_for_y = tmp_lfy;
          }

        if (look_for_y >= 0)
           start = _layout_find_paragraph_by_y(o, look_for_y);

        if (!start)
           start = o->paragraphs;
     }

   ITEM_WALK()
     {
        /* Check which other pass are necessary to avoid useless WALK */
        Evas_Object_Textblock2_Text_Item *ti;

        ti = (itr->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ? _ITEM_TEXT(itr) : NULL;
        if (ti)
          {
             if (ti->parent.format->style & (EVAS_TEXT_STYLE_SHADOW |
                                             EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW |
                                             EVAS_TEXT_STYLE_OUTLINE_SHADOW |
                                             EVAS_TEXT_STYLE_FAR_SHADOW |
                                             EVAS_TEXT_STYLE_FAR_SOFT_SHADOW |
                                             EVAS_TEXT_STYLE_SOFT_SHADOW))
               {
                  shadows = eina_list_append(shadows, itr);
               }
             if ((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) ==
                 EVAS_TEXT_STYLE_GLOW)
               {
                  glows = eina_list_append(glows, itr);
               }
             if (((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_OUTLINE) ||
                 ((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_OUTLINE_SHADOW) ||
                 ((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW) ||
                 (ti->parent.format->style == EVAS_TEXT_STYLE_SOFT_OUTLINE))
               {
                  outlines = eina_list_append(outlines, itr);
               }
          }

        /* Draw background */
        DRAW_FORMAT(backing, 0, ln->h);
     }
   ITEM_WALK_END();

   /* There are size adjustments that depend on the styles drawn here back
    * in "_text_item_update_sizes" should not modify one without the other. */

   /* prepare everything for text draw */

   /* shadows */
   EINA_LIST_FREE(shadows, itr)
     {
        int shad_dst, shad_sz, dx, dy, haveshad;
        Evas_Object_Textblock2_Text_Item *ti;
        Evas_Coord yoff;

        ti = (itr->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ? _ITEM_TEXT(itr) : NULL;
        if (!ti) continue;

        yoff = itr->yoff;
        ln = itr->ln;

        shad_dst = shad_sz = dx = dy = haveshad = 0;
        switch (ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC)
          {
           case EVAS_TEXT_STYLE_SHADOW:
              shad_dst = 1;
              haveshad = 1;
              break;
           case EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW:
              shad_dst = 1;
              shad_sz = 2;
              haveshad = 1;
              break;
           case EVAS_TEXT_STYLE_OUTLINE_SHADOW:
           case EVAS_TEXT_STYLE_FAR_SHADOW:
              shad_dst = 2;
              haveshad = 1;
              break;
           case EVAS_TEXT_STYLE_FAR_SOFT_SHADOW:
              shad_dst = 2;
              shad_sz = 2;
              haveshad = 1;
              break;
           case EVAS_TEXT_STYLE_SOFT_SHADOW:
              shad_dst = 1;
              shad_sz = 2;
              haveshad = 1;
              break;
           default:
              break;
          }
        if (haveshad)
          {
             if (shad_dst > 0)
               {
                  switch (ti->parent.format->style & EVAS_TEXT_STYLE_MASK_SHADOW_DIRECTION)
                    {
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT:
                        dx = 1;
                        dy = 1;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM:
                        dx = 0;
                        dy = 1;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_LEFT:
                        dx = -1;
                        dy = 1;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_LEFT:
                        dx = -1;
                        dy = 0;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_LEFT:
                        dx = -1;
                        dy = -1;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP:
                        dx = 0;
                        dy = -1;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_RIGHT:
                        dx = 1;
                        dy = -1;
                        break;
                     case EVAS_TEXT_STYLE_SHADOW_DIRECTION_RIGHT:
                        dx = 1;
                        dy = 0;
                     default:
                        break;
                    }
                  dx *= shad_dst;
                  dy *= shad_dst;
               }
             switch (shad_sz)
               {
                case 0:
                   COLOR_SET(shadow);
                   DRAW_TEXT(dx, dy);
                   break;
                case 2:
                   for (j = 0; j < 5; j++)
                     {
                        for (i = 0; i < 5; i++)
                          {
                             if (vals[i][j] != 0)
                               {
                                  COLOR_SET_AMUL(shadow, vals[i][j] * 50);
                                  DRAW_TEXT(i - 2 + dx, j - 2 + dy);
                               }
                          }
                     }
                   break;
                default:
                   break;
               }
          }
     }

   /* glows */
   EINA_LIST_FREE(glows, itr)
     {
        Evas_Object_Textblock2_Text_Item *ti;
        Evas_Coord yoff;

        ti = (itr->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ? _ITEM_TEXT(itr) : NULL;
        if (!ti) continue;

        yoff = itr->yoff;
        ln = itr->ln;

        if ((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_GLOW)
          {
             for (j = 0; j < 5; j++)
               {
                  for (i = 0; i < 5; i++)
                    {
                       if (vals[i][j] != 0)
                         {
                            COLOR_SET_AMUL(glow, vals[i][j] * 50);
                            DRAW_TEXT(i - 2, j - 2);
                         }
                    }
               }
             COLOR_SET(glow2);
             DRAW_TEXT(-1, 0);
             DRAW_TEXT(1, 0);
             DRAW_TEXT(0, -1);
             DRAW_TEXT(0, 1);
          }
     }

   /* outlines */
   EINA_LIST_FREE(outlines, itr)
     {
        Evas_Object_Textblock2_Text_Item *ti;
        Evas_Coord yoff;

        ti = (itr->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ? _ITEM_TEXT(itr) : NULL;
        if (!ti) continue;

        yoff = itr->yoff;
        ln = itr->ln;

        if (((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_OUTLINE) ||
            ((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_OUTLINE_SHADOW) ||
            ((ti->parent.format->style & EVAS_TEXT_STYLE_MASK_BASIC) == EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW))
          {
             COLOR_SET(outline);
             DRAW_TEXT(-1, 0);
             DRAW_TEXT(1, 0);
             DRAW_TEXT(0, -1);
             DRAW_TEXT(0, 1);
          }
        else if (ti->parent.format->style == EVAS_TEXT_STYLE_SOFT_OUTLINE)
          {
             for (j = 0; j < 5; j++)
               {
                  for (i = 0; i < 5; i++)
                    {
                       if (((i != 2) || (j != 2)) && (vals[i][j] != 0))
                         {
                            COLOR_SET_AMUL(outline, vals[i][j] * 50);
                            DRAW_TEXT(i - 2, j - 2);
                         }
                    }
               }
          }
     }

   /* normal text and lines */
   /* Get the thickness and position, and save them for non-text items. */
   int line_thickness =
           evas_common_font_instance_underline_thickness_get(NULL);
   int line_position =
           evas_common_font_instance_underline_position_get(NULL);
   ITEM_WALK()
     {
        Evas_Object_Textblock2_Text_Item *ti;
        ti = (itr->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ? _ITEM_TEXT(itr) : NULL;
        /* NORMAL TEXT */
        if (ti)
          {
             void *fi = _ITEM_TEXT(itr)->text_props.font_instance;
             COLOR_SET(normal);
             DRAW_TEXT(0, 0);
             line_thickness =
                evas_common_font_instance_underline_thickness_get(fi);
             line_position =
                evas_common_font_instance_underline_position_get(fi);
          }

        /* STRIKETHROUGH */
        DRAW_FORMAT(strikethrough, (ln->h / 2), line_thickness);

        /* UNDERLINE */
        DRAW_FORMAT(underline, ln->baseline + line_position, line_thickness);

        /* UNDERLINE DASHED */
        DRAW_FORMAT_DASHED(underline_dash, ln->baseline + line_position,
                         line_thickness,
                         itr->format->underline_dash_width,
                         itr->format->underline_dash_gap);

        /* UNDERLINE2 */
        DRAW_FORMAT(underline2, ln->baseline + line_position + line_thickness +
              line_position, line_thickness);
     }
   ITEM_WALK_END();
}

static void
evas_object_textblock2_coords_recalc(Evas_Object *eo_obj EINA_UNUSED,
                                    Evas_Object_Protected_Data *obj,
                                    void *type_private_data)
{
   Evas_Textblock2_Data *o = type_private_data;

   if (
       // width changed thus we may have to re-wrap or change centering etc.
       (obj->cur->geometry.w != o->last_w) ||
       // if valign not top OR we have ellipsis, then if height changed we need to re-eval valign or ... spot
       (((o->valign != 0.0) || (o->have_ellipsis)) &&
           (
               ((o->formatted.oneline_h == 0) &&
                   (obj->cur->geometry.h != o->last_h)) ||
               ((o->formatted.oneline_h != 0) &&
                   (((obj->cur->geometry.h != o->last_h) &&
                     (o->formatted.oneline_h < obj->cur->geometry.h))))
           )
       ) ||
       // obviously if content text changed we need to reformat it
       (o->content_changed) ||
       // if format changed (eg styles) we need to re-format/match tags etc.
       (o->format_changed)
      )
     {
        LYDBG("ZZ: invalidate 2 %p ## %i != %i || %3.3f || %i && %i != %i | %i %i\n", eo_obj, obj->cur->geometry.w, o->last_w, o->valign, o->have_ellipsis, obj->cur->geometry.h, o->last_h, o->content_changed, o->format_changed);
	o->formatted.valid = 0;
	o->changed = 1;
     }
}

static void
evas_object_textblock2_render_pre(Evas_Object *eo_obj,
				 Evas_Object_Protected_Data *obj,
				 void *type_private_data)
{
   Evas_Textblock2_Data *o = type_private_data;
   int is_v, was_v;

   /* dont pre-render the obj twice! */
   if (obj->pre_render_done) return;
   obj->pre_render_done = EINA_TRUE;

   /* pre-render phase. this does anything an object needs to do just before */
   /* rendering. this could mean loading the image data, retrieving it from */
   /* elsewhere, decoding video etc. */
   /* then when this is done the object needs to figure if it changed and */
   /* if so what and where and add the appropriate redraw textblock2s */

   evas_object_textblock2_coords_recalc(eo_obj, obj, obj->private_data);
   if (o->changed)
     {
        LYDBG("ZZ: relayout 16\n");
        _relayout(eo_obj);
        o->redraw = 0;
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        is_v = evas_object_is_visible(eo_obj, obj);
        was_v = evas_object_was_visible(eo_obj, obj);
        goto done;
     }

   if (o->redraw)
     {
        o->redraw = 0;
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        is_v = evas_object_is_visible(eo_obj, obj);
        was_v = evas_object_was_visible(eo_obj, obj);
        goto done;
     }
   /* if someone is clipping this obj - go calculate the clipper */
   if (obj->cur->clipper)
     {
        if (obj->cur->cache.clip.dirty)
          evas_object_clip_recalc(obj->cur->clipper);
        obj->cur->clipper->func->render_pre(obj->cur->clipper->object,
                                            obj->cur->clipper,
                                            obj->cur->clipper->private_data);
     }
   /* now figure what changed and add draw rects */
   /* if it just became visible or invisible */
   is_v = evas_object_is_visible(eo_obj, obj);
   was_v = evas_object_was_visible(eo_obj, obj);
   if (is_v != was_v)
     {
        evas_object_render_pre_visible_change(&obj->layer->evas->clip_changes,
                                              eo_obj, is_v, was_v);
        goto done;
     }
   if (obj->changed_map || obj->changed_src_visible)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        goto done;
     }
   /* it's not visible - we accounted for it appearing or not so just abort */
   if (!is_v) goto done;
   /* clipper changed this is in addition to anything else for obj */
   evas_object_render_pre_clipper_change(&obj->layer->evas->clip_changes,
                                         eo_obj);
   /* if we restacked (layer or just within a layer) and don't clip anyone */
   if (obj->restack)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        goto done;
     }
   /* if it changed color */
   if ((obj->cur->color.r != obj->prev->color.r) ||
       (obj->cur->color.g != obj->prev->color.g) ||
       (obj->cur->color.b != obj->prev->color.b) ||
       (obj->cur->color.a != obj->prev->color.a))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        goto done;
     }
   /* if it changed geometry - and obviously not visibility or color */
   /* calculate differences since we have a constant color fill */
   /* we really only need to update the differences */
   if ((obj->cur->geometry.x != obj->prev->geometry.x) ||
       (obj->cur->geometry.y != obj->prev->geometry.y) ||
       (obj->cur->geometry.w != obj->prev->geometry.w) ||
       (obj->cur->geometry.h != obj->prev->geometry.h))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        goto done;
     }
   if (obj->cur->render_op != obj->prev->render_op)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        goto done;
     }
done:
   evas_object_render_pre_effect_updates(&obj->layer->evas->clip_changes,
                                         eo_obj, is_v, was_v);
}

static void
evas_object_textblock2_render_post(Evas_Object *eo_obj,
                                  Evas_Object_Protected_Data *obj EINA_UNUSED,
                                  void *type_private_data EINA_UNUSED)
{
   /*   Evas_Textblock2_Data *o; */

   /* this moves the current data to the previous state parts of the object */
   /* in whatever way is safest for the object. also if we don't need object */
   /* data anymore we can free it if the object deems this is a good idea */
/*   o = (Evas_Textblock2_Data *)(obj->object_data); */
   /* remove those pesky changes */
   evas_object_clip_changes_clean(eo_obj);
   /* move cur to prev safely for object data */
   evas_object_cur_prev(eo_obj);
/*   o->prev = o->cur; */
}

static unsigned int evas_object_textblock2_id_get(Evas_Object *eo_obj)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (!o) return 0;
   return MAGIC_OBJ_TEXTBLOCK;
}

static unsigned int evas_object_textblock2_visual_id_get(Evas_Object *eo_obj)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (!o) return 0;
   return MAGIC_OBJ_CUSTOM;
}

static void *evas_object_textblock2_engine_data_get(Evas_Object *eo_obj)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (!o) return NULL;
   return o->engine_data;
}

static int
evas_object_textblock2_is_opaque(Evas_Object *eo_obj EINA_UNUSED,
                                Evas_Object_Protected_Data *obj EINA_UNUSED,
                                void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object is */
   /* currently fulyl opque over the entire gradient it occupies */
   return 0;
}

static int
evas_object_textblock2_was_opaque(Evas_Object *eo_obj EINA_UNUSED,
                                 Evas_Object_Protected_Data *obj EINA_UNUSED,
                                 void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object was */
   /* currently fulyl opque over the entire gradient it occupies */
   return 0;
}

static void
evas_object_textblock2_scale_update(Evas_Object *eo_obj EINA_UNUSED,
                                   Evas_Object_Protected_Data *obj EINA_UNUSED,
                                   void *type_private_data)
{
   Evas_Textblock2_Data *o = type_private_data;
   _evas_textblock2_invalidate_all(o);
   _evas_textblock2_changed(o, eo_obj);
   o->last_w = -1;
   o->last_h = -1;
}

void
_evas_object_textblock2_rehint(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   Evas_Object_Textblock2_Paragraph *par;
   Evas_Object_Textblock2_Line *ln;

   EINA_INLIST_FOREACH(o->paragraphs, par)
     {
        EINA_INLIST_FOREACH(par->lines, ln)
          {
             Evas_Object_Textblock2_Item *it;

             EINA_INLIST_FOREACH(ln->items, it)
               {
                  if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
                    {
                       Evas_Object_Textblock2_Text_Item *ti = _ITEM_TEXT(it);
                       if (ti->parent.format->font.font)
                         {
                            evas_font_load_hinting_set(obj->layer->evas->evas,
                                  ti->parent.format->font.font,
                                  obj->layer->evas->hinting);
                         }
                    }
               }
          }
     }
   _evas_textblock2_invalidate_all(o);
   _evas_textblock2_changed(o, eo_obj);
}

/**
 * @}
 */

#ifdef HAVE_TESTS
/* return EINA_FALSE on error, used in unit_testing */
EAPI Eina_Bool
_evas_textblock2_check_item_node_link(Evas_Object *eo_obj)
{
   Evas_Textblock2_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   Evas_Object_Textblock2_Paragraph *par;
   Evas_Object_Textblock2_Line *ln;
   Evas_Object_Textblock2_Item *it;

   if (!o) return EINA_FALSE;

   _relayout_if_needed(eo_obj, o);

   EINA_INLIST_FOREACH(o->paragraphs, par)
     {
        EINA_INLIST_FOREACH(par->lines, ln)
          {
             EINA_INLIST_FOREACH(ln->items, it)
               {
                  if (it->text_node != par->text_node)
                     return EINA_FALSE;
               }
          }
     }
   return EINA_TRUE;
}

#endif

#if 0
/* Good for debugging */

EAPI void
ptnode(Evas_Object_Textblock2_Node_Text *n)
{
   printf("Text Node: %p\n", n);
   printf("next = %p, prev = %p, last = %p\n", EINA_INLIST_GET(n)->next, EINA_INLIST_GET(n)->prev, EINA_INLIST_GET(n)->last);
   printf("format_node = %p\n", n->format_node);
   printf("'%ls'\n", eina_ustrbuf_string_get(n->unicode));
}

EAPI void
pitem(Evas_Object_Textblock2_Item *it)
{
   Evas_Object_Textblock2_Text_Item *ti;
   Evas_Object_Textblock2_Format_Item *fi;
   printf("Item: %p %s\n", it, (it->visually_deleted) ? "(visually deleted)" : "");
   printf("Type: %s (%d)\n", (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT) ?
         "TEXT" : "FORMAT", it->type);
   printf("Text pos: %u Visual pos: %u\n", (unsigned int) it->text_pos, (unsigned int)
#ifdef BIDI_SUPPORT
         it->visual_pos
#else
         it->text_pos
#endif
         );
   printf("Coords: x = %d w = %d adv = %d\n", (int) it->x, (int) it->w,
         (int) it->adv);
   if (it->type == EVAS_TEXTBLOCK2_ITEM_TEXT)
     {
        Eina_Unicode *tmp;
        ti = _ITEM_TEXT(it);
        tmp = eina_unicode_strdup(GET_ITEM_TEXT(ti));
        tmp[ti->text_props.text_len] = '\0';
        printf("Text: '%ls'\n", tmp);
        free(tmp);
     }
   else
     {
        fi = _ITEM_FORMAT(it);
        printf("Format: '%s'\n", fi->item);
     }
}

EAPI void
ppar(Evas_Object_Textblock2_Paragraph *par)
{
   Evas_Object_Textblock2_Item *it;
   Eina_List *i;
   EINA_LIST_FOREACH(par->logical_items, i, it)
     {
        printf("***********************\n");
        pitem(it);
     }
}

#endif

#include "canvas/evas_textblock2.eo.c"
