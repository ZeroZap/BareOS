#ifndef XY_DSC_H
#define XY_DSC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * DSC format codes (ITU-R M.493 Table 1)
 * ----------------------------------------------------------------------- */
typedef uint8_t dsc_format_t;
#define DSC_FMT_DISTRESS    102u
#define DSC_FMT_ALL_SHIPS   112u
#define DSC_FMT_INDIVIDUAL  120u
#define DSC_FMT_GEO_AREA    114u

/* -----------------------------------------------------------------------
 * DSC category codes
 * ----------------------------------------------------------------------- */
typedef uint8_t dsc_category_t;
#define DSC_CAT_DISTRESS    108u
#define DSC_CAT_URGENCY     110u
#define DSC_CAT_SAFETY      112u

/* -----------------------------------------------------------------------
 * Distress nature codes (ITU-R M.493 Table 2)
 * ----------------------------------------------------------------------- */
typedef uint8_t dsc_distress_t;
#define DSC_DIST_UNDESIGNATED  100u
#define DSC_DIST_FIRE          102u
#define DSC_DIST_FLOODING      104u
#define DSC_DIST_COLLISION     106u
#define DSC_DIST_GROUNDING     108u
#define DSC_DIST_LISTING       110u
#define DSC_DIST_SINKING       112u
#define DSC_DIST_DISABLED      114u
#define DSC_DIST_PIRACY        116u
#define DSC_DIST_MOB           120u
#define DSC_DIST_EPIRB         122u

/* -----------------------------------------------------------------------
 * Decoded DSC call
 * ----------------------------------------------------------------------- */
typedef struct {
    dsc_format_t   format;
    char           address[12];     /* MMSI or area code, NUL-terminated */
    dsc_category_t category;
    dsc_distress_t distress_type;   /* valid when category == DSC_CAT_DISTRESS */
    char           lat_str[8];      /* e.g. "3144.5N", empty if absent */
    char           lon_str[9];      /* e.g. "12131.5E", empty if absent */
    uint8_t        utc_hour;
    uint8_t        utc_min;
    /* Expansion data from $CDDSE */
    char           src_mmsi[10];
    char           rcvd_mmsi[10];
    bool           has_position;
    bool           has_expansion;
} dsc_call_t;

typedef void (*dsc_call_cb)(const dsc_call_t *call, void *user);

/* -----------------------------------------------------------------------
 * Parser state (caller-allocated)
 * ----------------------------------------------------------------------- */
typedef struct {
    dsc_call_t  pending;
    bool        has_pending;
    dsc_call_cb cb;
    void       *user;
} dsc_parser_t;

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * Initialise parser.  cb is fired for each complete DSC call.
 */
void dsc_parser_init(dsc_parser_t *p, dsc_call_cb cb, void *user);

/**
 * Feed one complete NMEA sentence (NUL-terminated, including leading '$').
 * Validates XOR checksum.  Accepts talker prefixes CD, DS, HE.
 * Fires cb on valid $CDDSC.  Fires cb again when the matching $CDDSE
 * arrives (has_expansion will be true on that second call).
 * Returns true if the sentence was recognised and valid.
 */
bool dsc_feed_sentence(dsc_parser_t *p, const char *sentence);

#ifdef __cplusplus
}
#endif

#endif /* XY_DSC_H */
