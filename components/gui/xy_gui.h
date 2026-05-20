/**
 * @file xy_gui.h
 * @brief Bare-metal hierarchical menu / list UI component.
 *
 * Ported from Kohler JV545 UI_V3 + T_Msg.  Decouples the UI from any
 * specific message pump — the application feeds events via xy_gui_post()
 * and calls xy_gui_run() each main-loop iteration.
 *
 * ── Quick start ──────────────────────────────────────────────────────
 *
 *   // 1. Provide a string table function:
 *   static const uint8_t *my_str(uint8_t id) { return g_strings[id]; }
 *   xy_gui_set_string_fn(my_str);
 *
 *   // 2. Provide a font and init GDI:
 *   xy_gdi_set_font(g_font14, g_font12);
 *   xy_gdi_init();
 *   xy_lcd_on_off(true);
 *
 *   // 3. Declare a menu (in a .c file):
 *   XY_GUI_BEGIN_LIST(g_menu_settings)
 *       XY_GUI_ITEM(STR_BRIGHTNESS,  &g_menu_brightness),
 *       XY_GUI_ITEM(STR_LANGUAGE,    &g_menu_language),
 *   XY_GUI_END_LIST(g_menu_settings, NULL, NULL, &g_menu_root, 0)
 *
 *   // 4. Init and enter first screen:
 *   xy_gui_init();
 *   xy_gui_switch(&g_menu_root);
 *
 *   // 5. Main loop:
 *   while (1) {
 *       xy_gui_post_key(read_hw_key());   // from BSP
 *       xy_gui_run();
 *   }
 *
 * ── List display ─────────────────────────────────────────────────────
 *
 * The default list proc (xy_gui_proc_list) shows two items at a time,
 * one per half of the display:
 *
 *   ┌──────────────────────────────┐
 *   │  Item A  [state indicator]   │  ← first
 *   ├──────────────────────────────┤
 *   │▌ Item B  [state indicator]   │  ← first+1  ← selected (inverted)
 *   └──────────────────────────────┘
 *
 * UP/DOWN cycle through items (wraps around).
 * OK navigates to the item's pNext_on_ok list (or pNext if items==NULL).
 * BACK returns to pBack.
 *
 * ── String IDs ───────────────────────────────────────────────────────
 *
 * String IDs 0x00–0x0F are reserved.  Application IDs start at
 * XY_GUI_STR_USER (0x10).
 */

#ifndef XY_GUI_H
#define XY_GUI_H

#include "xy_gdi.h"   /* xy_dc_t, xy_gdi_* */
#include "xy_typedef.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Event types ────────────────────────────────────────────────────── */

typedef uint8_t xy_gui_ev_t;

#define XY_GUI_EV_NONE   0x00u
#define XY_GUI_EV_INIT   0x01u   /* screen entered          */
#define XY_GUI_EV_EXIT   0x02u   /* screen leaving          */
#define XY_GUI_EV_KEY    0x04u   /* key press; see key field */
#define XY_GUI_EV_TICK   0x05u   /* periodic 1-second tick  */
#define XY_GUI_EV_TIMER  0x06u   /* general timer event     */

/* ── Key codes ──────────────────────────────────────────────────────── */

#define XY_GUI_KEY_UP    0x01u
#define XY_GUI_KEY_DOWN  0x02u
#define XY_GUI_KEY_OK    0x03u
#define XY_GUI_KEY_BACK  0x04u
#define XY_GUI_KEY_MENU  0x05u   /* optional menu/home key  */

/* ── String IDs ─────────────────────────────────────────────────────── */

#define XY_GUI_STR_USER  0x10u   /* application IDs start here */

/* ── Message ────────────────────────────────────────────────────────── */

typedef struct {
    xy_gui_ev_t  ev;
    uint8_t      key;    /* XY_GUI_KEY_* when ev == XY_GUI_EV_KEY */
    void        *data;   /* optional payload                      */
} xy_gui_msg_t;

/* ── Screen handler type ─────────────────────────────────────────────── */

typedef bool (*xy_gui_proc_t)(xy_gui_msg_t *msg);

/* ── List item flags ────────────────────────────────────────────────── */

#define XY_GUI_F_INFO    (1u << 0u)  /* info text (no selection)           */
#define XY_GUI_F_RADIO   (1u << 1u)  /* radio group: get/set give 0/1      */
#define XY_GUI_F_CHECK   (1u << 2u)  /* checkbox: independent get/set each */
#define XY_GUI_F_NOWRAP  (1u << 3u)  /* stop at ends instead of wrapping   */

/* ── Callback types ─────────────────────────────────────────────────── */

/** Read state of item i.  0 = unselected, 1 = selected/checked. */
typedef uint8_t (*xy_gui_get_fn)(uint8_t i);

/** Write state of item i.  Return 1 to accept / navigate, 0 to stay. */
typedef uint8_t (*xy_gui_set_fn)(uint8_t i);

/* ── Data structures ─────────────────────────────────────────────────── */

typedef struct xy_gui_list xy_gui_list_t;

