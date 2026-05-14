#ifndef _XY_COMMON_H_
#define _XY_COMMON_H_

#include <stdint.h>

#include "xy_config.h"
#include "xy_typedef.h"

typedef struct xy_list_node {
    int value;
    struct xy_list_node *next;
} xy_list_node;

#define xy_array_size(arr) (sizeof(arr) / sizeof((arr)[0]))
#define xy_max(a, b)       (((a) > (b)) ? (a) : (b))
#define xy_min(a, b)       (((a) < (b)) ? (a) : (b))

#define xy_set_bit(val, offset)           ((val) |= (1 << (offset)))
#define xy_set_bits(val, offset, bits)    ((val) |= (bits << (offset)))
#define xy_clear_bit(val, offset)         ((val) &= ~(1 << (offset)))
#define xy_clear_bits(val, offset, bits)  ((val) &= ~(bits << (offset)))
#define xy_toggle_bit(val, offset)        ((val) ^= (1 << (offset)))
#define xy_toggle_bits(val, offset, bits) ((val) ^= (bits << (offset)))
#define xy_get_bit(val, offset)           ((val) & (1 << (offset)))
#define xy_get_bits(val, offset, bits)    ((val) & (bits << (offset)))


#define xy_clamp(x, low, high) \
    (((x) < (low)) ? (low) : (((x) > (high)) ? (high) : (x)))
#define xy_swap(a, b, type) \
    do {                    \
        type temp = (a);    \
        (a)       = (b);    \
        (b)       = temp;   \
    } while (0)
#define xy_htonl(x)                                         \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) \
     | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#define xy_ntohl(x)     xy_htonl(x)
#define xy_htons(x)     ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
#define xy_rotl32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define xy_rotr32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define xy_rotl64(x, n) (((x) << (n)) | ((x) >> (64 - (n))))
#define xy_rotr64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))

#define xy_list_init_node(h) \
    do {                     \
        (h) = NULL;          \
    } while (0)

#define xy_list_add_note(h, n) \
    do {                       \
        if ((h) == NULL) {     \
            (h)       = (n);   \
            (n)->next = NULL;  \
        } else {               \
            (n)->next = (h);   \
            (h)       = (n);   \
        }                      \
    } while (0)

#define xy_list_add_note_tail(h, n, t) \
    do {                               \
        if ((h) == NULL) {             \
            (h) = (n);                 \
        } else {                       \
            (t) = (h);                 \
            while ((t) && (t)->next) { \
                (t) = (t)->next;       \
            }                          \
            (t)->next = (n);           \
        }                              \
        (n)->next = NULL;              \
    } while (0)

#define xy_list_del_node(h, n, t)        \
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

#define xy_list_for_node_safe(h, n, t) \
    for ((n) = (h); (n) && ((t) = (n)->next, 1); (n) = (t))

#define xy_list_for_node(h, n) for ((n) = (h); (n); (n) = (n)->next)

/*
@example
    struct node *next;
    char data;
};

struct node *head = NULL; //head init must be NULL(importent)
struct node *n, *t,*t1;
int i;

for (i = 0; i < 10; i++){
      n = malloc(sizeof(struct node));
      if (n == NULL){
              printf("malloc is null \n");
              return -1;
      }
      n->data = i;
      #if 0
        list_add_node(head,n); // head after
      #else
        list_add_node_tail(head,n,t); // head before
      #endif
}

list_for_node(head,n){
      printf("now node data %d \n",n->data);
}


list_for_node_safe(head,n,t){
    if (n->data = 5){//delete date is 5 node
        list_del_node(head,n,t1); //update node list
        free(n);
    }
}

list_for_node_safe(head,n,t){
     free(n);
}
*/
uint64_t xy_u64_div10(uint64_t u64val);

uint8_t xy_u8_mod10(uint8_t val);
uint16_t xy_u16_mod10(uint16_t val);
uint32_t xy_u32_mod10(uint32_t val);

uint32_t xy_hex2bcd(uint32_t hex);
uint32_t xy_bcd2hex(uint32_t bcd);
uint32_t xy_dec2bcd(uint32_t dec);
uint32_t xy_bcd2dec(uint32_t bcd);

#endif
