#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROVIDERS 6
#define MAX_ROWS_PER_PROVIDER 6

typedef struct {
    char label[32];
    double remaining_percent;   // 0.0 ~ 100.0
    char resets_at[32];         // e.g. "4d 14h", "3h 33m"
    char remaining_details[32]; // e.g. "120 / 500"
} limit_item_t;

typedef struct {
    char label[32];             // e.g. "7天", "Gemini 5小时"
    char percent[16];           // e.g. "84%", "$25.00"
    char reset[32];             // e.g. "4d 14h"
} limit_row_t;

typedef struct {
    char id[32];                // e.g. "codex", "antigravity", "cursor"
    char name[32];              // e.g. "Codex", "Antigravity"
    bool has_primary;
    limit_item_t primary;
    bool has_secondary;
    limit_item_t secondary;
    int row_count;
    limit_row_t rows[MAX_ROWS_PER_PROVIDER];
} provider_data_t;

typedef struct {
    char type[16];              // "ping" if heartbeat frame
    int64_t timestamp;
    int provider_count;
    provider_data_t providers[MAX_PROVIDERS];
} sync_payload_t;

/**
 * @brief Parse line-delimited JSON string into sync_payload_t
 * @param json_str Null-terminated JSON string
 * @param out_payload Destination payload struct
 * @return true on success, false otherwise
 */
bool parse_sync_payload(const char *json_str, sync_payload_t *out_payload);

/**
 * @brief Compare whether two payloads are identical in data content
 */
bool is_payload_equal(const sync_payload_t *a, const sync_payload_t *b);

#ifdef __cplusplus
}
#endif
