#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_http_server.h"
#include "cJSON.h"

typedef struct
{
    char obstacle[20];
    int inference_time_ms;
} SystemState_t;

SystemState_t current_state;   // 宣告全域變數來儲存最新狀態
SemaphoreHandle_t state_mutex; // 宣告 Mutex (互斥鎖)

esp_err_t api_data_get_handler(httpd_req_t *req)
{
    // 1. 建立一個空的 cJSON 物件準備回傳
    cJSON *root = cJSON_CreateObject();

    // 2. 安全地讀取全域變數 (嘗試取得 Mutex 鎖)
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE)
    {
        // 成功上鎖！現在只有這個 Task 可以存取 current_state
        cJSON_AddStringToObject(root, "obstacle", current_state.obstacle);
        cJSON_AddNumberToObject(root, "inference_time_ms", current_state.inference_time_ms);

        // 讀取完畢，務必解鎖，讓 MQTT 任務可以繼續更新資料
        xSemaphoreGive(state_mutex);
    }
    else
    {
        // 如果拿不到鎖 (極少發生)，可以直接回傳錯誤
        httpd_resp_send_500(req);
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // 3. 將 cJSON 物件轉為一般字串
    char *json_string_response = cJSON_Print(root);

    // 4. 設定 HTTP Header 並發送回應給瀏覽器
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string_response, strlen(json_string_response));

    // 5. ⚠️ 極度重要：釋放記憶體防 Memory Leak
    free(json_string_response); // cJSON_Print 會使用 malloc 分配記憶體，必須用 free 釋放
    cJSON_Delete(root);         // 刪除 cJSON 物件

    return ESP_OK;
}

// 定義 URI 結構
httpd_uri_t api_data_uri = {
    .uri = "/api/data",              // 前端要請求的網址
    .method = HTTP_GET,              // 請求方法
    .handler = api_data_get_handler, // 綁定函數
    .user_ctx = NULL};

void parse_incoming_json(const char *json_string)
{
    // 1. 將字串解析為 cJSON 物件
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL)
    {
        printf("JSON 解析失敗\n");
        return;
    }

    // 2. 提取欄位資料 (以 obstacle 為例)
    cJSON *obstacle = cJSON_GetObjectItem(root, "obstacle");
    if (cJSON_IsString(obstacle) && (obstacle->valuestring != NULL))
    {
        printf("當前障礙物狀態: %s\n", obstacle->valuestring);
        // 在這裡可以將狀態存入全域變數，供 WebServer 讀取
    }

    // 3. 提取數值資料 (以 inference_time_ms 為例)
    cJSON *inference_time = cJSON_GetObjectItem(root, "inference_time_ms");
    if (cJSON_IsNumber(inference_time))
    {
        printf("推論時間: %d ms\n", inference_time->valueint);
    }

    // ⚠️ 極度重要：解析完畢後，務必刪除 cJSON 物件以釋放記憶體！
    cJSON_Delete(root);
}