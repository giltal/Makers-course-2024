#include "Arduino.h"

// Semaphore handle
SemaphoreHandle_t mySemaphore;

// Task 1 - Tries to access the shared resource
void Task1(void* pvParameters) {
    while (1) {
        if (xSemaphoreTake(mySemaphore, portMAX_DELAY)) { // Take semaphore
            printf("Task 1 is using the resource...\n");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Simulate work
            printf("Task 1 done.\n");
            xSemaphoreGive(mySemaphore);  // Release semaphore
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Wait before retrying
    }
}

// Task 2 - Also tries to access the shared resource
void Task2(void* pvParameters) {
    while (1) {
        if (xSemaphoreTake(mySemaphore, portMAX_DELAY)) { // Take semaphore
            printf("Task 2 is using the resource...\n");
            vTaskDelay(pdMS_TO_TICKS(1000));  // Simulate work
            printf("Task 2 done.\n");
            xSemaphoreGive(mySemaphore);  // Release semaphore
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Wait before retrying
    }
}

void setup() {
    Serial.begin(115200);

    // Create binary semaphore
    mySemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(mySemaphore); // Give initial access

    // Create tasks
    xTaskCreate(Task1, "Task 1", 2048, NULL, 1, NULL);
    xTaskCreate(Task2, "Task 2", 2048, NULL, 1, NULL);
}

void loop() {
    // Empty, tasks run in FreeRTOS
}