#include "leds_driver.hpp"
#include "events_id.hpp"

static QueueHandle_t leds_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;
static const std::map<int, int> leds_pin_map = {
    {1, LED1_GPIO_PIN}, {2, LED2_GPIO_PIN}, {3, LED3_GPIO_PIN}, {4, LED4_GPIO_PIN}};
static int report = 0;
static int from_queue = 0;

leds_driver::leds_driver(base_driver *parent) : base_driver() {
  this->init();
  leds_driver_queue_handle = xQueueCreate(LEDS_DRIVER_QUEUE_SIZE, sizeof(int));
}

leds_driver::leds_driver(leds_driver &driver) { *this = driver; }

int leds_driver::init() {
  GPIO_InitTypeDef *GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN | LED4_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LEDS_GPIO_PORT, GPIO_InitStruct);
  delete GPIO_InitStruct;
  return 0;
}

void leds_driver::write(const char *p_params) {
  int led_num = 0;
  char cmd[1024] = {'\0'};
  const char *msg = p_params;

  if (std::sscanf(msg, "led_%u_%s", &led_num, cmd) == 2) {

    if (!std::strcmp(cmd, "set")) {

      report = 0;
      (leds_pin_map.find(led_num) != leds_pin_map.end()) ? GPIO_SetBits(LEDS_GPIO_PORT, leds_pin_map.at(led_num))
                                                         : (void)0;

    } else if (!std::strcmp(cmd, "reset")) {

      report = 0;
      (leds_pin_map.find(led_num) != leds_pin_map.end()) ? GPIO_ResetBits(LEDS_GPIO_PORT, leds_pin_map.at(led_num))
                                                         : (void)0;

    } else if (!std::strcmp(cmd, "read_state")) {

      int led_state = (leds_pin_map.find(led_num) != leds_pin_map.end())
                          ? GPIO_ReadInputDataBit(LEDS_GPIO_PORT, leds_pin_map.at(led_num))
                          : 0;
      report = led_state;
      xQueueSendToBack(leds_driver_queue_handle, &report, 0);
    }
  }
}

void *leds_driver::read(TickType_t timeout) const {
  if (xQueueReceive(leds_driver_queue_handle, &from_queue, timeout))
    return static_cast<void *>(&from_queue);
  else
    return nullptr;
}

int leds_driver::deinit() {
  GPIO_InitTypeDef *GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = LED1_GPIO_PIN | LED2_GPIO_PIN | LED3_GPIO_PIN | LED4_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(LEDS_GPIO_PORT, GPIO_InitStruct);
  delete GPIO_InitStruct;
  return 0;
}
