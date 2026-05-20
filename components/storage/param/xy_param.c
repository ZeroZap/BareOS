/**
 * @file xy_param.c
 * @brief PLB device parameter storage implementation.
 */

#include "xy_param.h"
#include "xy_string.h"

/* ── RAM shadow ─────────────────────────────────────────────────────── */

static struct {
    eflash_t *fee;

    uint32_t mmsi;
    char     serial[17];      /* 16 chars + NUL */
    uint8_t  tx_interval;
    uint8_t  flags;
    char     server[33];      /* 32 chars + NUL */
    int32_t  last_lat;
    int32_t  last_lon;
    uint32_t tx_count;

    uint16_t dirty;           /* bitmask of XY_PARAM_ID_* that need saving */
} s;

#define MARK_DIRTY(id)   (s.dirty |= (1u << (id)))
#define IS_DIRTY(id)     (s.dirty &  (1u << (id)))
#define CLEAR_DIRTY(id)  (s.dirty &= ~(1u << (id)))

/* ── Helpers ────────────────────────────────────────────────────────── */

static void load_u8(xy_param_id_t id, uint8_t *dst, uint8_t def)
{
    uint8_t v;
    if (eflash_read(s.fee, id, &v, 1) == EFLASH_OK) *dst = v;
    else *dst = def;
}

static void load_u32(xy_param_id_t id, uint32_t *dst, uint32_t def)
{
    if (eflash_read(s.fee, id, dst, 4) != EFLASH_OK) *dst = def;
}

static void load_i32(xy_param_id_t id, int32_t *dst, int32_t def)
{
    if (eflash_read(s.fee, id, dst, 4) != EFLASH_OK) *dst = def;
}

static void load_str(xy_param_id_t id, char *dst, int maxlen, const char *def)
{
    if (eflash_read(s.fee, id, dst, (uint32_t)(maxlen - 1)) != EFLASH_OK) {
        strncpy(dst, def, (size_t)(maxlen - 1));
    }
    dst[maxlen - 1] = '\0';
}

/* ── Init ───────────────────────────────────────────────────────────── */

void xy_param_init(eflash_t *fee)
{
    s.fee   = fee;
    s.dirty = 0;

    load_u32(XY_PARAM_ID_MMSI,        &s.mmsi,        XY_PARAM_DEFAULT_MMSI);
    load_str(XY_PARAM_ID_SERIAL,       s.serial,  17,  "UNSET");
    load_u8 (XY_PARAM_ID_TX_INTERVAL, &s.tx_interval, XY_PARAM_DEFAULT_TX_INTERVAL);
    load_u8 (XY_PARAM_ID_FLAGS,       &s.flags,       XY_PARAM_DEFAULT_FLAGS);
    load_str(XY_PARAM_ID_SERVER_ADDR,  s.server,  33,  XY_PARAM_DEFAULT_SERVER);
    load_i32(XY_PARAM_ID_LAST_LAT,    &s.last_lat,    0);
    load_i32(XY_PARAM_ID_LAST_LON,    &s.last_lon,    0);
    load_u32(XY_PARAM_ID_TX_COUNT,    &s.tx_count,    0);
}

/* ── Getters ────────────────────────────────────────────────────────── */

uint32_t xy_param_get_mmsi(void)         { return s.mmsi;        }
uint8_t  xy_param_get_tx_interval(void)  { return s.tx_interval; }
uint8_t  xy_param_get_flags(void)        { return s.flags;       }
int32_t  xy_param_get_last_lat(void)     { return s.last_lat;    }
int32_t  xy_param_get_last_lon(void)     { return s.last_lon;    }
uint32_t xy_param_get_tx_count(void)     { return s.tx_count;    }

void xy_param_get_serial(char *out, int maxlen)
{
    strncpy(out, s.serial, (size_t)(maxlen - 1));
    out[maxlen - 1] = '\0';
}

void xy_param_get_server(char *out, int maxlen)
{
    strncpy(out, s.server, (size_t)(maxlen - 1));
    out[maxlen - 1] = '\0';
}

/* ── Setters ────────────────────────────────────────────────────────── */

void xy_param_set_mmsi(uint32_t mmsi)
{
    if (s.mmsi != mmsi) { s.mmsi = mmsi; MARK_DIRTY(XY_PARAM_ID_MMSI); }
}

