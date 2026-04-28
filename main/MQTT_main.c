#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_http_server.h"
#include "cJSON.h"

// 定義系統狀態結構體
typedef struct {
    char obstacle[20];
    int inference_time_ms;
} SystemState_t;

SystemState_t current_state;       // 宣告全域變數來儲存最新狀態
SemaphoreHandle_t state_mutex;     // 宣告 Mutex (互斥鎖)

void parse_incoming_json(const char* json_string) {
    // 1. 將字串解析為 cJSON 物件
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        printf("JSON 解析失敗\n");
        return;
    }

    // 2. 提取欄位資料 (以 obstacle 為例)
    cJSON *obstacle = cJSON_GetObjectItem(root, "obstacle");
    if (cJSON_IsString(obstacle) && (obstacle->valuestring != NULL)) {
        printf("當前障礙物狀態: %s\n", obstacle->valuestring);
        // 在這裡可以將狀態存入全域變數，供 WebServer 讀取
    }

    // 3. 提取數值資料 (以 inference_time_ms 為例)
    cJSON *inference_time = cJSON_GetObjectItem(root, "inference_time_ms");
    if (cJSON_IsNumber(inference_time)) {
        printf("推論時間: %d ms\n", inference_time->valueint);
    }

    // ⚠️ 極度重要：解析完畢後，務必刪除 cJSON 物件以釋放記憶體！
    cJSON_Delete(root); 
}