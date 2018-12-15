#ifndef EVENTS_WORKER_HPP
#define EVENTS_WORKER_HPP

#include "signal.hpp"

#define EVENTS_WORKER_QUEUE_SIZE 1024

class events_worker
{
public:

  explicit events_worker();
  virtual ~events_worker() = default;
  
  void events_worker_task_code( void * parameters );  
  static inline void events_worker_task_code_wrapper( void * parameters )
  {
	reinterpret_cast< events_worker * >( parameters )->events_worker_task_code( parameters );
  }
  
  static std::string get_name(){ return "events_worker"; }
  static std::map< int, signal< void * > * > * get_event_signals_map();
};

#endif /* EVENTS_WORKER_HPP */
