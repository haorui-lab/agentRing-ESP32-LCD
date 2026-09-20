#include "sync_payload.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "SyncPayload";

static void safe_strcpy(char *dst, const char *src, size_t max_len) {
    if (!dst || max_len == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, max_len - 1);
    dst[max_len - 1] = '\0';
}

static void parse_limit_item(const cJSON *item_obj, limit_item_t *item) {
    if (!item_obj || !item) return;

    cJSON *label = cJSON_GetObjectItemCaseSensitive(item_obj, "label");
    if (cJSON_IsString(label) && label->valuestring) {
        safe_strcpy(item->label, label->valuestring, sizeof(item->label));
    }

    cJSON *pct = cJSON_GetObjectItemCaseSensitive(item_obj, "remainingPercent");
    if (cJSON_IsNumber(pct)) {
        item->remaining_percent = pct->valuedouble;
    } else {
        item->remaining_percent = 100.0;
    }

    cJSON *resets = cJSON_GetObjectItemCaseSensitive(item_obj, "resetsAt");
    if (cJSON_IsString(resets) && resets->valuestring) {
        safe_strcpy(item->resets_at, resets->valuestring, sizeof(item->resets_at));
    }

    cJSON *details = cJSON_GetObjectItemCaseSensitive(item_obj, "remainingDetails");
    if (cJSON_IsString(details) && details->valuestring) {
        safe_strcpy(item->remaining_details, details->valuestring, sizeof(item->remaining_details));
    }
}

static void parse_limit_row(const cJSON *row_obj, limit_row_t *row) {
    if (!row_obj || !row) return;

    cJSON *label = cJSON_GetObjectItemCaseSensitive(row_obj, "label");
    if (cJSON_IsString(label) && label->valuestring) {
        safe_strcpy(row->label, label->valuestring, sizeof(row->label));
    }

    cJSON *pct = cJSON_GetObjectItemCaseSensitive(row_obj, "percent");
    if (cJSON_IsString(pct) && pct->valuestring) {
        safe_strcpy(row->percent, pct->valuestring, sizeof(row->percent));
    } else if (cJSON_IsNumber(pct)) {
        snprintf(row->percent, sizeof(row->percent), "%d%%", (int)pct->valuedouble);
    }

    cJSON *reset = cJSON_GetObjectItemCaseSensitive(row_obj, "reset");
    if (cJSON_IsString(reset) && reset->valuestring) {
        safe_strcpy(row->reset, reset->valuestring, sizeof(row->reset));
    }
}

bool parse_sync_payload(const char *json_str, sync_payload_t *out_payload) {
    if (!json_str || !out_payload) return false;

    memset(out_payload, 0, sizeof(sync_payload_t));

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGW(TAG, "JSON parse error before: [%.32s]", json_str);
        return false;
    }

    // Check type (e.g. "ping")
    cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
    if (cJSON_IsString(type) && type->valuestring) {
        safe_strcpy(out_payload->type, type->valuestring, sizeof(out_payload->type));
    }

    cJSON *ts = cJSON_GetObjectItemCaseSensitive(root, "timestamp");
    if (cJSON_IsNumber(ts)) {
        out_payload->timestamp = (int64_t)ts->valuedouble;
    }

    // If it's a ping packet, we're done
    if (strcmp(out_payload->type, "ping") == 0) {
        cJSON_Delete(root);
        return true;
    }

    cJSON *providers = cJSON_GetObjectItemCaseSensitive(root, "providers");
    if (cJSON_IsArray(providers)) {
        int count = cJSON_GetArraySize(providers);
        if (count > MAX_PROVIDERS) count = MAX_PROVIDERS;
        out_payload->provider_count = count;

        for (int i = 0; i < count; i++) {
            cJSON *p_obj = cJSON_GetArrayItem(providers, i);
            if (!p_obj) continue;

            provider_data_t *p_data = &out_payload->providers[i];

            cJSON *id = cJSON_GetObjectItemCaseSensitive(p_obj, "id");
            if (cJSON_IsString(id) && id->valuestring) {
                safe_strcpy(p_data->id, id->valuestring, sizeof(p_data->id));
            }

            cJSON *name = cJSON_GetObjectItemCaseSensitive(p_obj, "name");
            if (cJSON_IsString(name) && name->valuestring) {
                safe_strcpy(p_data->name, name->valuestring, sizeof(p_data->name));
            }

            cJSON *primary = cJSON_GetObjectItemCaseSensitive(p_obj, "primary");
            if (cJSON_IsObject(primary)) {
                p_data->has_primary = true;
                parse_limit_item(primary, &p_data->primary);
            }

            cJSON *secondary = cJSON_GetObjectItemCaseSensitive(p_obj, "secondary");
            if (cJSON_IsObject(secondary)) {
                p_data->has_secondary = true;
                parse_limit_item(secondary, &p_data->secondary);
            }

            cJSON *rows = cJSON_GetObjectItemCaseSensitive(p_obj, "rows");
            if (cJSON_IsArray(rows)) {
                int row_count = cJSON_GetArraySize(rows);
                if (row_count > MAX_ROWS_PER_PROVIDER) row_count = MAX_ROWS_PER_PROVIDER;
                p_data->row_count = row_count;

                for (int r = 0; r < row_count; r++) {
                    cJSON *r_obj = cJSON_GetArrayItem(rows, r);
                    if (r_obj) {
                        parse_limit_row(r_obj, &p_data->rows[r]);
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    return true;
}

bool is_payload_equal(const sync_payload_t *a, const sync_payload_t *b) {
    if (!a || !b) return false;
    if (a->provider_count != b->provider_count) return false;

    for (int i = 0; i < a->provider_count; i++) {
        const provider_data_t *pa = &a->providers[i];
        const provider_data_t *pb = &b->providers[i];

        if (strcmp(pa->id, pb->id) != 0) return false;
        if (strcmp(pa->name, pb->name) != 0) return false;
        if (pa->has_primary != pb->has_primary) return false;
        if (pa->has_secondary != pb->has_secondary) return false;

        if (pa->has_primary) {
            if (fabs(pa->primary.remaining_percent - pb->primary.remaining_percent) > 0.05) return false;
            if (strcmp(pa->primary.resets_at, pb->primary.resets_at) != 0) return false;
        }

        if (pa->has_secondary) {
            if (fabs(pa->secondary.remaining_percent - pb->secondary.remaining_percent) > 0.05) return false;
            if (strcmp(pa->secondary.resets_at, pb->secondary.resets_at) != 0) return false;
        }

        if (pa->row_count != pb->row_count) return false;
        for (int r = 0; r < pa->row_count; r++) {
            if (strcmp(pa->rows[r].label, pb->rows[r].label) != 0) return false;
            if (strcmp(pa->rows[r].percent, pb->rows[r].percent) != 0) return false;
            if (strcmp(pa->rows[r].reset, pb->rows[r].reset) != 0) return false;
        }
    }

    return true;
}
