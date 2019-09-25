#include "gp_timers_driver.hpp"
#include "events_id.hpp"

static SemaphoreHandle_t gp_timers_driver_semaphore_handle;
static QueueHandle_t gp_timers_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;

static const std::map<unsigned int, TIM_TypeDef *> tim_periph_map = {
    {0, TIM0_PERIPH}, {1, TIM1_PERIPH}, {2, TIM2_PERIPH}, {3, TIM3_PERIPH}};
static int report = 0;
static int from_queue = 0;

gp_timers_driver::gp_timers_driver(base_driver *parent) : base_driver() {
  this->init();
  gp_timers_driver_semaphore_handle = xSemaphoreCreateBinary();
  gp_timers_driver_queue_handle = xQueueCreate(GP_TIMERS_DRIVER_QUEUE_SIZE, sizeof(int));
}

gp_timers_driver::gp_timers_driver(gp_timers_driver &driver) { *this = driver; }

int gp_timers_driver::init() {
  TIM_TimeBaseInitTypeDef *TIM_TimeBaseInitStruct = new TIM_TimeBaseInitTypeDef;

  TIM_TimeBaseStructInit(TIM_TimeBaseInitStruct);
  TIM_TimeBaseInitStruct->TIM_ClockDivision = 1;
  TIM_TimeBaseInitStruct->TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStruct->TIM_Period = 8192;
  TIM_TimeBaseInitStruct->TIM_Prescaler = 1000;
  TIM_TimeBaseInitStruct->TIM_RepetitionCounter = ENABLE;
  TIM_TimeBaseInit(TIM0_PERIPH, TIM_TimeBaseInitStruct);
  TIM_ITConfig(TIM0_PERIPH, TIM_IT_Update, ENABLE);

  TIM_TimeBaseStructInit(TIM_TimeBaseInitStruct);
  TIM_TimeBaseInitStruct->TIM_ClockDivision = 1;
  TIM_TimeBaseInitStruct->TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStruct->TIM_Period = 8192;
  TIM_TimeBaseInitStruct->TIM_Prescaler = 1000;
  TIM_TimeBaseInitStruct->TIM_RepetitionCounter = ENABLE;
  TIM_TimeBaseInit(TIM1_PERIPH, TIM_TimeBaseInitStruct);
  TIM_ITConfig(TIM1_PERIPH, TIM_IT_Update, ENABLE);

  TIM_TimeBaseStructInit(TIM_TimeBaseInitStruct);
  TIM_TimeBaseInitStruct->TIM_ClockDivision = 1;
  TIM_TimeBaseInitStruct->TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStruct->TIM_Period = 8192;
  TIM_TimeBaseInitStruct->TIM_Prescaler = 1000;
  TIM_TimeBaseInitStruct->TIM_RepetitionCounter = ENABLE;
  TIM_TimeBaseInit(TIM2_PERIPH, TIM_TimeBaseInitStruct);
  TIM_ITConfig(TIM2_PERIPH, TIM_IT_Update, ENABLE);

  TIM_TimeBaseStructInit(TIM_TimeBaseInitStruct);
  TIM_TimeBaseInitStruct->TIM_ClockDivision = 1;
  TIM_TimeBaseInitStruct->TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInitStruct->TIM_Period = 8192;
  TIM_TimeBaseInitStruct->TIM_Prescaler = 1000;
  TIM_TimeBaseInitStruct->TIM_RepetitionCounter = ENABLE;
  TIM_TimeBaseInit(TIM3_PERIPH, TIM_TimeBaseInitStruct);
  TIM_ITConfig(TIM3_PERIPH, TIM_IT_Update, ENABLE);

  delete TIM_TimeBaseInitStruct;
  NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 12);
  NVIC_SetPriority(TIM2_IRQn, 12);
  NVIC_SetPriority(TIM3_IRQn, 12);
  NVIC_SetPriority(TIM4_IRQn, 12);
  NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  NVIC_EnableIRQ(TIM2_IRQn);
  NVIC_EnableIRQ(TIM3_IRQn);
  NVIC_EnableIRQ(TIM4_IRQn);
  return 0;
}