void xy_param_set_serial(const char *serial)
{
    strncpy(s.serial, serial, 16);
    s.serial[16] = '\0';
    MARK_DIRTY(XY_PARAM_ID_SERIAL);
}

void xy_param_set_tx_interval(uint8_t minutes)
{
    if (s.tx_interval != minutes) {
        s.tx_interval = minutes;
        MARK_DIRTY(XY_PARAM_ID_TX_INTERVAL);
    }
}

void xy_param_set_flags(uint8_t flags)
{
    if (s.flags != flags) { s.flags = flags; MARK_DIRTY(XY_PARAM_ID_FLAGS); }
}

void xy_param_set_flag(uint8_t mask, bool value)
{
    uint8_t newflags = value ? (s.flags | mask) : (s.flags & (uint8_t)~mask);
    xy_param_set_flags(newflags);
}

void xy_param_set_server(const char *addr)
{
    strncpy(s.server, addr, 32);
    s.server[32] = '\0';
    MARK_DIRTY(XY_PARAM_ID_SERVER_ADDR);
}

void xy_param_set_last_pos(int32_t lat_1e7, int32_t lon_1e7)
{
    s.last_lat = lat_1e7;
    s.last_lon = lon_1e7;
    MARK_DIRTY(XY_PARAM_ID_LAST_LAT);
    MARK_DIRTY(XY_PARAM_ID_LAST_LON);
}

void xy_param_inc_tx_count(void)
{
    s.tx_count++;
    MARK_DIRTY(XY_PARAM_ID_TX_COUNT);
}

/* ── Persistence ────────────────────────────────────────────────────── */

int xy_param_save(void)
{
    int rc = 0;

    if (IS_DIRTY(XY_PARAM_ID_MMSI)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_MMSI, &s.mmsi, 4);
        CLEAR_DIRTY(XY_PARAM_ID_MMSI);
    }
    if (IS_DIRTY(XY_PARAM_ID_SERIAL)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_SERIAL, s.serial, 16);
        CLEAR_DIRTY(XY_PARAM_ID_SERIAL);
    }
    if (IS_DIRTY(XY_PARAM_ID_TX_INTERVAL)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_TX_INTERVAL, &s.tx_interval, 1);
        CLEAR_DIRTY(XY_PARAM_ID_TX_INTERVAL);
    }
    if (IS_DIRTY(XY_PARAM_ID_FLAGS)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_FLAGS, &s.flags, 1);
        CLEAR_DIRTY(XY_PARAM_ID_FLAGS);
    }
    if (IS_DIRTY(XY_PARAM_ID_SERVER_ADDR)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_SERVER_ADDR, s.server, 32);
        CLEAR_DIRTY(XY_PARAM_ID_SERVER_ADDR);
    }
    if (IS_DIRTY(XY_PARAM_ID_LAST_LAT)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_LAST_LAT, &s.last_lat, 4);
        CLEAR_DIRTY(XY_PARAM_ID_LAST_LAT);
    }
    if (IS_DIRTY(XY_PARAM_ID_LAST_LON)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_LAST_LON, &s.last_lon, 4);
        CLEAR_DIRTY(XY_PARAM_ID_LAST_LON);
    }
    if (IS_DIRTY(XY_PARAM_ID_TX_COUNT)) {
        rc |= eflash_write(s.fee, XY_PARAM_ID_TX_COUNT, &s.tx_count, 4);
        CLEAR_DIRTY(XY_PARAM_ID_TX_COUNT);
    }

    return rc;
}

int xy_param_factory_reset(void)
{
    int rc = eflash_erase_all(s.fee);
    if (rc == EFLASH_OK) {
        /* Reload defaults into RAM shadow. */
        s.mmsi        = XY_PARAM_DEFAULT_MMSI;
        s.tx_interval = XY_PARAM_DEFAULT_TX_INTERVAL;
        s.flags       = XY_PARAM_DEFAULT_FLAGS;
        strncpy(s.serial, "UNSET", 16);
        strncpy(s.server, XY_PARAM_DEFAULT_SERVER, 32);
        s.last_lat    = 0;
        s.last_lon    = 0;
        s.tx_count    = 0;
        s.dirty       = 0;
    }
    return (rc == EFLASH_OK) ? 0 : -1;
}
