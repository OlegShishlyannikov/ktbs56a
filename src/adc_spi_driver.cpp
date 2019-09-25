#include "adc_spi_driver.hpp"
#include "events_id.hpp"

static QueueHandle_t adc_spi_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;
decltype(adc_spi_driver::ADS1118_ConfRegInitStruct) adc_spi_driver::ADS1118_ConfRegInitStruct;
static const double vref = 2.048000f;
static const double divider = 21.0f;
static const double max_adc_val = 32767.0f;
static const double celsius_val = 0.03125f;
static double measured = 0.0f;
static size_t report = 0;
static double from_queue = 0.0f;
static size_t table_offset = 0x08060000;
static const unsigned int page_size = 2048;

adc_spi_driver::adc_spi_driver(base_driver *parent) : base_driver() {
  this->init();
  adc_spi_driver_queue_handle = xQueueCreate(ADC_SPI_DRIVER_QUEUE_SIZE, sizeof(double));
}

adc_spi_driver::adc_spi_driver(adc_spi_driver &driver) { *this = driver; }

int adc_spi_driver::init() {
  GPIO_InitTypeDef *GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = ADC_SPI_SCK_PIN | ADC_SPI_MOSI_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ADC_SPI_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = ADC_SPI_MISO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ADC_SPI_GPIO_PORT, GPIO_InitStruct);
  delete GPIO_InitStruct;

  SPI_InitTypeDef *SPI_InitStruct = new SPI_InitTypeDef;
  SPI_StructInit(SPI_InitStruct);
  SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
  SPI_InitStruct->SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStruct->SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStruct->SPI_DataSize = SPI_DataSize_16b;
  SPI_InitStruct->SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStruct->SPI_Mode = SPI_Mode_Master;
  SPI_InitStruct->SPI_NSS = SPI_NSS_Soft;
  SPI_Init(ADC_SPI_PERIPH, SPI_InitStruct);
  SPI_Cmd(ADC_SPI_PERIPH, ENABLE);
  delete SPI_InitStruct;
  this->flash_unlock();
  return 0;
}

void adc_spi_driver::write(const char *p_params) {
  unsigned int adc_channel = 0;
  unsigned int mode = 0;
  double *table_address = nullptr;
  unsigned int table_size = 0;
  const char *msg = p_params;
  char cmd[1024] = {'\0'};

  if (std::sscanf(msg, "ch_%u_%u_%s", &adc_channel, &mode, cmd) == 3) {

    if (!std::strcmp(cmd, "get_data")) {

      report = this->get_data(adc_channel, mode);

    } else if (!std::strcmp(cmd, "get_temp")) {

      report = this->get_temp(adc_channel, mode);
    }
  } else if (std::sscanf(msg, "write_calibration_table_%p_%u", &table_address, &table_size) == 2) {

    this->write_calibration_table(table_address, table_size);

  } else if (std::sscanf(msg, "read_calibration_table_size_%u", &table_size) == 1) {

    this->read_calibration_table(table_size);
  }
}

void *adc_spi_driver::read(TickType_t timeout) const {
  if (xQueueReceive(adc_spi_driver_queue_handle, &from_queue, timeout))
    return static_cast<void *>(&from_queue);
  return nullptr;
}

int adc_spi_driver::get_data(unsigned int adc_channel, unsigned int mode) {
  adc_spi_driver::ADS1118_ConfRegInitStruct.OS = ADC_SS_START_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.PGA = ADC_PGA_FSR_2048_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.MODE = ADC_OPMODE_SS_PWRDWN_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.DR = (mode == 0) ? ADC_DR_8_MODE : ADC_DR_8_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.TS_MODE = ADC_TSMODE_ADC_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.PULL_UP_EN = ADC_PULLUP_EN_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.NOP = ADC_NOP_VALID_DATA_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.RDY_FLAG = ADC_RDY_FLAG_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.MUX = ((adc_channel % 2) == 0) ? ADC_MUX_0P_1N_MODE : ADC_MUX_2P_3N_MODE;
  std::bitset<16> data_packet(
      (adc_spi_driver::ADS1118_ConfRegInitStruct.OS.to_ulong() << ADS1118_SS_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.MUX.to_ulong() << ADS1118_MUX_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.PGA.to_ulong() << ADS1118_PGA_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.MODE.to_ulong() << ADS1118_OPMODE_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.DR.to_ulong() << ADS1118_DR_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.TS_MODE.to_ulong() << ADS1118_TSMODE_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.PULL_UP_EN.to_ulong() << ADS1118_PULLUPEN_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.NOP.to_ulong() << ADS1118_NOP_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.RDY_FLAG.to_ulong() << ADS1118_RDY_FLAG_POS));

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  short temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);

  int timeout = 1000;

  while (GPIO_ReadInputDataBit(ADC_SPI_GPIO_PORT, ADC_SPI_MISO_PIN)) {

    vTaskDelay(1);
    timeout--;

    if (timeout == 0) {

      return 1;
    }
  }

  adc_spi_driver::ADS1118_ConfRegInitStruct.OS = ADC_SS_RESET_MODE;
  data_packet =
      std::bitset<16>((adc_spi_driver::ADS1118_ConfRegInitStruct.OS.to_ulong() << ADS1118_SS_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.MUX.to_ulong() << ADS1118_MUX_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.PGA.to_ulong() << ADS1118_PGA_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.MODE.to_ulong() << ADS1118_OPMODE_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.DR.to_ulong() << ADS1118_DR_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.TS_MODE.to_ulong() << ADS1118_TSMODE_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.PULL_UP_EN.to_ulong() << ADS1118_PULLUPEN_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.NOP.to_ulong() << ADS1118_NOP_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.RDY_FLAG.to_ulong() << ADS1118_RDY_FLAG_POS));

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);
  short data = temp;

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);

  measured = static_cast<double>(data / max_adc_val) * vref * divider;
  xQueueSendToBack(adc_spi_driver_queue_handle, &measured, 0);
  return 0;
}