void gp_timers_driver::write(const char *p_params) {
  int tim_number = 0;
  char cmd[1024] = {'\0'};
  const char *msg = p_params;

  if (std::sscanf(msg, "timer_%u_%s", &tim_number, cmd) == 2) {

    if (!std::strcmp(cmd, "start")) {

      report = 0;
      if (tim_periph_map.find(tim_number) != tim_periph_map.end())
        TIM_Cmd(tim_periph_map.at(tim_number), ENABLE);

    } else if (!std::strcmp(cmd, "stop")) {

      report = 0;
      if (tim_periph_map.find(tim_number) != tim_periph_map.end())
        TIM_Cmd(tim_periph_map.at(tim_number), DISABLE);

    } else if (!std::strcmp(cmd, "reset")) {

      report = 0;
      if (tim_periph_map.find(tim_number) != tim_periph_map.end())
        TIM_SetCounter(tim_periph_map.at(tim_number), 0x0000);
    }
  }
}

void *gp_timers_driver::read(TickType_t timeout) const {
  if (xQueueReceive(gp_timers_driver_queue_handle, &from_queue, timeout))
    return static_cast<void *>(&from_queue);
  else
    return nullptr;
}

int gp_timers_driver::deinit() {
  TIM_DeInit(TIM0_PERIPH);
  TIM_DeInit(TIM1_PERIPH);
  TIM_DeInit(TIM2_PERIPH);
  TIM_DeInit(TIM3_PERIPH);
  NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
  NVIC_DisableIRQ(TIM2_IRQn);
  NVIC_DisableIRQ(TIM3_IRQn);
  NVIC_DisableIRQ(TIM4_IRQn);
  return 0;
}

extern "C" {

void TIM1_UP_TIM10_IRQHandler() {
  if (TIM_GetITStatus(TIM0_PERIPH, TIM_IT_Update)) {

    TIM_ClearITPendingBit(TIM0_PERIPH, TIM_IT_Update);
    portBASE_TYPE gp_timers_driver_task_irq_woken;
    int event_id = TIMER_0_EVENT_ID;
    xQueueSendFromISR(events_worker_queue_handle, &event_id, &gp_timers_driver_task_irq_woken);
    if (gp_timers_driver_task_irq_woken == pdTRUE)
      taskYIELD();

  } else
    TIM_ClearITPendingBit(TIM0_PERIPH, TIM_IT_Break | TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM |
                                           TIM_IT_Trigger | TIM_IT_Update);
}

void TIM2_IRQHandler() {
  if (TIM_GetITStatus(TIM1_PERIPH, TIM_IT_Update)) {

    TIM_ClearITPendingBit(TIM1_PERIPH, TIM_IT_Update);
    portBASE_TYPE gp_timers_driver_task_irq_woken;
    int event_id = TIMER_1_EVENT_ID;
    xQueueSendFromISR(events_worker_queue_handle, &event_id, &gp_timers_driver_task_irq_woken);
    if (gp_timers_driver_task_irq_woken == pdTRUE)
      taskYIELD();

  } else
    TIM_ClearITPendingBit(TIM1_PERIPH, TIM_IT_Break | TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM |
                                           TIM_IT_Trigger | TIM_IT_Update);
}

void TIM3_IRQHandler() {
  if (TIM_GetITStatus(TIM2_PERIPH, TIM_IT_Update)) {

    TIM_ClearITPendingBit(TIM2_PERIPH, TIM_IT_Update);
    portBASE_TYPE gp_timers_driver_task_irq_woken;
    int event_id = TIMER_2_EVENT_ID;
    xQueueSendFromISR(events_worker_queue_handle, &event_id, &gp_timers_driver_task_irq_woken);
    if (gp_timers_driver_task_irq_woken == pdTRUE)
      taskYIELD();

  } else
    TIM_ClearITPendingBit(TIM2_PERIPH, TIM_IT_Break | TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM |
                                           TIM_IT_Trigger | TIM_IT_Update);
}

void TIM4_IRQHandler() {
  if (TIM_GetITStatus(TIM3_PERIPH, TIM_IT_Update)) {

    TIM_ClearITPendingBit(TIM3_PERIPH, TIM_IT_Update);
    portBASE_TYPE gp_timers_driver_task_irq_woken;
    int event_id = TIMER_3_EVENT_ID;
    xQueueSendFromISR(events_worker_queue_handle, &event_id, &gp_timers_driver_task_irq_woken);
    if (gp_timers_driver_task_irq_woken == pdTRUE)
      taskYIELD();

  } else
    TIM_ClearITPendingBit(TIM3_PERIPH, TIM_IT_Break | TIM_IT_CC1 | TIM_IT_CC2 | TIM_IT_CC3 | TIM_IT_CC4 | TIM_IT_COM |
                                           TIM_IT_Trigger | TIM_IT_Update);
}
}
