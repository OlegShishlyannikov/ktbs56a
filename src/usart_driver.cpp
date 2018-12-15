#include "usart_driver.hpp"
#include "events_id.hpp"

static SemaphoreHandle_t usart_driver_semaphore_handle;
static QueueHandle_t usart_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;
static char rcvd_char;

usart_driver::usart_driver( base_driver * p_parent ) : base_driver()
{
  this->init();
  usart_driver_semaphore_handle = xSemaphoreCreateBinary();
  usart_driver_queue_handle = xQueueCreate( USART_DRIVER_OUTPUT_BUFFER_LENGTH, sizeof( char ));
}

usart_driver::usart_driver( usart_driver & driver )
{
  *this = driver;
}

int usart_driver::init()
{
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;  
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = USART_TX_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( USART_GPIO_PORT, GPIO_InitStruct );

  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = USART_RX_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( USART_GPIO_PORT, GPIO_InitStruct );    
  delete GPIO_InitStruct;

  USART_InitTypeDef * USART_InitStruct = new USART_InitTypeDef;  
  USART_StructInit( USART_InitStruct );
  USART_InitStruct->USART_BaudRate = 115200;
  USART_InitStruct->USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStruct->USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_InitStruct->USART_Parity = USART_Parity_No;
  USART_InitStruct->USART_StopBits = USART_StopBits_1;
  USART_InitStruct->USART_WordLength = USART_WordLength_8b;
  USART_Init( USART_PERIPH_PORT, USART_InitStruct );

  USART_ITConfig( USART_PERIPH_PORT, USART_IT_RXNE, ENABLE );
  DMA_ITConfig( USART_DMA_PERIPH_CHANNEL, DMA_IT_TC, ENABLE );
  USART_Cmd( USART_PERIPH_PORT, ENABLE );  
  delete USART_InitStruct;  
  NVIC_SetPriority( USART1_IRQn, 5 );
  NVIC_EnableIRQ( USART1_IRQn );
  NVIC_SetPriority( DMA1_Channel4_IRQn, 5 );
  NVIC_EnableIRQ( DMA1_Channel4_IRQn );
  return 0;
}

int usart_driver::deinit()
{
  USART_DeInit( USART_PERIPH_PORT );
  DMA_DeInit( USART_DMA_PERIPH_CHANNEL );  
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = USART_TX_GPIO_PIN | USART_TX_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( USART_GPIO_PORT, GPIO_InitStruct );  
  delete GPIO_InitStruct;  
  NVIC_DisableIRQ( USART1_IRQn );
  NVIC_DisableIRQ( DMA1_Channel4_IRQn );
  return 0;
}

void * usart_driver::read( TickType_t timeout ) const
{
  if( xQueueReceive( usart_driver_queue_handle, &rcvd_char, timeout )) return static_cast< void * >( &rcvd_char );
  else return nullptr;
}

void usart_driver::write( const char * p_params )
{
  const char * p_str = p_params; 
  DMA_InitTypeDef DMA_InitStruct;  
  DMA_Cmd( USART_DMA_PERIPH_CHANNEL, DISABLE );
  DMA_StructInit( &DMA_InitStruct );
  DMA_InitStruct.DMA_PeripheralBaseAddr = reinterpret_cast< size_t >( &USART_PERIPH_PORT->DR );
  DMA_InitStruct.DMA_MemoryBaseAddr = reinterpret_cast< size_t >( p_str );
  DMA_InitStruct.DMA_BufferSize = std::strlen( p_str );
  DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_Init( USART_DMA_PERIPH_CHANNEL, &DMA_InitStruct );
  DMA_ITConfig( USART_DMA_PERIPH_CHANNEL, DMA_IT_TC, ENABLE );
  USART_DMACmd( USART_PERIPH_PORT, USART_DMAReq_Tx, ENABLE );
  DMA_Cmd( USART_DMA_PERIPH_CHANNEL, ENABLE );

  if( !xSemaphoreTake( usart_driver_semaphore_handle, portIO_MAX_DELAY * 20 )){
	
	DMA_DeInit( USART_DMA_PERIPH_CHANNEL );
	DMA_Cmd( USART_DMA_PERIPH_CHANNEL, DISABLE );
	
  }
}

extern "C" void USART1_IRQHandler()
{
  if( USART_GetITStatus( USART_PERIPH_PORT, USART_IT_RXNE )){
	
	char temp = USART_ReceiveData( USART_PERIPH_PORT );
	portBASE_TYPE usart_driver_task_irq_woken;
	int event_id = USART_CHAR_RECEIVED_EVENT_ID;
	xQueueSendToBackFromISR( events_worker_queue_handle, &event_id, &usart_driver_task_irq_woken );
	xQueueSendToBackFromISR( usart_driver_queue_handle, &temp, &usart_driver_task_irq_woken );
	if( usart_driver_task_irq_woken == pdTRUE ) taskYIELD();
	
  } else USART_ClearITPendingBit( USART_PERIPH_PORT, USART_IT_RXNE | USART_IT_CTS | USART_IT_ERR | USART_IT_FE | USART_IT_IDLE | USART_IT_LBD | USART_IT_NE | USART_IT_ORE | USART_IT_PE | USART_IT_TC | USART_IT_TXE );
}

extern "C" void DMA1_Channel4_IRQHandler()
{
  if( DMA_GetITStatus( DMA1_IT_TC4 )){

	DMA_ClearITPendingBit( DMA1_Channel4_IRQn );
	DMA_ClearFlag( DMA1_IT_GL4 | DMA1_IT_HT4 | DMA1_IT_TE4 | DMA1_IT_TC4 );
	portBASE_TYPE usart_driver_task_irq_woken;
	xSemaphoreGiveFromISR( usart_driver_semaphore_handle, &usart_driver_task_irq_woken );
	if( usart_driver_task_irq_woken == pdTRUE ) taskYIELD();
	
  } else {
	
	DMA_ClearITPendingBit( DMA1_Channel4_IRQn );
	DMA_ClearFlag( DMA1_IT_GL4 | DMA1_IT_HT4 | DMA1_IT_TE4 | DMA1_IT_TC4 );
	
  }
}
