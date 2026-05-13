/**
 * @file xy_observer.h
 * @brief Observer Pattern Implementation for IPC
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_OBSERVER_H
#define XY_OBSERVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_OBSERVER_MAX_OBSERVERS
#define XY_OBSERVER_MAX_OBSERVERS 16
#endif

#ifndef XY_OBSERVER_MAX_TOPICS
#define XY_OBSERVER_MAX_TOPICS 8
#endif

/* ==================== Error Codes ==================== */

#define XY_OBSERVER_OK              0
#define XY_OBSERVER_ERROR           (-1)
#define XY_OBSERVER_INVALID_PARAM   (-2)
#define XY_OBSERVER_NOT_FOUND       (-3)
#define XY_OBSERVER_FULL            (-4)

/* ==================== Data Structures ==================== */

/* Forward declaration */
typedef struct xy_observer xy_observer_t;
typedef struct xy_subject xy_subject_t;

/**
 * @brief Observer callback function type
 * @param subject Subject that triggered notification
 * @param data Notification data
 * @param user_data User-defined data
 */
typedef void (*observer_callback_t)(xy_subject_t *subject, const void *data, void *user_data);

/**
 * @brief Observer structure
 */
struct xy_observer {
    char name[32];
    observer_callback_t callback;
    void *user_data;
    bool active;
};

/**
 * @brief Subject (Observable) structure
 */
struct xy_subject {
    char name[32];
    xy_observer_t observers[XY_OBSERVER_MAX_OBSERVERS];
    size_t observer_count;
    bool notifying;
};

/* ==================== Observer Operations ==================== */

/**
 * @brief Initialize an observer
 * @param observer Observer handle
 * @param name Observer name
 * @param callback Callback function
 * @param user_data User-defined data
 * @return XY_OBSERVER_OK on success
 */
int xy_observer_init(xy_observer_t *observer, const char *name,
                     observer_callback_t callback, void *user_data);

/**
 * @brief Deinitialize an observer
 * @param observer Observer handle
 * @return XY_OBSERVER_OK on success
 */
int xy_observer_deinit(xy_observer_t *observer);

/* ==================== Subject Operations ==================== */

/**
 * @brief Initialize a subject
 * @param subject Subject handle
 * @param name Subject name
 * @return XY_OBSERVER_OK on success
 */
int xy_subject_init(xy_subject_t *subject, const char *name);

/**
 * @brief Deinitialize a subject
 * @param subject Subject handle
 * @return XY_OBSERVER_OK on success
 */
int xy_subject_deinit(xy_subject_t *subject);

/**
 * @brief Attach an observer to a subject
 * @param subject Subject handle
 * @param observer Observer to attach
 * @return XY_OBSERVER_OK on success, XY_OBSERVER_FULL if no space
 */
int xy_subject_attach(xy_subject_t *subject, xy_observer_t *observer);

/**
 * @brief Detach an observer from a subject
 * @param subject Subject handle
 * @param observer Observer to detach
 * @return XY_OBSERVER_OK on success, XY_OBSERVER_NOT_FOUND if not found
 */
int xy_subject_detach(xy_subject_t *subject, xy_observer_t *observer);

/**
 * @brief Notify all observers
 * @param subject Subject handle
 * @param data Data to pass to observers
 * @return XY_OBSERVER_OK on success
 */
int xy_subject_notify(xy_subject_t *subject, const void *data);

/**
 * @brief Get observer count
 * @param subject Subject handle
 * @return Number of attached observers
 */
size_t xy_subject_observer_count(xy_subject_t *subject);

/**
 * @brief Remove all observers
 * @param subject Subject handle
 * @return XY_OBSERVER_OK on success
 */
int xy_subject_clear(xy_subject_t *subject);

#ifdef __cplusplus
}
#endif

#endif /* XY_OBSERVER_H */
