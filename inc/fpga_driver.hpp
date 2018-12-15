#ifndef FPGA_DRIVER_HPP
#define FPGA_DRIVER_HPP

#include "base_driver.hpp"

#define FPGA_GPIO_PORT GPIOB
#define FPGA_GPIO_VS0_PIN GPIO_Pin_0
#define FPGA_GPIO_VS1_PIN GPIO_Pin_1
#define FPGA_GPIO_VS2_PIN GPIO_Pin_3
#define FPGA_GPIO_M0_PIN GPIO_Pin_4
#define FPGA_GPIO_M1_PIN GPIO_Pin_5
#define FPGA_GPIO_M2_PIN GPIO_Pin_6
#define FPGA_GPIO_DONE_PIN GPIO_Pin_7
#define FPGA_GPIO_PROG_B_PIN GPIO_Pin_8
#define FPGA_GPIO_SUSPEND_PIN GPIO_Pin_9
#define FPGA_GPIO_NRST_PIN GPIO_Pin_10
#define FPGA_GPIO_EN_PIN GPIO_Pin_11
#define FPGA_GPIO_NCS_PIN GPIO_Pin_12
#define FPGA_GPIO_CLK_PIN GPIO_Pin_13
#define FPGA_GPIO_DOUT_PIN GPIO_Pin_14
#define FPGA_GPIO_DIN_PIN GPIO_Pin_15
#define FPGA_SPI_PERIPH SPI2

#define GREEN_LED_ON_COMMAND      0x31
#define GREEN_LED_OFF_COMMAND     0x32
#define RED_LED_ON_COMMAND        0x33
#define RED_LED_OFF_COMMAND       0x34
#define ADC_SELECT_COMMAND        0x35
#define ADC_DESELECT_COMMAND      0x36
#define ADC_DESELECT_ALL_COMMAND  0x37
#define RESET_ALL_COMMAND         0x38

#define FPGA_DRIVER_QUEUE_SIZE 128

class fpga_driver : public base_driver
{
public:

  explicit fpga_driver( base_driver * p_parent = nullptr );
  fpga_driver( fpga_driver & driver );
  virtual ~fpga_driver() override { this->deinit(); }
  
  virtual void * read( TickType_t timeout ) const override;
  virtual void write( const char * p_params ) override;
  
private:

  int init();
  int deinit();
  int green_led_on( unsigned int led_num );
  int green_led_off( unsigned int led_num );
  int red_led_on( unsigned int led_num );
  int red_led_off( unsigned int led_num );
  int adc_select( unsigned int adc_num );
  int adc_deselect( unsigned int adc_num );
  int adc_deselect_all();
  int reset_all();
  int get_conf_status();
  int send_packet( unsigned int command, unsigned int data );

};
#endif /* FPGA_DRIVER_HPP */
