#include "buttons_driver.hpp"
#include "events_id.hpp"

static QueueHandle_t buttons_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;

static const std::map< std::string, IRQn_Type > buttons_irq_map = {{ "minus", BUTTON_MINUS_IRQ_CHANNEL }, { "plus", BUTTON_MINUS_IRQ_CHANNEL }, { "run", BUTTON_MINUS_IRQ_CHANNEL }};
static const std::map< std::string, int > buttons_pin_map = {{ "minus", BUTTONS_GPIO_MINUS_PIN }, { "plus", BUTTONS_GPIO_PLUS_PIN }, { "run", BUTTONS_GPIO_RUN_PIN }};
static int report = 0;
static int from_queue = 0;

buttons_driver::buttons_driver( base_driver * parent ) : base_driver()
{
  this->init();
  buttons_driver_queue_handle = xQueueCreate( BUTTONS_DRIVER_QUEUE_SIZE, sizeof( int ));
}

buttons_driver::buttons_driver( buttons_driver & driver )
{
  *this = driver;
}

int buttons_driver::init()
{
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = BUTTONS_GPIO_MINUS_PIN | BUTTONS_GPIO_PLUS_PIN | BUTTONS_GPIO_RUN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( BUTTONS_GPIO_PORT, GPIO_InitStruct );
  delete GPIO_InitStruct;

  EXTI_InitTypeDef * EXTI_InitStruct = new EXTI_InitTypeDef;
  EXTI_StructInit( EXTI_InitStruct );
  EXTI_InitStruct->EXTI_Line = BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE;
  EXTI_InitStruct->EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStruct->EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStruct->EXTI_LineCmd = ENABLE;
  EXTI_ClearITPendingBit( BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE );
  EXTI_Init( EXTI_InitStruct );
  delete EXTI_InitStruct;
  
  GPIO_EXTILineConfig( BUTTONS_GPIO_PORTSOURCE, BUTTON_MINUS_GPIO_PINSOURCE );
  GPIO_EXTILineConfig( BUTTONS_GPIO_PORTSOURCE, BUTTON_PLUS_GPIO_PINSOURCE );
  GPIO_EXTILineConfig( BUTTONS_GPIO_PORTSOURCE, BUTTON_RUN_GPIO_PINSOURCE );

  NVIC_SetPriority( BUTTON_MINUS_IRQ_CHANNEL, 12 );
  NVIC_SetPriority( BUTTON_PLUS_IRQ_CHANNEL, 12 );
  NVIC_SetPriority( BUTTON_RUN_IRQ_CHANNEL, 12 );

  NVIC_EnableIRQ( BUTTON_MINUS_IRQ_CHANNEL );
  NVIC_EnableIRQ( BUTTON_PLUS_IRQ_CHANNEL );
  NVIC_EnableIRQ( BUTTON_RUN_IRQ_CHANNEL );
  return 0;
}

void buttons_driver::write( const char * p_params )
{
  char cmd[ 1024 ] = { '\0' };
  const char * msg = p_params; 

  if( std::sscanf( msg, "%s", cmd ) == 1 ){

	if( !std::strcmp( cmd, "get_run_button_state" )){

	  int button_state = ( buttons_pin_map.find( "run" ) != buttons_pin_map.end() ) ? GPIO_ReadInputDataBit( BUTTONS_GPIO_PORT, buttons_pin_map.at( "run" )) : 0;
	  report = button_state;
	  xQueueSendToBack( buttons_driver_queue_handle, &report, 0 );
	 
	} else if( !std::strcmp( cmd, "get_minus_button_state" )){

	  int button_state = ( buttons_pin_map.find( "minus" ) != buttons_pin_map.end() ) ? GPIO_ReadInputDataBit( BUTTONS_GPIO_PORT, buttons_pin_map.at( "minus" )) : 0;
	  report = button_state;
	  xQueueSendToBack( buttons_driver_queue_handle, &report, 0 );
	  
	} else if( !std::strcmp( cmd, "get_plus_button_state" )){

	  int button_state = ( buttons_pin_map.find( "plus" ) != buttons_pin_map.end() ) ? GPIO_ReadInputDataBit( BUTTONS_GPIO_PORT, buttons_pin_map.at( "plus" )) : 0;
	  report = button_state;
	  xQueueSendToBack( buttons_driver_queue_handle, &report, 0 );
	  
	} else if( !std::strcmp( cmd, "emulate_run_press" )){

	  report = 0;
	  ( buttons_irq_map.find( "run" ) != buttons_irq_map.end() ) ? NVIC_SetPendingIRQ( buttons_irq_map.at( "run" )) : ( void ) 0;
	  
	} else if( !std::strcmp( cmd, "emulate_minus_press" )){

	  report = 0;
	  ( buttons_irq_map.find( "minus" ) != buttons_irq_map.end() ) ? NVIC_SetPendingIRQ( buttons_irq_map.at( "minus" )) : ( void ) 0;

	} else if( !std::strcmp( cmd, "emulate_plus_press" )){

	  report = 0;
	  ( buttons_irq_map.find( "plus" ) != buttons_irq_map.end() ) ? NVIC_SetPendingIRQ( buttons_irq_map.at( "plus" )) : ( void ) 0;

	}
  }
}

