#ifndef LEDS_DRIVER_HPP
#define LEDS_DRIVER_HPP

#include "stm32f10x_conf.h"
#include "base_driver.hpp"

#define LEDS_GPIO_PORT GPIOC
#define LED1_GPIO_PIN GPIO_Pin_3
#define LED2_GPIO_PIN GPIO_Pin_2
#define LED3_GPIO_PIN GPIO_Pin_1
#define LED4_GPIO_PIN GPIO_Pin_0

#define LEDS_DRIVER_QUEUE_SIZE 128

class leds_driver : public base_driver
{
public:

  explicit leds_driver( base_driver * parent = nullptr );
  explicit leds_driver( leds_driver & driver );
  virtual ~leds_driver() override { this->deinit(); };

  virtual void write( const char * p_params ) override;
  virtual void * read( TickType_t timeout ) const override;
  
private:

  int init();
  int deinit();

};

#endif /* LEDS_DRIVER_HPP */
