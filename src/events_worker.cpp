#include "events_worker.hpp"
#include "events_id.hpp"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

QueueHandle_t events_worker_queue_handle;
static std::map<int, signal<void *> *> event_signals_map;
static TaskHandle_t events_worker_task_handle;

#define REGISTER_EVENT_SIGNAL(x) event_signals_map.insert(std::make_pair(x, new signal<void *>));

events_worker::events_worker() {
  events_worker_queue_handle = xQueueCreate(EVENTS_WORKER_QUEUE_SIZE, sizeof(int));
  xTaskCreate(events_worker_task_code_wrapper, events_worker::get_name().c_str(), configMINIMAL_STACK_SIZE / 2, nullptr,
              configAPP_PRIORITY, &events_worker_task_handle);
}

std::map<int, signal<void *> *> *events_worker::get_event_signals_map() { return &event_signals_map; }

void events_worker::events_worker_task_code(void *parameters) {
  REGISTER_EVENT_SIGNAL(USART_CHAR_RECEIVED_EVENT_ID);
  REGISTER_EVENT_SIGNAL(BUTTON_MINUS_PRESS_EVENT_ID);
  REGISTER_EVENT_SIGNAL(BUTTON_PLUS_PRESS_EVENT_ID);
  REGISTER_EVENT_SIGNAL(BUTTON_RUN_PRESS_EVENT_ID);
  REGISTER_EVENT_SIGNAL(TIMER_0_EVENT_ID);
  REGISTER_EVENT_SIGNAL(TIMER_1_EVENT_ID);
  REGISTER_EVENT_SIGNAL(TIMER_2_EVENT_ID);
  REGISTER_EVENT_SIGNAL(TIMER_3_EVENT_ID);

  while (true) {

    int event_id = 0;
    xQueueReceive(events_worker_queue_handle, &event_id, portMAX_DELAY);

    if (event_signals_map.find(event_id) != event_signals_map.end())
      event_signals_map.at(event_id)->emit(nullptr);
  }

  vTaskDelete(nullptr);
}
