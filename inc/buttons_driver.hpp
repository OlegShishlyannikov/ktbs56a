#ifndef BUTTONS_DRIVER_HPP
#define BUTTONS_DRIVER_HPP

#include "stm32f10x_conf.h"
#include "base_driver.hpp"

#define BUTTONS_GPIO_PORT GPIOA
#define BUTTONS_GPIO_PLUS_PIN GPIO_Pin_0
#define BUTTONS_GPIO_MINUS_PIN GPIO_Pin_1
#define BUTTONS_GPIO_RUN_PIN GPIO_Pin_2

#define BUTTON_PLUS_IRQHandler EXTI0_IRQHandler
#define BUTTON_MINUS_IRQHandler EXTI1_IRQHandler
#define BUTTON_RUN_IRQHandler EXTI2_IRQHandler

#define BUTTON_PLUS_EXTI_LINE EXTI_Line0
#define BUTTON_MINUS_EXTI_LINE EXTI_Line1
#define BUTTON_RUN_EXTI_LINE EXTI_Line2

#define BUTTON_PLUS_IRQ_CHANNEL EXTI0_IRQn
#define BUTTON_MINUS_IRQ_CHANNEL EXTI1_IRQn
#define BUTTON_RUN_IRQ_CHANNEL EXTI2_IRQn

#define BUTTONS_GPIO_PORTSOURCE GPIO_PortSourceGPIOA
#define BUTTON_PLUS_GPIO_PINSOURCE GPIO_PinSource0
#define BUTTON_MINUS_GPIO_PINSOURCE GPIO_PinSource1
#define BUTTON_RUN_GPIO_PINSOURCE GPIO_PinSource2

#define BUTTONS_DRIVER_QUEUE_SIZE 128

class buttons_driver : public base_driver
{
public:

  explicit buttons_driver( base_driver * parent = nullptr );
  explicit buttons_driver( buttons_driver & driver );
  virtual ~buttons_driver() override { this->deinit(); }
  virtual void write( const char * p_params ) override;
  virtual void * read( TickType_t timeout ) const override;
  
private:

  int init();
  int deinit();

};

extern "C"
{
  void BUTTON_MINUS_IRQHandler();
  void BUTTON_PLUS_IRQHandler();
  void BUTTON_RUN_IRQHandler();
}
#endif /* BUTTONS_DRIVER_HPP */
