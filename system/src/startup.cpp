#include "startup.hpp"
#include <cstdarg>
#include <cstdio>

inline void __attribute__(( always_inline )) __initialize_data( unsigned int * from, unsigned int * section_begin, unsigned int * section_end )
{
  unsigned int * p = section_begin;
  while( p < section_end ) * p ++ = * from ++;
}

inline void __attribute__(( always_inline )) __initialize_bss( unsigned int * section_begin, unsigned int * section_end )
{
  unsigned int * p = section_begin;
  while( p < section_end ) * p ++ = 0;
}

extern void ( * __preinit_array_start[] )( void ) __attribute__(( weak ));
extern void ( * __preinit_array_end[] )( void ) __attribute__(( weak ));
extern void ( * __init_array_start[] )( void ) __attribute__(( weak ));
extern void ( * __init_array_end[] )( void ) __attribute__(( weak ));
extern void ( * __fini_array_start[] )( void ) __attribute__(( weak ));
extern void ( * __fini_array_end[] )( void ) __attribute__(( weak ));

void log( const char * p_str_fmt, ... )
{
  std::va_list arg;
  char temp[ 1024 ];
  std::memset( temp, '\0', 1024 );
  va_start( arg, p_str_fmt );
  std::vsprintf( temp, p_str_fmt, arg );
  va_end( arg );  

  for( unsigned int i = 0; i < std::strlen( temp ); i ++ ){

	while( !USART_GetFlagStatus( USART1, USART_FLAG_TC ));
	USART_SendData( USART1, temp[ i ]);
	
  }
}

inline void __attribute__(( always_inline )) __run_init_array( void )
{
  int count;
  int i;
  count = __preinit_array_end - __preinit_array_start;
  
  for( i = 0; i < count; i ++ ) __preinit_array_start[ i ]();

  count = __init_array_end - __init_array_start;

  for( i = 0; i < count; i ++ ) __init_array_start[ i ]();
}

inline void __attribute__(( always_inline )) __run_fini_array( void )
{
  int count;
  int i;
  count = __fini_array_end - __fini_array_start;

  for( i = count; i > 0; i -- ) __fini_array_start[ i - 1 ]();
}

#if defined( DEBUG ) && defined( OS_INCLUDE_STARTUP_GUARD_CHECKS )

#define BSS_GUARD_BAD_VALUE ( 0xCADEBABA )

static unsigned int volatile __attribute__ (( section( ".bss_begin" ))) __bss_begin_guard;
static unsigned int volatile __attribute__ (( section( ".bss_end" ))) __bss_end_guard;

#define DATA_GUARD_BAD_VALUE ( 0xCADEBABA )
#define DATA_BEGIN_GUARD_VALUE ( 0x12345678 )
#define DATA_END_GUARD_VALUE ( 0x98765432 )

static unsigned int volatile __attribute__ (( section( ".data_begin" ))) __data_begin_guard = DATA_BEGIN_GUARD_VALUE;
static unsigned int volatile __attribute__ (( section( ".data_end" ))) __data_end_guard = DATA_END_GUARD_VALUE;

#endif /* defined( DEBUG ) && defined( OS_INCLUDE_STARTUP_GUARD_CHECKS ) */

void __initialize_rcc( void )
{
  RCC_DeInit(); /* Clear prevous RCC configuration */
  RCC_HSEConfig( RCC_HSE_ON ); /* Enable HSE */
  RCC_WaitForHSEStartUp(); /* Wait for HSE startup */
  RCC_HSICmd( DISABLE ); /* Disable HSI */
  RCC_PLLConfig( RCC_PLLSource_HSE_Div1, 16 ); /* Setup HSE */
  RCC_PLLCmd( ENABLE ); /* Enable PLL */

  while( !RCC_GetFlagStatus( RCC_FLAG_PLLRDY )); /* Wait for PLL startup */

  RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK ); /* Set SYSCLK clock source */

  while( RCC_GetSYSCLKSource() != RCC_CFGR_SWS_PLL ); /* Wait for sysclk source */
  
  RCC_HCLKConfig( RCC_SYSCLK_Div1 ); /* Set HCLK clock source */
  RCC_PCLK1Config( RCC_HCLK_Div2 ); /* Set PCLK1 clock source */
  RCC_PCLK2Config( RCC_HCLK_Div1 ); /* Set PCLK2 clock source */

  RCC_APB1PeriphClockCmd( RCC_APB1Periph_DAC | RCC_APB1Periph_PWR | RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_SPI2 | RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM4, ENABLE );
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1 | RCC_APB2Periph_SPI1 | RCC_APB2Periph_ADC1 | RCC_APB2Periph_TIM1, ENABLE );
  RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE );
  SystemCoreClockUpdate();
}

