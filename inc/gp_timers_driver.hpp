#ifndef GP_TIMERS_DRIVER_HPP
#define GP_TIMERS_DRIVER_HPP

#include "stm32f10x_conf.h"
#include "base_driver.hpp"

#define TIM0_PERIPH TIM1
#define TIM1_PERIPH TIM2
#define TIM2_PERIPH TIM3
#define TIM3_PERIPH TIM4

#define GP_TIMERS_DRIVER_QUEUE_SIZE 128

class gp_timers_driver : public base_driver
{
public:

  explicit gp_timers_driver( base_driver * parent = nullptr );
  explicit gp_timers_driver( gp_timers_driver & driver );
  virtual ~gp_timers_driver() override { this->deinit(); }

  virtual void write( const char * p_params ) override;
  virtual void * read( TickType_t timeout ) const override;
  
private:

  int init();
  int deinit();

};


#endif /* GP_TIMERS_DRIVER_HPP */
