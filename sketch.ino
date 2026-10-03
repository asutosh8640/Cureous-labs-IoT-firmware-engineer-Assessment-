#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define BUTTON_1_PIN     12
#define BUTTON_2_PIN     14
#define POT_ADC_PIN      34

#define MOVING_AVG_WINDOW 10

typedef struct {
  uint8_t button_id;
  unsigned long timestamp;
} ButtonEvent_t;

QueueHandle_t buttonQueue;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

uint16_t adc_buffer[MOVING_AVG_WINDOW] = {0};
uint8_t buffer_index = 0;

uint16_t apply_moving_average(uint16_t raw_sample) {
  adc_buffer[buffer_index] = raw_sample;
  buffer_index = (buffer_index + 1) % MOVING_AVG_WINDOW;

  uint32_t sum = 0;
  for (uint8_t i = 0; i < MOVING_AVG_WINDOW; i++) {
    sum += adc_buffer[i];
  }
  return (uint16_t)(sum / MOVING_AVG_WINDOW);
}

void IRAM_ATTR isr_button1() {
  ButtonEvent_t event = {1, millis()};
  xQueueSendFromISR(buttonQueue, &event, NULL);
}

void IRAM_ATTR isr_button2() {
  ButtonEvent_t event = {2, millis()};
  xQueueSendFromISR(buttonQueue, &event, NULL);
}

void TaskTelemetry(void *pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) {
        client.connect("CureousLabs_IoT_Device");
      }
      
      StaticJsonDocument<128> doc;
      doc["uptime_ms"] = millis();
      doc["status"] = "HEALTHY";
      doc["free_heap"] = ESP.getFreeHeap();

      char buffer[128];
      serializeJson(doc, buffer);
      
      client.publish("cureous/telemetry/heartbeat", buffer);
      Serial.print("[TASK 1 - MQTT Heartbeat Sent]: ");
      Serial.println(buffer);
    } else {
      WiFi.reconnect();
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void TaskSensorSampling(void *pvParameters) {
  for (;;) {
    uint16_t raw_val = analogRead(POT_ADC_PIN);
    uint16_t filtered_val = apply_moving_average(raw_val);

    StaticJsonDocument<128> doc;
    doc["adc_raw"] = raw_val;
    doc["adc_filtered"] = filtered_val;

    char buffer[128];
    serializeJson(doc, buffer);
    Serial.print("[TASK 2 - ADC Stream 10Hz]: ");
    Serial.println(buffer);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void TaskUARTLogger(void *pvParameters) {
  ButtonEvent_t rx_event;
  for (;;) {
    if (xQueueReceive(buttonQueue, &rx_event, portMAX_DELAY) == pdTRUE) {
      StaticJsonDocument<128> doc;
      doc["button_id"] = rx_event.button_id;
      doc["timestamp"] = rx_event.timestamp;

      char buffer[128];
      serializeJson(doc, buffer);
      Serial.print("[TASK 3 - Event Logged]: ");
      Serial.println(buffer);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_1_PIN), isr_button1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_2_PIN), isr_button2, FALLING);

  buttonQueue = xQueueCreate(10, sizeof(ButtonEvent_t));

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  client.setServer(mqtt_server, 1883);

  xTaskCreate(TaskTelemetry, "TaskTelemetry", 4096, NULL, 2, NULL);
  xTaskCreate(TaskSensorSampling, "TaskSensorSampling", 2048, NULL, 1, NULL);
  xTaskCreate(TaskUARTLogger, "TaskUARTLogger", 2048, NULL, 3, NULL);
}

void loop() {
  vTaskDelete(NULL);
}
