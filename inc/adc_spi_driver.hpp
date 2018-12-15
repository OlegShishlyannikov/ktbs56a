#ifndef ADC_SPI_DRIVER_HPP
#define ADC_SPI_DRIVER_HPP

#include "stm32f10x_conf.h"
#include "base_driver.hpp"

#define ADC_SPI_GPIO_PORT GPIOA
#define ADC_SPI_PERIPH SPI1
#define ADC_SPI_MISO_PIN GPIO_Pin_6
#define ADC_SPI_MOSI_PIN GPIO_Pin_7
#define ADC_SPI_SCK_PIN GPIO_Pin_5

#define ADC_SS_RESET_MODE           std::bitset< 1 >( 0b0 )
#define ADC_SS_START_MODE           std::bitset< 1 >( 0b1 )
#define ADC_SS_FALSE_MODE           std::bitset< 1 >( 0b0 )
#define ADS1118_SS_POS              15

#define ADC_MUX_RESET_MODE          std::bitset< 3 >( 0b000 )
#define ADC_MUX_0P_1N_MODE          std::bitset< 3 >( 0b000 )
#define ADC_MUX_0P_3N_MODE          std::bitset< 3 >( 0b001 )
#define ADC_MUX_1P_3N_MODE          std::bitset< 3 >( 0b010 )
#define ADC_MUX_2P_3N_MODE          std::bitset< 3 >( 0b011 )
#define ADC_MUX_0P_GN_MODE          std::bitset< 3 >( 0b100 )
#define ADC_MUX_1P_GN_MODE          std::bitset< 3 >( 0b101 )
#define ADC_MUX_2P_GN_MODE          std::bitset< 3 >( 0b110 )
#define ADC_MUX_3P_GN_MODE          std::bitset< 3 >( 0b111 )
#define ADS1118_MUX_POS             12

#define ADC_PGA_RESET_MODE          std::bitset< 3 >( 0b010 )
#define ADC_PGA_FSR_6144_MODE       std::bitset< 3 >( 0b000 )
#define ADC_PGA_FSR_4096_MODE       std::bitset< 3 >( 0b001 )
#define ADC_PGA_FSR_2048_MODE       std::bitset< 3 >( 0b010 )
#define ADC_PGA_FSR_1024_MODE       std::bitset< 3 >( 0b011 )
#define ADC_PGA_FSR_512_MODE        std::bitset< 3 >( 0b100 )
#define ADC_PGA_FSR_256_MODE        std::bitset< 3 >( 0b101 )
#define ADC_PGA_FSR_128_MODE        std::bitset< 3 >( 0b110 )
#define ADC_PGA_FSR_64_MODE         std::bitset< 3 >( 0b111 )
#define ADS1118_PGA_POS             9

#define ADC_OPMODE_RESET_MODE       std::bitset< 1 >( 0b1 )
#define ADC_OPMODE_CC_MODE          std::bitset< 1 >( 0b0 )
#define ADC_OPMODE_SS_PWRDWN_MODE   std::bitset< 1 >( 0b1 )
#define ADS1118_OPMODE_POS          8

#define ADC_DR_RESET_MODE           std::bitset< 3 >( 0b100 )
#define ADC_DR_8_MODE               std::bitset< 3 >( 0b000 )
#define ADC_DR_16_MODE              std::bitset< 3 >( 0b001 )
#define ADC_DR_32_MODE              std::bitset< 3 >( 0b010 )
#define ADC_DR_64_MODE              std::bitset< 3 >( 0b011 )
#define ADC_DR_128_MODE             std::bitset< 3 >( 0b100 )
#define ADC_DR_250_MODE             std::bitset< 3 >( 0b101 )
#define ADC_DR_475_MODE             std::bitset< 3 >( 0b110 )
#define ADC_DR_860_MODE             std::bitset< 3 >( 0b111 )
#define ADS1118_DR_POS              5

#define ADC_TSMODE_RESET_MODE       std::bitset< 1 >( 0b0 )
#define ADC_TSMODE_ADC_MODE         std::bitset< 1 >( 0b0 )
#define ADC_TSMODE_TEMP_MODE        std::bitset< 1 >( 0b1 )
#define ADS1118_TSMODE_POS          4

#define ADC_PULLUP_RESET_MODE       std::bitset< 1 >( 0b1 )
#define ADC_PULLUP_DIS_MODE         std::bitset< 1 >( 0b0 )
#define ADC_PULLUP_EN_MODE          std::bitset< 1 >( 0b1 )
#define ADS1118_PULLUPEN_POS        3

#define ADC_NOP_RESET_MODE          std::bitset< 2 >( 0b01 )
#define ADC_NOP_INVALID_DATA_MODE   std::bitset< 2 >( 0b00 )
#define ADC_NOP_VALID_DATA_MODE     std::bitset< 2 >( 0b01 )
#define ADS1118_NOP_POS             1

#define ADC_RDY_FLAG_MODE           std::bitset< 1 >( 0b0 )
#define ADS1118_RDY_FLAG_POS        0

#define FLASH_KEY1 (( uint32_t ) 0x45670123 )
#define FLASH_KEY2 (( uint32_t ) 0xCDEF89AB )
  
#define ADC_SPI_DRIVER_QUEUE_SIZE 128
#define DUTS_NUMBER 35

extern void log( const char * p_str_fmt, ... );

class adc_spi_driver : public base_driver
{
public:

  explicit adc_spi_driver( base_driver * parent = nullptr );
  explicit adc_spi_driver( adc_spi_driver & driver );
  virtual ~adc_spi_driver() override { this->deinit(); }

  virtual void write( const char * p_params ) override;
  virtual void * read( TickType_t timeout ) const override;
  
private:

  int init();
  int deinit();
  int get_data( unsigned int channel, unsigned int mode );
  int get_temp( unsigned int channel, unsigned int mode );
  int write_calibration_table( double * table_address, unsigned int table_size );
  int read_calibration_table( unsigned int table_size );
  void flash_unlock();
  void flash_lock();
  bool flash_ready();
  int flash_erase_all_pages();
  int flash_erase_page( uint32_t address );
  int flash_write_word( unsigned int address, unsigned int data );
  int flash_write_double( uint32_t address, double data );
  
  static class {

  public:
	
	std::bitset< 1 > OS = ADC_SS_RESET_MODE;
	std::bitset< 3 > MUX = ADC_MUX_RESET_MODE;
	std::bitset< 3 > PGA = ADC_PGA_RESET_MODE;
	std::bitset< 1 > MODE = ADC_OPMODE_RESET_MODE;
	std::bitset< 3 > DR = ADC_DR_RESET_MODE;
	std::bitset< 1 > TS_MODE = ADC_TSMODE_RESET_MODE;
	std::bitset< 1 > PULL_UP_EN = ADC_PULLUP_RESET_MODE;
	std::bitset< 2 > NOP = ADC_NOP_RESET_MODE;
	std::bitset< 1 > RDY_FLAG = ADC_RDY_FLAG_MODE;
	
  } ADS1118_ConfRegInitStruct;
};

#endif /* ADC_SPI_DRIVER_HPP */