typedef struct {
    xy_gui_list_t *next_on_ok;  /* navigate here when this item is selected */
    uint8_t        str_id;       /* string ID for the item label             */
} xy_gui_item_t;

typedef struct {
    int8_t  now;    /* currently highlighted item index */
    int8_t  first;  /* first visible item index         */
} xy_gui_list_state_t;

struct xy_gui_list {
    xy_gui_proc_t             proc;    /* message handler (xy_gui_proc_list) */
    xy_gui_get_fn             get;     /* state getter; NULL = no indicator  */
    xy_gui_set_fn             set;     /* state setter; NULL = always accept */
    int8_t                    count;   /* number of items in items[]         */
    const xy_gui_item_t      *items;   /* ROM array (NULL for single-target) */
    xy_gui_list_t            *back;    /* BACK key target (NULL = root)      */
    xy_gui_list_t            *next;    /* OK target when items == NULL       */
    xy_gui_list_state_t      *state;   /* RAM selection state                */
    uint8_t                   str_id;  /* string ID for XY_GUI_F_INFO text   */
    uint8_t                   flags;   /* XY_GUI_F_* bitmask                 */
};

/* ── Declarative macros ─────────────────────────────────────────────── */

/**
 * Open the item array for a list.  Follow with XY_GUI_ITEM() entries, then
 * close with XY_GUI_END_LIST().
 */
#define XY_GUI_BEGIN_LIST(name)  \
    static const xy_gui_item_t _items_##name[] = {

/**
 * One selectable item within BEGIN_LIST … END_LIST.
 * @param str_id    String ID for the label.
 * @param next_list xy_gui_list_t* to navigate to on OK, or NULL.
 */
#define XY_GUI_ITEM(str_id_, next_list)  { (next_list), (str_id_) }

/**
 * Close the item array and define the list object.
 * @param name       C identifier for the xy_gui_list_t.
 * @param get_fn     xy_gui_get_fn or NULL.
 * @param set_fn     xy_gui_set_fn or NULL.
 * @param back_list  Parent list for BACK key, or NULL.
 * @param flags      XY_GUI_F_* bitmask.
 */
#define XY_GUI_END_LIST(name, get_fn, set_fn, back_list, flags_)               \
    };                                                                          \
    static xy_gui_list_state_t _state_##name = {0, 0};                         \
    xy_gui_list_t name = {                                                      \
        xy_gui_proc_list,                                                       \
        (get_fn), (set_fn),                                                     \
        (int8_t)(sizeof(_items_##name) / sizeof(_items_##name[0])),             \
        _items_##name,                                                          \
        (back_list), NULL, &_state_##name, 0u, (flags_)                         \
    }

/**
 * Define a single-page info/text list (no item array).
 * @param up_fn    xy_gui_get_fn called on UP key (or NULL for auto-scroll).
 * @param ok_fn    xy_gui_set_fn called on OK key (return 1 to navigate).
 */
#define XY_GUI_INFO_LIST(name, up_fn, ok_fn, back_list, next_list, str_id_, flags_) \
    xy_gui_list_t name = {                                                           \
        xy_gui_proc_list, (up_fn), (ok_fn),                                          \
        0, NULL, (back_list), (next_list), NULL,                                     \
        (str_id_), XY_GUI_F_INFO | (flags_)                                          \
    }

/** Forward-declare an externally defined list. */
#define XY_GUI_EXTERN(name)  extern xy_gui_list_t name

/* ── API ─────────────────────────────────────────────────────────────── */

/**
 * Provide the string table lookup function.
 * The function must return a NUL-terminated UTF-8 string for every str_id
 * used in the menus.  A default that returns "()" is used if not set.
 */
typedef const uint8_t *(*xy_gui_string_fn)(uint8_t str_id);
void xy_gui_set_string_fn(xy_gui_string_fn fn);

/** Initialise the GUI message queue.  Call once before xy_gui_switch(). */
void xy_gui_init(void);

/**
 * Switch to a new screen.
 * Sends XY_GUI_EV_EXIT to the current screen, then XY_GUI_EV_INIT to the
 * new one.  Updates g_xy_gui_list.
 */
void xy_gui_switch(xy_gui_list_t *list);

/**
 * Post an event into the GUI event queue (ISR-safe).
 * @return 0 on success, -1 if the queue is full.
 */
int xy_gui_post(xy_gui_ev_t ev, uint8_t key, void *data);

/** Convenience: post a key press event. */
static inline int xy_gui_post_key(uint8_t key)
{
    return xy_gui_post(XY_GUI_EV_KEY, key, NULL);
}

/**
 * Dispatch all pending events to the active screen proc.
 * Call each main-loop iteration.
 * @return Number of events dispatched.
 */
int xy_gui_run(void);

/** Standard list/menu message handler — used by the macros above. */
bool xy_gui_proc_list(xy_gui_msg_t *msg);

/** Currently active list (set by xy_gui_switch). */
extern xy_gui_list_t *g_xy_gui_list;

/* ── Queue depth ─────────────────────────────────────────────────────── */

#ifndef XY_GUI_QUEUE_SIZE
#define XY_GUI_QUEUE_SIZE  8u  /* must be power of 2 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_GUI_H */