void __hard_exceptions_enable( void )
{
  SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA;
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA;
  SCB->SHCSR |= SCB_SHCSR_USGFAULTENA;  
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA;
}

void __hard_exceptions_disable( void )
{
  SCB->SHCSR &= ~( SCB_SHCSR_BUSFAULTENA );
  SCB->SHCSR &= ~( SCB_SHCSR_MEMFAULTENA );
  SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA );  
  SCB->SHCSR &= ~( SCB_SHCSR_MEMFAULTENA );
}

void __attribute__(( noreturn )) _exit( int )
{
  __asm__( "nop" );
  while( true );
}

void __attribute__ (( used, noreturn, section( ".after_vectors" ))) _start( void )
{

#if defined( DEBUG ) && defined( OS_INCLUDE_STARTUP_GUARD_CHECKS )
  __bss_begin_guard = BSS_GUARD_BAD_VALUE;
  __bss_end_guard = BSS_GUARD_BAD_VALUE;
#endif

  __initialize_bss( &__bss_start__, &__bss_end__ );

#if defined( DEBUG ) && defined( OS_INCLUDE_STARTUP_GUARD_CHECKS )
  if(( __bss_begin_guard != 0 ) || ( __bss_end_guard != 0 )){
	
	while( true );
	
  }
#endif

#if defined( DEBUG ) && defined( OS_INCLUDE_STARTUP_GUARD_CHECKS )
  __data_begin_guard = DATA_GUARD_BAD_VALUE;
  __data_end_guard = DATA_GUARD_BAD_VALUE;
#endif

  __initialize_data( &_sidata, &_sdata, &_edata);

#if defined( DEBUG ) && defined( OS_INCLUDE_STARTUP_GUARD_CHECKS )
  if(( __data_begin_guard != DATA_BEGIN_GUARD_VALUE )){
	
	while( true );
	
  }
#endif
  
  __initialize_rcc();
  int argc;
  char ** argv;
  __run_init_array();
  __hard_exceptions_enable();
  int code = main( argc, argv );
  __run_fini_array();
  _exit( code );
  while( true );
}

void __nmi_handler( void )
{
  __asm__( "nop" );
  while( true );
}

