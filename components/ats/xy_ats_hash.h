/**
 * @file xy_ats_hash.h
 * @brief AT Server Hash Table for Command Mapping
 * @version 1.0.0
 * @date 2026-03-21
 */

#ifndef XY_ATS_HASH_H
#define XY_ATS_HASH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct at_cmd;

/* ==================== Configuration ==================== */

#ifndef ATS_HASH_TABLE_SIZE
#define ATS_HASH_TABLE_SIZE 32  ///< Hash table bucket count (power of 2 recommended)
#endif

#ifndef ATS_HASH_MAX_COMMANDS  
#define ATS_HASH_MAX_COMMANDS 64 ///< Maximum number of commands that can be stored
#endif

/* ==================== Hash Table Types ==================== */

/**
 * @brief Hash table node for command storage
 */
typedef struct ats_hash_node {
    const char *cmd_name;           ///< Command name (e.g., "AT+CMD")
    struct at_cmd *cmd;             ///< Pointer to command structure
    struct ats_hash_node *next;     ///< Next node in collision chain
} ats_hash_node_t;

/**
 * @brief Hash table structure
 */
typedef struct ats_hash_table {
    ats_hash_node_t *buckets[ATS_HASH_TABLE_SIZE];  ///< Array of bucket pointers
    ats_hash_node_t nodes[ATS_HASH_MAX_COMMANDS];   ///< Pre-allocated node pool
    size_t used_nodes;                              ///< Number of used nodes
    bool initialized;                               ///< Initialization flag
} ats_hash_table_t;

/* ==================== Function Prototypes ==================== */

/**
 * @brief Initialize hash table
 * @param table Hash table to initialize
 * @return true on success, false on failure
 */
bool ats_hash_init(ats_hash_table_t *table);

/**
 * @brief Calculate hash value for command name
 * @param cmd_name Command name string
 * @return Hash value (0 to ATS_HASH_TABLE_SIZE-1)
 */
uint32_t ats_hash_calculate(const char *cmd_name);

/**
 * @brief Insert command into hash table
 * @param table Hash table
 * @param cmd_name Command name
 * @param cmd Command structure pointer
 * @return true on success, false on failure (table full)
 */
bool ats_hash_insert(ats_hash_table_t *table, const char *cmd_name, struct at_cmd *cmd);

/**
 * @brief Find command in hash table
 * @param table Hash table
 * @param cmd_name Command name to find
 * @return Command structure pointer, or NULL if not found
 */
struct at_cmd* ats_hash_find(ats_hash_table_t *table, const char *cmd_name);

/**
 * @brief Remove command from hash table
 * @param table Hash table
 * @param cmd_name Command name to remove
 * @return true on success, false if not found
 */
bool ats_hash_remove(ats_hash_table_t *table, const char *cmd_name);

/**
 * @brief Get number of commands in hash table
 * @param table Hash table
 * @return Number of commands
 */
size_t ats_hash_count(ats_hash_table_t *table);

#ifdef __cplusplus
}
#endif

#endif // XY_ATS_HASH_H