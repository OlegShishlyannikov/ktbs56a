#include "fpga_driver.hpp"
#include "events_id.hpp"

static QueueHandle_t fpga_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;
static int report = 0;

fpga_driver::fpga_driver(base_driver *p_parent) : base_driver() {
  this->init();
  GPIO_SetBits(FPGA_GPIO_PORT, FPGA_GPIO_M0_PIN | FPGA_GPIO_M1_PIN | FPGA_GPIO_VS0_PIN | FPGA_GPIO_VS1_PIN |
                                   FPGA_GPIO_VS2_PIN | FPGA_GPIO_NRST_PIN);

  while (!GPIO_ReadInputDataBit(FPGA_GPIO_PORT, FPGA_GPIO_DONE_PIN))
    ;

  fpga_driver_queue_handle = xQueueCreate(FPGA_DRIVER_QUEUE_SIZE, sizeof(int));
}

fpga_driver::fpga_driver(fpga_driver &driver) { *this = driver; }

void *fpga_driver::read(TickType_t timeout) const {
  if (xQueueReceive(fpga_driver_queue_handle, &report, timeout))
    return static_cast<void *>(&report);
  else
    return nullptr;
}

void fpga_driver::write(const char *p_params) {
  const char *msg = p_params;
  int adc_num = 0;
  int rled_num = 0;
  int gled_num = 0;
  char cmd[1024] = {'\0'};

  if (std::sscanf(msg, "%u_red_led_%s", &rled_num, cmd) == 2) {

    if (!std::strcmp(cmd, "on")) {

      int report = this->red_led_on(rled_num);

    } else if (!std::strcmp(cmd, "off")) {

      int report = this->red_led_off(rled_num);
    }
  } else if (std::sscanf(msg, "%u_green_led_%s", &gled_num, cmd) == 2) {

    if (!std::strcmp(cmd, "on")) {

      int report = this->green_led_on(gled_num);

    } else if (!std::strcmp(cmd, "off")) {

      int report = this->green_led_off(gled_num);
    }
  } else if (std::sscanf(msg, "%u_adc_%s", &adc_num, cmd) == 2) {

    if (!std::strcmp(cmd, "select")) {

      int report = this->adc_select(adc_num);

    } else if (!std::strcmp(cmd, "deselect")) {

      int report = this->adc_deselect(adc_num);
    }
  } else if (std::sscanf(msg, "%s", cmd) == 1) {

    if (!std::strcmp(cmd, "get_conf_status")) {

      int report = this->get_conf_status();
      xQueueSendToBack(fpga_driver_queue_handle, &report, 0);

    } else if (!std::strcmp(cmd, "reset_all")) {

      int report = this->reset_all();

    } else if (!std::strcmp(cmd, "deselect_all")) {

      int report = this->adc_deselect_all();
    }
  }
}

int fpga_driver::init() {
  GPIO_InitTypeDef *GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_VS0_PIN | FPGA_GPIO_VS1_PIN | FPGA_GPIO_VS2_PIN | FPGA_GPIO_M0_PIN |
                              FPGA_GPIO_M1_PIN | FPGA_GPIO_M2_PIN | FPGA_GPIO_NCS_PIN | FPGA_GPIO_NRST_PIN |
                              FPGA_GPIO_EN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_DONE_PIN | FPGA_GPIO_PROG_B_PIN | FPGA_GPIO_SUSPEND_PIN | FPGA_GPIO_DIN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_DOUT_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_DIN_PIN | FPGA_GPIO_CLK_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);
  delete GPIO_InitStruct;

  SPI_InitTypeDef *SPI_InitStruct = new SPI_InitTypeDef;
  SPI_StructInit(SPI_InitStruct);
  SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
  SPI_InitStruct->SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStruct->SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStruct->SPI_DataSize = SPI_DataSize_16b;
  SPI_InitStruct->SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStruct->SPI_Mode = SPI_Mode_Master;
  SPI_InitStruct->SPI_NSS = SPI_NSS_Soft;
  SPI_Init(FPGA_SPI_PERIPH, SPI_InitStruct);
  SPI_Cmd(FPGA_SPI_PERIPH, ENABLE);
  delete SPI_InitStruct;
  return 0;
}

int fpga_driver::green_led_on(unsigned int led_num) { return fpga_driver::send_packet(GREEN_LED_ON_COMMAND, led_num); }

int fpga_driver::green_led_off(unsigned int led_num) {
  return fpga_driver::send_packet(GREEN_LED_OFF_COMMAND, led_num);
}

int fpga_driver::red_led_on(unsigned int led_num) { return fpga_driver::send_packet(RED_LED_ON_COMMAND, led_num); }

int fpga_driver::red_led_off(unsigned int led_num) { return fpga_driver::send_packet(RED_LED_OFF_COMMAND, led_num); }

int fpga_driver::adc_select(unsigned int adc_num) { return fpga_driver::send_packet(ADC_SELECT_COMMAND, adc_num); }

int fpga_driver::adc_deselect(unsigned int adc_num) { return fpga_driver::send_packet(ADC_DESELECT_COMMAND, adc_num); }

int fpga_driver::adc_deselect_all() { return fpga_driver::send_packet(ADC_DESELECT_COMMAND, 0); }

int fpga_driver::reset_all() { return fpga_driver::send_packet(RESET_ALL_COMMAND, 0); }

int fpga_driver::get_conf_status() { return GPIO_ReadInputDataBit(FPGA_GPIO_PORT, FPGA_GPIO_DONE_PIN); }

int fpga_driver::send_packet(unsigned int command, unsigned int data) {
  GPIO_ResetBits(FPGA_GPIO_PORT, FPGA_GPIO_NCS_PIN);

  while (SPI_I2S_GetFlagStatus(FPGA_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;

  SPI_I2S_SendData(FPGA_SPI_PERIPH, (command << 8) | (data));

  while (SPI_I2S_GetFlagStatus(FPGA_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;

  GPIO_SetBits(FPGA_GPIO_PORT, FPGA_GPIO_NCS_PIN);
  GPIO_SetBits(FPGA_GPIO_PORT, FPGA_GPIO_EN_PIN);
  GPIO_ResetBits(FPGA_GPIO_PORT, FPGA_GPIO_EN_PIN);
  return 0;
}

int fpga_driver::deinit() {
  GPIO_InitTypeDef *GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_VS0_PIN | FPGA_GPIO_VS1_PIN | FPGA_GPIO_VS2_PIN | FPGA_GPIO_M0_PIN |
                              FPGA_GPIO_M1_PIN | FPGA_GPIO_M2_PIN | FPGA_GPIO_NCS_PIN | FPGA_GPIO_NRST_PIN |
                              FPGA_GPIO_EN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_DONE_PIN | FPGA_GPIO_PROG_B_PIN | FPGA_GPIO_SUSPEND_PIN | FPGA_GPIO_DIN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_DIN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = FPGA_GPIO_DOUT_PIN | FPGA_GPIO_CLK_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(FPGA_GPIO_PORT, GPIO_InitStruct);
  delete GPIO_InitStruct;

  SPI_I2S_DeInit(FPGA_SPI_PERIPH);
  return 0;
}
