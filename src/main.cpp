// ======== ОТПРАВКА СООБЩЕНИЯ В ТЕЛЕГРАМ ПРИ СРАБАТЫВАНИИ PIR ДАТЧИКА ========
// ===================== ПОДКЛЮЧЕНИЯ =====================

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "esp_log.h"

// ===== ГОТОВЫЕ БИБЛИОТЕКИ =====

#include "WifiManager/WifiManager.hpp"
#include "HttpClient/HttpClient.hpp"

// ===================== НАСТРОЙКИ =====================

#define PIR_GPIO GPIO_NUM_4

#define BOT_TOKEN "8438508129:AAEaH39kNp5BDXTvC4kqovybLFZxN0M1IlA"
#define CHAT_ID   "349691021"
// //     std::string token = "8438508129:AAEaH39kNp5BDXTvC4kqovybLFZxN0M1IlA";
// //     std::string chat_id = "349691021";

// ===================== RTOS ОБЪЕКТЫ =====================

// Бинарный семафор для сигнала от ISR
static SemaphoreHandle_t pirSemaphore;

// Мьютекс для защиты HTTP клиента
static SemaphoreHandle_t httpMutex;

// ===================== ISR PIR =====================

// Обработчик прерывания
static void IRAM_ATTR pir_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // "Отдаём" семафор из ISR
    xSemaphoreGiveFromISR(pirSemaphore, &xHigherPriorityTaskWoken);

    // Если нужно немедленное переключение контекста
    if (xHigherPriorityTaskWoken == pdTRUE)
        portYIELD_FROM_ISR();
}

// ===================== ОТПРАВКА В TELEGRAM =====================

void send_telegram_message()
{
    char url[512];

    // Формируем GET запрос к Telegram API
    sprintf(url,
            "https://api.telegram.org/bot%s/sendMessage?chat_id=%s&text=Motion%%20detected!",
            BOT_TOKEN,
            CHAT_ID);

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

// ===================== app_main =====================

extern "C" void app_main(void)
{
    // Создаём RTOS объекты
    pirSemaphore  = xSemaphoreCreateBinary(); // 2️⃣ Создание бинарного семафора
    httpMutex     = xSemaphoreCreateMutex();  // 3️⃣ Создание мьютекса
    
    // 4️⃣ Настройка GPIO для PIR
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIR_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;  // Прерывание по фронту
    gpio_config(&io_conf);

    // 5️⃣ Установка ISR сервиса
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIR_GPIO, pir_isr_handler, NULL);

    // Инициализация WiFi через вашу библиотеку
    WiFiManager wifi("Kryptos", "13572468");
    
    // 4️⃣ Только после подключения к WiFi создаём Telegram задачу
    if (wifi.connect() == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi connected, starting Telegram task");

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
}