void * buttons_driver::read( TickType_t timeout ) const
{
  if( xQueueReceive( buttons_driver_queue_handle, &from_queue, timeout )) return static_cast< void * >( &from_queue );
  else return nullptr;
}

int buttons_driver::deinit()
{
  NVIC_DisableIRQ( BUTTON_MINUS_IRQ_CHANNEL );
  NVIC_DisableIRQ( BUTTON_PLUS_IRQ_CHANNEL );
  NVIC_DisableIRQ( BUTTON_RUN_IRQ_CHANNEL );
  
  EXTI_InitTypeDef * EXTI_InitStruct = new EXTI_InitTypeDef;
  EXTI_StructInit( EXTI_InitStruct );
  EXTI_InitStruct->EXTI_Line = BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE;
  EXTI_InitStruct->EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStruct->EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStruct->EXTI_LineCmd = DISABLE;
  EXTI_ClearITPendingBit( BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE );
  EXTI_Init( EXTI_InitStruct );
  delete EXTI_InitStruct;
  
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = BUTTONS_GPIO_MINUS_PIN | BUTTONS_GPIO_PLUS_PIN | BUTTONS_GPIO_RUN_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( BUTTONS_GPIO_PORT, GPIO_InitStruct );
  delete GPIO_InitStruct;
  return 0;
}

extern "C" {

  void BUTTON_MINUS_IRQHandler()
  {
	if( EXTI_GetITStatus( BUTTON_MINUS_EXTI_LINE )){

	  EXTI_ClearITPendingBit( BUTTON_MINUS_EXTI_LINE );
	  portBASE_TYPE buttons_driver_task_irq_woken;
	  int event_id = BUTTON_MINUS_PRESS_EVENT_ID;
	  xQueueSendFromISR( events_worker_queue_handle, &event_id, &buttons_driver_task_irq_woken );
	  if( buttons_driver_task_irq_woken == pdTRUE ) taskYIELD();
	  
	} else EXTI_ClearITPendingBit( BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE );
  }
  
  void BUTTON_PLUS_IRQHandler()
  {
	if( EXTI_GetITStatus( BUTTON_PLUS_EXTI_LINE )){

	  EXTI_ClearITPendingBit( BUTTON_PLUS_EXTI_LINE );
	  portBASE_TYPE buttons_driver_task_irq_woken;
	  int event_id = BUTTON_PLUS_PRESS_EVENT_ID;
	  xQueueSendFromISR( events_worker_queue_handle, &event_id, &buttons_driver_task_irq_woken );
	  if( buttons_driver_task_irq_woken == pdTRUE ) taskYIELD();
	  
	} else EXTI_ClearITPendingBit( BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE );
  }
  
  void BUTTON_RUN_IRQHandler()
  {
	if( EXTI_GetITStatus( BUTTON_RUN_EXTI_LINE )){

	  EXTI_ClearITPendingBit( BUTTON_RUN_EXTI_LINE );
	  portBASE_TYPE buttons_driver_task_irq_woken;
	  int event_id = BUTTON_RUN_PRESS_EVENT_ID;
	  xQueueSendFromISR( events_worker_queue_handle, &event_id, &buttons_driver_task_irq_woken );
	  if( buttons_driver_task_irq_woken == pdTRUE ) taskYIELD();
	  
	} else EXTI_ClearITPendingBit( BUTTON_MINUS_EXTI_LINE | BUTTON_PLUS_EXTI_LINE | BUTTON_RUN_EXTI_LINE );
  }
}
