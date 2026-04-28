idf_component_register(
    SRCS "MQTT_main.c" "station_example_main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        esp_wifi 
        nvs_flash # 負責非揮發性儲存 Wi-Fi 的配對資訊都存這
        esp_http_server  # 讓你能在 ESP32 上架設網頁伺服器，這是做串流必備的
        esp32-camera # 這是攝影機驅動
        esp_psram
        freertos
)