int adc_spi_driver::get_temp(unsigned int adc_channel, unsigned int mode) {
  adc_spi_driver::ADS1118_ConfRegInitStruct.OS = ADC_SS_START_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.PGA = ADC_PGA_FSR_2048_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.MODE = ADC_OPMODE_SS_PWRDWN_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.DR = (mode == 0) ? ADC_DR_8_MODE : ADC_DR_8_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.TS_MODE = ADC_TSMODE_TEMP_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.PULL_UP_EN = ADC_PULLUP_EN_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.NOP = ADC_NOP_VALID_DATA_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.RDY_FLAG = ADC_RDY_FLAG_MODE;
  adc_spi_driver::ADS1118_ConfRegInitStruct.MUX = ((adc_channel % 2) == 0) ? ADC_MUX_0P_1N_MODE : ADC_MUX_2P_3N_MODE;
  std::bitset<16> data_packet(
      (adc_spi_driver::ADS1118_ConfRegInitStruct.OS.to_ulong() << ADS1118_SS_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.MUX.to_ulong() << ADS1118_MUX_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.PGA.to_ulong() << ADS1118_PGA_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.MODE.to_ulong() << ADS1118_OPMODE_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.DR.to_ulong() << ADS1118_DR_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.TS_MODE.to_ulong() << ADS1118_TSMODE_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.PULL_UP_EN.to_ulong() << ADS1118_PULLUPEN_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.NOP.to_ulong() << ADS1118_NOP_POS) |
      (adc_spi_driver::ADS1118_ConfRegInitStruct.RDY_FLAG.to_ulong() << ADS1118_RDY_FLAG_POS));

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  short temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);

  int timeout = 1000;

  while (GPIO_ReadInputDataBit(ADC_SPI_GPIO_PORT, ADC_SPI_MISO_PIN)) {

    vTaskDelay(1);
    timeout--;

    if (timeout == 0) {

      return 1;
    }
  }

  adc_spi_driver::ADS1118_ConfRegInitStruct.OS = ADC_SS_RESET_MODE;
  data_packet =
      std::bitset<16>((adc_spi_driver::ADS1118_ConfRegInitStruct.OS.to_ulong() << ADS1118_SS_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.MUX.to_ulong() << ADS1118_MUX_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.PGA.to_ulong() << ADS1118_PGA_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.MODE.to_ulong() << ADS1118_OPMODE_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.DR.to_ulong() << ADS1118_DR_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.TS_MODE.to_ulong() << ADS1118_TSMODE_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.PULL_UP_EN.to_ulong() << ADS1118_PULLUPEN_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.NOP.to_ulong() << ADS1118_NOP_POS) |
                      (adc_spi_driver::ADS1118_ConfRegInitStruct.RDY_FLAG.to_ulong() << ADS1118_RDY_FLAG_POS));

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);
  short data = temp;

  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  SPI_I2S_SendData(ADC_SPI_PERIPH, data_packet.to_ulong());
  while (SPI_I2S_GetFlagStatus(ADC_SPI_PERIPH, SPI_I2S_FLAG_BSY))
    ;
  temp = SPI_I2S_ReceiveData(ADC_SPI_PERIPH);

  ((data << 2) >= 0) ? measured = static_cast<double>(data / 4) *celsius_val
                     : measured = static_cast<double>(data / 4) * celsius_val * (-1);
  xQueueSendToBack(adc_spi_driver_queue_handle, &measured, 0);
  return 0;
}

