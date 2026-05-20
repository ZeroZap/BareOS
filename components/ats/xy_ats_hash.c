/**
 * @file xy_ats_hash.c
 * @brief AT Server Hash Table Implementation
 * @version 1.0.0
 * @date 2026-03-21
 */

#include "xy_ats_hash.h"
#include "xy_string.h"

/* ==================== Helper Functions ==================== */

/**
 * @brief DJB2 hash function for strings
 * @param str Input string
 * @return Hash value
 */
static uint32_t djb2_hash(const char *str)
{
    uint32_t hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
}

/* ==================== Hash Table Implementation ==================== */

bool ats_hash_init(ats_hash_table_t *table)
{
    if (!table) {
        return false;
    }
    
    // Clear all buckets
    for (size_t i = 0; i < ATS_HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
    
    // Reset node pool
    table->used_nodes = 0;
    table->initialized = true;
    
    return true;
}

uint32_t ats_hash_calculate(const char *cmd_name)
{
    if (!cmd_name) {
        return 0;
    }
    
    uint32_t hash = djb2_hash(cmd_name);
    return hash & (ATS_HASH_TABLE_SIZE - 1); // Assuming power of 2 size
}

bool ats_hash_insert(ats_hash_table_t *table, const char *cmd_name, struct at_cmd *cmd)
{
    if (!table || !cmd_name || !cmd || !table->initialized) {
        return false;
    }
    
    // Check if table is full
    if (table->used_nodes >= ATS_HASH_MAX_COMMANDS) {
        return false;
    }
    
    uint32_t bucket_index = ats_hash_calculate(cmd_name);
    
    // Check if command already exists
    ats_hash_node_t *current = table->buckets[bucket_index];
    while (current) {
        if (strcmp(current->cmd_name, cmd_name) == 0) {
            // Command already exists, update it
            current->cmd = cmd;
            return true;
        }
        current = current->next;
    }
    
    // Allocate new node from pool
    ats_hash_node_t *new_node = &table->nodes[table->used_nodes++];
    new_node->cmd_name = cmd_name;
    new_node->cmd = cmd;
    new_node->next = table->buckets[bucket_index];
    table->buckets[bucket_index] = new_node;
    
    return true;
}

struct at_cmd* ats_hash_find(ats_hash_table_t *table, const char *cmd_name)
{
    if (!table || !cmd_name || !table->initialized) {
        return NULL;
    }
    
    uint32_t bucket_index = ats_hash_calculate(cmd_name);
    ats_hash_node_t *current = table->buckets[bucket_index];
    
    while (current) {
        if (strcmp(current->cmd_name, cmd_name) == 0) {
            return current->cmd;
        }
        current = current->next;
    }
    
    return NULL;
}

bool ats_hash_remove(ats_hash_table_t *table, const char *cmd_name)
{
    if (!table || !cmd_name || !table->initialized) {
        return false;
    }
    
    uint32_t bucket_index = ats_hash_calculate(cmd_name);
    ats_hash_node_t *current = table->buckets[bucket_index];
    ats_hash_node_t *prev = NULL;
    
    while (current) {
        if (strcmp(current->cmd_name, cmd_name) == 0) {
            // Found the node to remove
            if (prev) {
                prev->next = current->next;
            } else {
                table->buckets[bucket_index] = current->next;
            }
            
            // Note: We don't actually free the node since it's from a static pool
            // The node will be reused when inserting new commands
            table->used_nodes--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    return false;
}

size_t ats_hash_count(ats_hash_table_t *table)
{
    if (!table || !table->initialized) {
        return 0;
    }
    
    return table->used_nodes;
}