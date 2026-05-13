#ifndef __XY_DEF_H__
#define __XY_DEF_H__

struct xy_slist {
    struct xy_slist *next;
} xy_slist_t;

#define xy_slist_init_node(h) \
    do {                      \
        (h) = NULL;           \
    } while (0)

#define xy_slist_add_note(h, n) \
    do {                        \
        if ((h) == NULL) {      \
            (h)       = (n);    \
            (n)->next = NULL;   \
        } else {                \
            (n)->next = (h);    \
            (h)       = (n);    \
        }                       \
    } while (0)

#define xy_slist_add_note_tail(h, n, t) \
    do {                                \
        if ((h) == NULL) {              \
            (h) = (n);                  \
        } else {                        \
            (t) = (h);                  \
            while ((t) && (t)->next) {  \
                (t) = (t)->next;        \
            }                           \
            (t)->next = (n);            \
        }                               \
        (n)->next = NULL;               \
    } while (0)

#define xy_slist_del_node(h, n, t)       \
    do {                                 \
        (t) = (h);                       \
        if ((t) != NULL && (t) == (n)) { \
            (h) = (t)->next;             \
            (t) = NULL;                  \
        }                                \
        while ((t)) {                    \
            if ((t)->next == (n)) {      \
                (t)->next = (n)->next;   \
                break;                   \
            }                            \
            (t) = (t)->next;             \
        }                                \
    } while (0)

#define xy_slist_for_node_safe(h, n, t) \
    for ((n) = (h); (n) && ((t) = (n)->next, 1); (n) = (t))

#define xy_slist_for_node(h, n) for ((n) = (h); (n); (n) = (n)->next)

typedef struct xy_dlist {
    struct xy_dlist *prev;
    struct xy_dlist *next;
} xy_dlist_t;

#define xy_dlist_init_node(h) \
    do {                      \
        (h) = NULL;           \
    } while (0)


#define xy_dlist_add_head(h, n) \
    do {                        \
        (n)->prev = NULL;       \
        (n)->next = (h);        \
        if ((h) != NULL) {      \
            (h)->prev = (n);    \
        }                       \
        (h) = (n);              \
    } while (0)


#define xy_dlist_add_tail(h, n, t) \
    do {                           \
        (n)->next = NULL;          \
        if ((h) == NULL) {         \
            (n)->prev = NULL;      \
            (h)       = (n);       \
        } else {                   \
            (t) = (h);             \
            while ((t)->next) {    \
                (t) = (t)->next;   \
            }                      \
            (t)->next = (n);       \
            (n)->prev = (t);       \
        }                          \
    } while (0)


#define xy_dlist_del_node(h, n)          \
    do {                                 \
        if ((n)->prev != NULL) {         \
            (n)->prev->next = (n)->next; \
        } else {                         \
            (h) = (n)->next;             \
        }                                \
        if ((n)->next != NULL) {         \
            (n)->next->prev = (n)->prev; \
        }                                \
        (n)->prev = NULL;                \
        (n)->next = NULL;                \
    } while (0)


#define xy_dlist_for_node_safe(h, n, t) \
    for ((n) = (h); (n) && ((t) = (n)->next, 1); (n) = (t))

#define xy_dlist_for_node(h, n) for ((n) = (h); (n); (n) = (n)->next)

#define xy_dlist_for_node_reverse(t, n) for ((n) = (t); (n); (n) = (n)->prev)

#define xy_dlist_insert_after(pos, n) \
    do {                              \
        (n)->next = (pos)->next;      \
        (n)->prev = (pos);            \
        if ((pos)->next) {            \
            (pos)->next->prev = (n);  \
        }                             \
        (pos)->next = (n);            \
    } while (0)


#define xy_dlist_insert_before(pos, n) \
    do {                               \
        (n)->prev = (pos)->prev;       \
        (n)->next = (pos);             \
        if ((pos)->prev) {             \
            (pos)->prev->next = (n);   \
        }                              \
        (pos)->prev = (n);             \
    } while (0)


struct xy_obj {
#if XY_NAME_MAX > 0
    char name[XY_NAME_MAX];
#endif
    uint8_t type;
    uint8_t flag;

    xy_dlist_t dlist;
};
struct xy_obj *xy_obj_t;

struct xy_device {
    struct xy_obj parent;

    uint8_t ref_count;
    uint8_t device_id;
};

struct xy_spinlock {
#ifdef XY_USING_DEBUG
    xy_uint32_t critical_level;
#endif /* RT_USING_DEBUG */
    xy_base_t lock;
};
#endif