int adc_spi_driver::write_calibration_table(double *table_address, unsigned int table_size) {
  if (table_address != nullptr) {

    this->flash_erase_page(table_offset - page_size);
    this->flash_erase_page(table_offset);
    this->flash_erase_page(table_offset + page_size);

    for (unsigned int i = 0; i < table_size; i++) {

      this->flash_write_double(table_offset + i * sizeof(table_address[i]), table_address[i]);
    }

    return 0;

  } else
    return 1;
}

int adc_spi_driver::read_calibration_table(unsigned int table_size) {
  static double temp[DUTS_NUMBER * 5] = {0.0f};

  for (unsigned int i = 0; i < table_size; i++) {

    unsigned int msb = *reinterpret_cast<unsigned int *>(static_cast<size_t>(table_offset + i * sizeof(double)));
    unsigned int lsb = *reinterpret_cast<unsigned int *>(
        static_cast<size_t>(table_offset + sizeof(unsigned int) + i * sizeof(double)));
    unsigned long long int dat = static_cast<unsigned long long int>(
        ((std::bitset<64>(msb) << (sizeof(double) * 8 / 2)) | std::bitset<64>(lsb)).to_ullong());
    temp[i] = *reinterpret_cast<double *>(&dat);
  }

  static double pp_temp = static_cast<double>(reinterpret_cast<size_t>(&temp));
  xQueueSendToBack(adc_spi_driver_queue_handle, &pp_temp, 0);
  return 0;
}

void adc_spi_driver::flash_unlock() {
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
}

void adc_spi_driver::flash_lock() { FLASH->CR |= FLASH_CR_LOCK; }

bool adc_spi_driver::flash_ready() { return !(FLASH->SR & FLASH_SR_BSY); }

int adc_spi_driver::flash_erase_all_pages() {
  FLASH->CR |= FLASH_CR_MER;
  FLASH->CR |= FLASH_CR_STRT;

  while (!flash_ready())
    ;

  FLASH->CR &= FLASH_CR_MER;
  return 0;
}

int adc_spi_driver::flash_erase_page(uint32_t address) {
  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = address;
  FLASH->CR |= FLASH_CR_STRT;

  while (!flash_ready())
    ;

  FLASH->CR &= ~FLASH_CR_PER;
  return 0;
}

int adc_spi_driver::flash_write_word(unsigned int address, unsigned int data) {
  FLASH->CR |= FLASH_CR_PG;

  while (!flash_ready())
    ;

  *reinterpret_cast<unsigned short *>(static_cast<size_t>(address)) = static_cast<unsigned short>(data);

  while (!flash_ready())
    ;

  address += 2;
  data >>= 16;
  *reinterpret_cast<unsigned short *>(static_cast<size_t>(address)) = static_cast<unsigned short>(data);

  while (!flash_ready())
    ;

  FLASH->CR &= ~(FLASH_CR_PG);
  return 0;
}

int adc_spi_driver::flash_write_double(uint32_t address, double data) {
  unsigned long long int data_binary = *reinterpret_cast<unsigned long long int *>(&data);
  unsigned int data_binary_lsb = *reinterpret_cast<unsigned int *>(&data_binary);
  unsigned int data_binary_msb = static_cast<unsigned int>(data_binary >> (sizeof(double) * 8 / 2));
  this->flash_write_word(address, data_binary_msb);
  this->flash_write_word(address + sizeof(unsigned int), data_binary_lsb);
  unsigned int msb = *reinterpret_cast<unsigned int *>(static_cast<size_t>(address));
  unsigned int lsb = *reinterpret_cast<unsigned int *>(static_cast<size_t>(address + sizeof(unsigned int)));
  unsigned long long int dat = static_cast<unsigned long long int>(
      ((std::bitset<64>(msb) << (sizeof(double) * 8 / 2)) | std::bitset<64>(lsb)).to_ullong());
  double readed_double = *reinterpret_cast<double *>(&dat);
  log("Verify written data (%lf MSB : %u, LSB : %u) on address 0x%x (%lf MSB : %u, LSB : %u) - %s\r\n", data,
      data_binary_msb, data_binary_lsb, address, readed_double, msb, lsb, (data == readed_double) ? "OK" : "FAIL");

  if (readed_double == data)
    return 0;
  else
    return 1;
}

int adc_spi_driver::deinit() {
  GPIO_InitTypeDef *GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = ADC_SPI_SCK_PIN | ADC_SPI_MOSI_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ADC_SPI_GPIO_PORT, GPIO_InitStruct);

  GPIO_StructInit(GPIO_InitStruct);
  GPIO_InitStruct->GPIO_Pin = ADC_SPI_MISO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ADC_SPI_GPIO_PORT, GPIO_InitStruct);
  delete GPIO_InitStruct;

  SPI_I2S_DeInit(ADC_SPI_PERIPH);
  this->flash_lock();
  return 0;
}
