#ifndef BASE_DRIVER_HPP
#define BASE_DRIVER_HPP

#include <string>
#include <tuple>
#include <map>
#include <functional>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <sstream>
#include <bitset>
#include <vector>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "message_buffer.h"
#include "timers.h"
#include "stm32f10x_conf.h"

class base_driver
{
public:
  
  virtual ~base_driver(){ __asm__( "nop" ); };
  virtual void * read( TickType_t timeout ) const { return nullptr; };
  virtual void write( const char * p_params ){ __asm__( "nop" ); };
  
};

#endif /* BASE_DRIVER_HPP */
