// ======== ОТПРАВКА СООБЩЕНИЯ В ТЕЛЕГРАМ ПРИ СРАБАТЫВАНИИ PIR ДАТЧИКА ========
// ===================== ПОДКЛЮЧЕНИЯ =====================

#include <stdio.h>
#include <string>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"  // <- берём значения паролей из menuconfig
#include "esp_sntp.h"

// ===== ГОТОВЫЕ БИБЛИОТЕКИ =====

#include "WifiManager/WifiManager.hpp"
#include "HttpClient/HttpClient.hpp"

// ===================== НАСТРОЙКИ =====================

#define PIR_GPIO GPIO_NUM_4
#define LED_GPIO GPIO_NUM_2

#define LED_BLINK_DURATION_MS 5000
#define LED_BLINK_PERIOD_MS   200

//#define BOT_TOKEN "8438508129:AAEaH39kNp5BDXTvC4kqovybLFZxN0M1IlA"
//#define CHAT_ID   "349691021"
//  std::string token = "8438508129:AAEaH39kNp5BDXTvC4kqovybLFZxN0M1IlA";
//  std::string chat_id = "349691021";
//  #define BOT_TOKEN  "YOUR_BOT_TOKEN"
//  #define CHAT_ID    "YOUR_CHAT_ID"

// ===================== RTOS ОБЪЕКТЫ =====================

// Бинарный семафор для сигнала от ISR
static SemaphoreHandle_t pirSemaphore;

// Мьютекс для защиты HTTP клиента
static SemaphoreHandle_t httpMutex;

static TaskHandle_t ledTaskHandle = nullptr;

// ===================== ISR PIR =====================

// Обработчик прерывания
static void IRAM_ATTR pir_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // "Отдаём" семафор из ISR. Telegram задача просыпается через семафор
    xSemaphoreGiveFromISR(pirSemaphore, &xHigherPriorityTaskWoken);

    // Уведомление для LED задачи. LED задача просыпается через task notification
    if (ledTaskHandle != nullptr)
    {
        vTaskNotifyGiveFromISR(ledTaskHandle, &xHigherPriorityTaskWoken);
    }

    // Если нужно немедленное переключение контекста
    if (xHigherPriorityTaskWoken == pdTRUE)
        portYIELD_FROM_ISR();
}

// ===================== ОТПРАВКА В TELEGRAM =====================

void send_telegram_message()
{
    char url[512];
    char time_str[64];

    // Получаем текущее время
    time_t now;
    struct tm timeinfo;
    time(&now);

    //UTC время +2 с переходом на летнее
    setenv("TZ", "CET-2CEST,M3.5.0/02:00,M10.5.0/03:00", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d%%20%H%%3A%M%%3A%S", &timeinfo);

    // Формируем URL с текстом: Motion detected! Time: ...
    sprintf(url,
        "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=Motion%%20detected!%%20Time%%3A%%20%s",
        CONFIG_BOT_TOKEN,
        CONFIG_CHAT_ID,
        time_str
    );

    // Захватываем мьютекс (защита сетевого ресурса)
    if (xSemaphoreTake(httpMutex, pdMS_TO_TICKS(5000)) == pdTRUE)
    {
        HttpClient http;
        std::string response = http.get(std::string(url));
        ESP_LOGI(TAG, "Telegram response: %s", response.c_str());
        xSemaphoreGive(httpMutex);
    }
    else
    {
        ESP_LOGE(TAG, "HTTP mutex timeout");
    }
}

// ===================== ЗАДАЧА TELEGRAM =====================

void telegram_task(void* pvParameters)
{
    while (true)
    {
        // Ждём активации PIR (семафор)
        if (xSemaphoreTake(pirSemaphore, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "Motion detected!");

            send_telegram_message();

            // Антиспам 10 секунд
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}

// ===================== Задача управления светодиодом =====================
void ledTask(void* pvParameters)
{
    const TickType_t half_period = pdMS_TO_TICKS(LED_BLINK_PERIOD_MS / 2);
    const TickType_t duration_ticks = pdMS_TO_TICKS(LED_BLINK_DURATION_MS);

    while (true)
    {
        // Ждём первое срабатывание. Приостановка текущей задачи, пока не получит уведомление от другой задачи или прерывания
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t start_time = xTaskGetTickCount();

        while (true)
        {
            // Проверяем: пришло ли новое уведомление?
            if (ulTaskNotifyTake(pdTRUE, 0) > 0)
            {
                // Сброс таймера
                start_time = xTaskGetTickCount();
            }

            // Проверяем истечение 5 секунд
            if ((xTaskGetTickCount() - start_time) >= duration_ticks)
                break;

            gpio_set_level(LED_GPIO, 1);
            vTaskDelay(half_period);

            gpio_set_level(LED_GPIO, 0);
            vTaskDelay(half_period);
        }

        gpio_set_level(LED_GPIO, 0); // гарантированно выключить
    }
}

// ===================== app_main =====================

extern "C" void app_main(void)
{
    // Создаём RTOS объекты
    pirSemaphore  = xSemaphoreCreateBinary(); // 2️⃣ Создание бинарного семафора
    httpMutex     = xSemaphoreCreateMutex();  // 3️⃣ Создание мьютекса
    if (!pirSemaphore || !httpMutex) {
        ESP_LOGE(TAG, "Failed to create RTOS objects");
        return;
    }

    // 4️⃣ Настройка GPIO для PIR
    gpio_config_t pir_conf = {};
    pir_conf.pin_bit_mask = (1ULL << PIR_GPIO);
    pir_conf.mode = GPIO_MODE_INPUT;
    pir_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    pir_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // включаем pull-down, если требуется PIR
    pir_conf.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&pir_conf);

    // 5️⃣ Установка ISR сервиса
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIR_GPIO, pir_isr_handler, NULL);

    // 4️⃣ Настройка GPIO для LED
    gpio_config_t led_conf = {};
    led_conf.pin_bit_mask = (1ULL << LED_GPIO);
    led_conf.mode = GPIO_MODE_OUTPUT;
    led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&led_conf);
    gpio_set_level(LED_GPIO, 0);

    // Инициализация WiFi через вашу библиотеку
    WiFiManager wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
    //  WiFiManager wifi("YOUR_SSID", "YOUR_PASSWORD");

    // 4️⃣ Только после подключения к WiFi создаём Telegram задачу
    if (wifi.connect() == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi connected, starting Telegram task");

        // Настройка NTP для времени
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);          // вместо sntp_setoperatingmode
        esp_sntp_setservername(0, "pool.ntp.org");           // вместо sntp_setservername
        esp_sntp_init(); 

        // Ждём синхронизации времени
        time_t now = 0;
        struct tm timeinfo = {};
        int retry = 0;
        while(timeinfo.tm_year < (2020 - 1900) && retry < 10) {
            time(&now);
            localtime_r(&now, &timeinfo);
            vTaskDelay(pdMS_TO_TICKS(500));
            retry++;
        }

        // 3️⃣ Создание задачи Telegram
        xTaskCreate(
            telegram_task,
            "telegram_task",
            8192,
            NULL,
            5,
            NULL
        );
    }
    else
    {
        ESP_LOGE(TAG, "WiFi connection failed");
    }

    // 3️⃣ Создание задачи LED
    xTaskCreate(
        ledTask, 
        "ledTask", 
        2048, 
        NULL, 
        10, 
        &ledTaskHandle);
}