/**
 * @file xy_hal_dma.h
 * @brief DMA Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_DMA_H
#define XY_HAL_DMA_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "xy_hal.h"

/* DMA Direction */
typedef enum {
    XY_HAL_DMA_DIR_PERIPH_TO_MEM = 0, /**< Peripheral to memory */
    XY_HAL_DMA_DIR_MEM_TO_PERIPH,     /**< Memory to peripheral */
    XY_HAL_DMA_DIR_MEM_TO_MEM,        /**< Memory to memory */
} xy_hal_dma_direction_t;

/* DMA Mode */
typedef enum {
    XY_HAL_DMA_MODE_NORMAL = 0, /**< Normal mode */
    XY_HAL_DMA_MODE_CIRCULAR,   /**< Circular mode */
} xy_hal_dma_mode_t;

/* DMA Priority */
typedef enum {
    XY_HAL_DMA_PRIORITY_LOW = 0,   /**< Low priority */
    XY_HAL_DMA_PRIORITY_MEDIUM,    /**< Medium priority */
    XY_HAL_DMA_PRIORITY_HIGH,      /**< High priority */
    XY_HAL_DMA_PRIORITY_VERY_HIGH, /**< Very high priority */
} xy_hal_dma_priority_t;

/* DMA Data Width */
typedef enum {
    XY_HAL_DMA_WIDTH_BYTE = 0, /**< 8-bit */
    XY_HAL_DMA_WIDTH_HALFWORD, /**< 16-bit */
    XY_HAL_DMA_WIDTH_WORD,     /**< 32-bit */
} xy_hal_dma_width_t;

/* DMA Increment Mode */
typedef enum {
    XY_HAL_DMA_INCR_DISABLE = 0, /**< No increment */
    XY_HAL_DMA_INCR_ENABLE,      /**< Increment */
} xy_hal_dma_incr_t;

/* DMA Configuration Structure */
typedef struct {
    xy_hal_dma_direction_t direction; /**< Transfer direction */
    xy_hal_dma_mode_t mode;           /**< DMA mode */
    xy_hal_dma_priority_t priority;   /**< Priority level */
    xy_hal_dma_width_t periph_width;  /**< Peripheral data width */
    xy_hal_dma_width_t mem_width;     /**< Memory data width */
    xy_hal_dma_incr_t periph_incr;    /**< Peripheral increment */
    xy_hal_dma_incr_t mem_incr;       /**< Memory increment */
} xy_hal_dma_config_t;

/* DMA Event Types */
typedef enum {
    XY_HAL_DMA_EVENT_COMPLETE = 0,  /**< Transfer complete */
    XY_HAL_DMA_EVENT_HALF_COMPLETE, /**< Half transfer complete */
    XY_HAL_DMA_EVENT_ERROR,         /**< Transfer error */
} xy_hal_dma_event_t;

/* DMA callback */
typedef void (*xy_hal_dma_callback_t)(void *dma, xy_hal_dma_event_t event,
                                      void *arg);

/**
 * @brief Initialize DMA
 * @param dma DMA channel/stream instance
 * @param config DMA configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config);

/**
 * @brief Deinitialize DMA
 * @param dma DMA instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_dma_deinit(void *dma);

/**
 * @brief Start DMA transfer
 * @param dma DMA instance
 * @param src_addr Source address
 * @param dst_addr Destination address
 * @param data_len Data length
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr,
                                size_t data_len);

/**
 * @brief Stop DMA transfer
 * @param dma DMA instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_dma_stop(void *dma);

/**
 * @brief Register DMA callback
 * @param dma DMA instance
 * @param event Event type
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback,
                                            void *arg);

/**
 * @brief Get remaining data count
 * @param dma DMA instance
 * @return Remaining data count, negative on error
 */
int xy_hal_dma_get_counter(void *dma);

/**
 * @brief Poll for DMA transfer completion
 * @param dma DMA instance
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error/timeout
 */
xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_DMA_H */