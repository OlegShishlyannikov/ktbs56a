#ifndef SHELL_HPP
#define SHELL_HPP

#include <functional>
#include <vector>
#include <sstream>
#include <tuple>
#include <typeinfo>
#include <map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "message_buffer.h"
#include "timers.h"

#include "events_worker.hpp"
#include "base_driver.hpp"
#include "term.hpp"

#define MAX_ARGS 8
#define MAX_BUFFER_LENGTH 64
#define MAX_ESC_SEQ_BUFFER_LENGTH 8
#define MAX_HISTORY_LENGTH 16

class shell;

extern bool exec( std::string func_name, std::tuple< std::vector< std::string > *, SemaphoreHandle_t *, shell * > * args );
extern void load_applications();
extern std::vector< std::string > * get_app_names();
extern std::vector< std::string > * get_startup_applications();
extern TaskHandle_t * get_task_handle( std::string task_name );

extern base_driver * get_driver_ptr( std::string driver_name );
extern void load_drivers();

static double result_double = 0.0f;
static int result_int = 0;
static char result_char = '\0';

class shell
{
public:

  explicit shell();
  virtual ~shell() = default;
  
  void shell_task_code( void * parameters );
  
  static inline void shell_task_code_wrapper( void * parameters ){ reinterpret_cast< shell * >( parameters )->shell_task_code( parameters ); }
  static inline std::string get_name(){ return "shell"; }
  void handle_input_char( const char c );
  
  static class {
	
  public:
	
	void disconnect_all()
	{
	  for( auto it : *( events_worker::get_event_signals_map() )) it.second->disconnect_all();
	}
	
	void * operator[]( int event_id )
	{
	  return ( events_worker::get_event_signals_map()->find( event_id ) != events_worker::get_event_signals_map()->end() )
		? events_worker::get_event_signals_map()->at( event_id )
		: nullptr;
	}
  } callback_signals;
  
  static class {

	class driver {
	
	public:

	  base_driver * p_driver_inst;
	  void write( std::string str_fmt, ... )
	  {
		std::va_list arg;
		char temp[ 1024 ] = { '\0' };
		va_start( arg, str_fmt );
		std::vsprintf( temp, str_fmt.c_str(), arg );
		va_end( arg );
		p_driver_inst->write( temp );
	  }
	  
	  void * read( TickType_t timeout )
	  {
		return p_driver_inst->read( timeout );
	  }
	};

  public:
	
	driver operator[]( std::string driver_name )
	{
	  driver driver_inst;
	  driver_inst.p_driver_inst = get_driver_ptr( driver_name );
	  return driver_inst;
	};
  } hardware;
};

#endif /* SHELL_HPP */
