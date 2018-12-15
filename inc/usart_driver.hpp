#ifndef USART_DRIVER_HPP
#define USART_DRIVER_HPP

#include "stm32f10x_conf.h"
#include "base_driver.hpp"

#define USART_PERIPH_PORT USART1
#define USART_IRQ_PRIORITY 12

#define USART_DMA_PERIPH_PORT DMA1
#define USART_DMA_PERIPH_CHANNEL DMA1_Channel4

#define USART_GPIO_PORT GPIOA
#define USART_RX_GPIO_PIN GPIO_Pin_10
#define USART_TX_GPIO_PIN GPIO_Pin_9

#define USART_DRIVER_MESSAGE_BUFFER_SIZE 1024
#define USART_DRIVER_OUTPUT_BUFFER_LENGTH 64

class usart_driver : public base_driver
{
public:

  explicit usart_driver( base_driver * parent = nullptr );
  explicit usart_driver( usart_driver & driver );
  virtual ~usart_driver() override { this->deinit(); }
  
  virtual void write( const char * p_params ) override;
  virtual void * read( TickType_t timeout ) const override;
  
private:
  
  int init();
  int deinit();
};
#endif /* USART_DRIVER_HPP */
