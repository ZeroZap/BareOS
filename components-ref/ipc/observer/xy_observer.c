/**
 * @file xy_observer.c
 * @brief Observer Pattern Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_observer.h"
#include <string.h>

/* ==================== Observer Implementation ==================== */

int xy_observer_init(xy_observer_t *observer, const char *name,
                     observer_callback_t callback, void *user_data)
{
    if (!observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    memset(observer, 0, sizeof(*observer));

    if (name) {
        strncpy(observer->name, name, sizeof(observer->name) - 1);
        observer->name[sizeof(observer->name) - 1] = '\0';
    }

    observer->callback = callback;
    observer->user_data = user_data;
    observer->active = true;

    return XY_OBSERVER_OK;
}

int xy_observer_deinit(xy_observer_t *observer)
{
    if (!observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    observer->callback = NULL;
    observer->user_data = NULL;
    observer->active = false;

    return XY_OBSERVER_OK;
}

/* ==================== Subject Implementation ==================== */

int xy_subject_init(xy_subject_t *subject, const char *name)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    memset(subject, 0, sizeof(*subject));

    if (name) {
        strncpy(subject->name, name, sizeof(subject->name) - 1);
        subject->name[sizeof(subject->name) - 1] = '\0';
    }

    subject->observer_count = 0;
    subject->notifying = false;

    return XY_OBSERVER_OK;
}

int xy_subject_deinit(xy_subject_t *subject)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    return xy_subject_clear(subject);
}

int xy_subject_attach(xy_subject_t *subject, xy_observer_t *observer)
{
    if (!subject || !observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    if (subject->observer_count >= XY_OBSERVER_MAX_OBSERVERS) {
        return XY_OBSERVER_FULL;
    }

    /* Check if already attached */
    for (size_t i = 0; i < subject->observer_count; i++) {
        if (subject->observers[i].callback == observer->callback) {
            return XY_OBSERVER_OK; /* Already attached */
        }
    }

    /* Add observer */
    memcpy(&subject->observers[subject->observer_count], observer, sizeof(*observer));
    subject->observer_count++;

    return XY_OBSERVER_OK;
}

int xy_subject_detach(xy_subject_t *subject, xy_observer_t *observer)
{
    if (!subject || !observer) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    for (size_t i = 0; i < subject->observer_count; i++) {
        if (subject->observers[i].callback == observer->callback) {
            /* Remove observer by shifting remaining observers */
            for (size_t j = i; j < subject->observer_count - 1; j++) {
                memcpy(&subject->observers[j], &subject->observers[j + 1], sizeof(*observer));
            }
            subject->observer_count--;
            return XY_OBSERVER_OK;
        }
    }

    return XY_OBSERVER_NOT_FOUND;
}

int xy_subject_notify(xy_subject_t *subject, const void *data)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    subject->notifying = true;

    /* Notify all observers */
    for (size_t i = 0; i < subject->observer_count; i++) {
        if (subject->observers[i].active && subject->observers[i].callback) {
            subject->observers[i].callback(subject, data, subject->observers[i].user_data);
        }
    }

    subject->notifying = false;

    return XY_OBSERVER_OK;
}

size_t xy_subject_observer_count(xy_subject_t *subject)
{
    if (!subject) {
        return 0;
    }

    return subject->observer_count;
}

int xy_subject_clear(xy_subject_t *subject)
{
    if (!subject) {
        return XY_OBSERVER_INVALID_PARAM;
    }

    memset(subject->observers, 0, sizeof(subject->observers));
    subject->observer_count = 0;

    return XY_OBSERVER_OK;
}
