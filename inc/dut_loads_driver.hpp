#ifndef DUT_LOADS_DRIVER_HPP
#define DUT_LOADS_DRIVER_HPP

#include "stm32f10x_conf.h"
#include "base_driver.hpp"

#define DUT_LOADS_GPIO_PORT GPIOA
#define DUT_LOADS_ADC_PERIPH ADC1
#define DUT_LOADS_GPIO_PIN GPIO_Pin_4
#define DUT_LOADS_DAC_CHANNEL DAC_Channel_1
#define DUT_LOADS_ADC_GPIO_PIN GPIO_Pin_3

#define DUT_LOADS_DRIVER_QUEUE_SIZE 128

class dut_loads_driver : public base_driver
{
public:

  explicit dut_loads_driver( base_driver * parent = nullptr );
  explicit dut_loads_driver( dut_loads_driver & driver );
  virtual ~dut_loads_driver() override { this->deinit(); };

  virtual void write( const char * p_params ) override;
  virtual void * read( TickType_t timeout ) const override;
  
private:

  int init();
  int deinit();
  int set_current( double ma );
  int calibrate();
  uint16_t read_adc();
  
  constexpr static const double ref_voltage = 3.2865f;
  constexpr static const double ref_ohms = 21.563625;
};

#endif /* DUT_LOADS_DRIVER_HPP */