extern "C" void __usage_fault_handler( unsigned int * args )
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;
 
  stacked_r0 = (( unsigned long ) args[ 0 ]);
  stacked_r1 = (( unsigned long ) args[ 1 ]);
  stacked_r2 = (( unsigned long ) args[ 2 ]);
  stacked_r3 = (( unsigned long ) args[ 3 ]);
 
  stacked_r12 = (( unsigned long ) args[ 4 ]);
  stacked_lr = (( unsigned long ) args[ 5 ]);
  stacked_pc = (( unsigned long ) args[ 6 ]);
  stacked_psr = (( unsigned long ) args[ 7 ]);
 
  log( "\r\n\r\n[ UsageFault_Handler fault handler - all numbers in hex ]\r\n");
  log( "R0 = 0x%x\r\n", stacked_r0 );
  log( "R1 = 0x%x\r\n", stacked_r1 );
  log( "R2 = 0x%x\r\n", stacked_r2 );
  log( "R3 = 0x%x\r\n", stacked_r3 );
  log( "R12 = 0x%x\r\n", stacked_r12 );
  log( "LR [R14] = 0x%x  subroutine call return address\r\n", stacked_lr );
  log( "PC [R15] = 0x%x  program counter\r\n", stacked_pc );
  log( "PSR = 0x%x\r\n", stacked_psr );
  log( "BFAR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED38 ))));
  log( "CFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED28 ))));
  log( "HFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED2C ))));
  log( "DFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED30 ))));
  log( "AFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED3C ))));
  log( "SCB_SHCSR = 0x%x\r\n", SCB->SHCSR );
  
  while( true );
}

extern "C" void __bus_fault_handler( unsigned int * args )
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;
 
  stacked_r0 = (( unsigned long ) args[ 0 ]);
  stacked_r1 = (( unsigned long ) args[ 1 ]);
  stacked_r2 = (( unsigned long ) args[ 2 ]);
  stacked_r3 = (( unsigned long ) args[ 3 ]);
 
  stacked_r12 = (( unsigned long ) args[ 4 ]);
  stacked_lr = (( unsigned long ) args[ 5 ]);
  stacked_pc = (( unsigned long ) args[ 6 ]);
  stacked_psr = (( unsigned long ) args[ 7 ]);
 
  log( "\r\n\r\n[ BusFault_Handler fault handler - all numbers in hex ]\r\n");
  log( "R0 = 0x%x\r\n", stacked_r0 );
  log( "R1 = 0x%x\r\n", stacked_r1 );
  log( "R2 = 0x%x\r\n", stacked_r2 );
  log( "R3 = 0x%x\r\n", stacked_r3 );
  log( "R12 = 0x%x\r\n", stacked_r12 );
  log( "LR [R14] = 0x%x  subroutine call return address\r\n", stacked_lr );
  log( "PC [R15] = 0x%x  program counter\r\n", stacked_pc );
  log( "PSR = 0x%x\r\n", stacked_psr );
  log( "BFAR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED38 ))));
  log( "CFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED28 ))));
  log( "HFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED2C ))));
  log( "DFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED30 ))));
  log( "AFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED3C ))));
  log( "SCB_SHCSR = 0x%x\r\n", SCB->SHCSR );
  
  while( true );
}

extern "C" void __mem_manage_handler( unsigned int * args )
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;
 
  stacked_r0 = (( unsigned long ) args[ 0 ]);
  stacked_r1 = (( unsigned long ) args[ 1 ]);
  stacked_r2 = (( unsigned long ) args[ 2 ]);
  stacked_r3 = (( unsigned long ) args[ 3 ]);
 
  stacked_r12 = (( unsigned long ) args[ 4 ]);
  stacked_lr = (( unsigned long ) args[ 5 ]);
  stacked_pc = (( unsigned long ) args[ 6 ]);
  stacked_psr = (( unsigned long ) args[ 7 ]);
 
  log( "\r\n\r\n[ MemManage_Handler fault handler - all numbers in hex ]\r\n");
  log( "R0 = 0x%x\r\n", stacked_r0 );
  log( "R1 = 0x%x\r\n", stacked_r1 );
  log( "R2 = 0x%x\r\n", stacked_r2 );
  log( "R3 = 0x%x\r\n", stacked_r3 );
  log( "R12 = 0x%x\r\n", stacked_r12 );
  log( "LR [R14] = 0x%x  subroutine call return address\r\n", stacked_lr );
  log( "PC [R15] = 0x%x  program counter\r\n", stacked_pc );
  log( "PSR = 0x%x\r\n", stacked_psr );
  log( "BFAR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED38 ))));
  log( "CFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED28 ))));
  log( "HFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED2C ))));
  log( "DFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED30 ))));
  log( "AFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED3C ))));
  log( "SCB_SHCSR = 0x%x\r\n", SCB->SHCSR );
  
  while( true );
}

extern "C" void __hard_fault_handler( unsigned int * args )
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;
 
  stacked_r0 = (( unsigned long ) args[ 0 ]);
  stacked_r1 = (( unsigned long ) args[ 1 ]);
  stacked_r2 = (( unsigned long ) args[ 2 ]);
  stacked_r3 = (( unsigned long ) args[ 3 ]);
 
  stacked_r12 = (( unsigned long ) args[ 4 ]);
  stacked_lr = (( unsigned long ) args[ 5 ]);
  stacked_pc = (( unsigned long ) args[ 6 ]);
  stacked_psr = (( unsigned long ) args[ 7 ]);
 
  log( "\r\n\r\n[ HardFault_Handler - all numbers in hex ]\r\n");
  log( "R0 = 0x%x\r\n", stacked_r0 );
  log( "R1 = 0x%x\r\n", stacked_r1 );
  log( "R2 = 0x%x\r\n", stacked_r2 );
  log( "R3 = 0x%x\r\n", stacked_r3 );
  log( "R12 = 0x%x\r\n", stacked_r12 );
  log( "LR [R14] = 0x%x  subroutine call return address\r\n", stacked_lr );
  log( "PC [R15] = 0x%x  program counter\r\n", stacked_pc );
  log( "PSR = 0x%x\r\n", stacked_psr );
  log( "BFAR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED38 ))));
  log( "CFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED28 ))));
  log( "HFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED2C ))));
  log( "DFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED30 ))));
  log( "AFSR = 0x%x\r\n", ( *(( volatile unsigned long * )( 0xE000ED3C ))));
  log( "SCB_SHCSR = 0x%x\r\n", SCB->SHCSR );
  
  while( true );
}

void __attribute__(( used, section( ".isr_vector" ))) ( * const g_pfnVectors[] )( void ) = {

  reinterpret_cast< void ( * )( void )>( &_estack ),
  Reset_Handler,
  NMI_Handler,
  HardFault_Handler,
  MemManage_Handler,
  BusFault_Handler,
  UsageFault_Handler,
  0,
  0,
  0,
  0,
  SVC_Handler,
  DebugMon_Handler,
  0,
  PendSV_Handler,
  SysTick_Handler,
  WWDG_IRQHandler,
  PVD_IRQHandler,
  TAMPER_IRQHandler,
  RTC_IRQHandler,
  FLASH_IRQHandler,
  RCC_IRQHandler,
  EXTI0_IRQHandler,
  EXTI1_IRQHandler,
  EXTI2_IRQHandler,
  EXTI3_IRQHandler,
  EXTI4_IRQHandler,
  DMA1_Channel1_IRQHandler,
  DMA1_Channel2_IRQHandler,
  DMA1_Channel3_IRQHandler,
  DMA1_Channel4_IRQHandler,
  DMA1_Channel5_IRQHandler,
  DMA1_Channel6_IRQHandler,
  DMA1_Channel7_IRQHandler,
  ADC1_2_IRQHandler,
  USB_HP_CAN1_TX_IRQHandler,
  USB_LP_CAN1_RX0_IRQHandler,
  CAN1_RX1_IRQHandler,
  CAN1_SCE_IRQHandler,
  EXTI9_5_IRQHandler,
  TIM1_BRK_TIM9_IRQHandler,
  TIM1_UP_TIM10_IRQHandler,
  TIM1_TRG_COM_TIM11_IRQHandler,
  TIM1_CC_IRQHandler,
  TIM2_IRQHandler,
  TIM3_IRQHandler,
  TIM4_IRQHandler,
  I2C1_EV_IRQHandler,
  I2C1_ER_IRQHandler,
  I2C2_EV_IRQHandler,
  I2C2_ER_IRQHandler,
  SPI1_IRQHandler,
  SPI2_IRQHandler,
  USART1_IRQHandler,
  USART2_IRQHandler,
  USART3_IRQHandler,
  EXTI15_10_IRQHandler,
  RTCAlarm_IRQHandler,
  USBWakeUp_IRQHandler,
  TIM8_BRK_TIM12_IRQHandler,
  TIM8_UP_TIM13_IRQHandler,
  TIM8_TRG_COM_TIM14_IRQHandler,
  TIM8_CC_IRQHandler,
  ADC3_IRQHandler,
  FSMC_IRQHandler,
  SDIO_IRQHandler,
  TIM5_IRQHandler,
  SPI3_IRQHandler,
  UART4_IRQHandler,
  UART5_IRQHandler,
  TIM6_IRQHandler,
  TIM7_IRQHandler,
  DMA2_Channel1_IRQHandler,
  DMA2_Channel2_IRQHandler,
  DMA2_Channel3_IRQHandler,
  DMA2_Channel4_5_IRQHandler,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  reinterpret_cast< void ( * )( void )>( 0xF1E0F85F )
  
};
