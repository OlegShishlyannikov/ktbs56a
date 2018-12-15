#include "dut_loads_driver.hpp"
#include "events_id.hpp"

static QueueHandle_t dut_loads_driver_queue_handle;
extern QueueHandle_t events_worker_queue_handle;

static bool state = false;
static int calibrating_diff_value = 0;
static int adc_calibrating_value = 0;
static int dac_calibrating_value = 0;
static int dac_val = 0;
static int adc_data = 0;
static int report = 0;
static int from_queue = 0;

dut_loads_driver::dut_loads_driver( base_driver * parent ) : base_driver()
{
  this->init();
  dut_loads_driver_queue_handle = xQueueCreate( DUT_LOADS_DRIVER_QUEUE_SIZE, sizeof( int ));
}

dut_loads_driver::dut_loads_driver( dut_loads_driver & driver )
{
  *this = driver;
}

int dut_loads_driver::init()
{
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = DUT_LOADS_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( DUT_LOADS_GPIO_PORT, GPIO_InitStruct );  
  delete GPIO_InitStruct;

  DAC_InitTypeDef * DAC_InitStruct = new DAC_InitTypeDef;
  DAC_StructInit( DAC_InitStruct );
  DAC_InitStruct->DAC_Trigger = DAC_Trigger_None;
  DAC_InitStruct->DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStruct->DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init( DAC_Channel_1, DAC_InitStruct );
  DAC_Cmd( DAC_Channel_1, ENABLE );
  delete DAC_InitStruct;
  state = true;
  return 0;
}

void dut_loads_driver::write( const char * p_params )
{
  double current = 0.0f;
  char cmd[ 1024 ] = { '\0' };
  const char * msg = static_cast< const char * >( p_params );

  if( std::sscanf( msg, "%lf_ma_%s", &current, cmd ) == 2 ){

	if( !std::strcmp( cmd, "set_current" )){

	  report = this->set_current( current );
	  
	}	
  } else if( std::sscanf( msg, "%s", cmd ) == 1 ){

	if( !std::strcmp( cmd, "calibrate" )){

	  report = this->calibrate();

	} else if( !std::strcmp( cmd, "get_adc_cal_val" )){

	  report = adc_calibrating_value;
	  xQueueSendToBack( dut_loads_driver_queue_handle, &report, 0 );
	  
	} else if( !std::strcmp( cmd, "get_dac_cal_val" )){

	  report = dac_calibrating_value;
	  xQueueSendToBack( dut_loads_driver_queue_handle, &report, 0 );
	  
	} else if( !std::strcmp( cmd, "get_diff_cal_val" )){

	  report = calibrating_diff_value;
	  xQueueSendToBack( dut_loads_driver_queue_handle, &report, 0 );
	  
	} else if( !std::strcmp( cmd, "get_dac_val" )){

	  dac_val = DAC_GetDataOutputValue( DUT_LOADS_DAC_CHANNEL );
	  report  = dac_val;
	  xQueueSendToBack( dut_loads_driver_queue_handle, &report, 0 );
	  
	} else if( !std::strcmp( cmd, "get_adc_val" )){

	  adc_data = this->read_adc();
	  report = adc_data;
	  xQueueSendToBack( dut_loads_driver_queue_handle, &report, 0 );
	  
	}	
  }
}

void * dut_loads_driver::read( TickType_t timeout ) const
{
  if( xQueueReceive( dut_loads_driver_queue_handle, &from_queue, timeout )) return static_cast< void * >( &from_queue );
  else return nullptr;
}

uint16_t dut_loads_driver::read_adc()
{
  std::function< uint16_t( void )> get_adc_value = []( void ) -> uint16_t {
	ADC_RegularChannelConfig( DUT_LOADS_ADC_PERIPH, ADC_Channel_3, 1, ADC_SampleTime_239Cycles5 );
	ADC_SoftwareStartConvCmd( DUT_LOADS_ADC_PERIPH, ENABLE);
	
	while( ADC_GetFlagStatus( DUT_LOADS_ADC_PERIPH, ADC_FLAG_EOC ) == RESET );
	
	return ADC_GetConversionValue( DUT_LOADS_ADC_PERIPH );
  };
  
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = DUT_LOADS_ADC_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( DUT_LOADS_GPIO_PORT, GPIO_InitStruct );
  delete GPIO_InitStruct;
  
  ADC_InitTypeDef * ADC_InitStruct = new ADC_InitTypeDef;
  ADC_StructInit( ADC_InitStruct );
  ADC_InitStruct->ADC_ContinuousConvMode = DISABLE;
  ADC_InitStruct->ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStruct->ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStruct->ADC_Mode = ADC_Mode_Independent;
  ADC_InitStruct->ADC_NbrOfChannel = ADC_Channel_3;
  ADC_InitStruct->ADC_ScanConvMode = DISABLE;
  ADC_Init( DUT_LOADS_ADC_PERIPH, ADC_InitStruct );
  ADC_Cmd( DUT_LOADS_ADC_PERIPH, ENABLE );
  ADC_ResetCalibration( DUT_LOADS_ADC_PERIPH );
  
  while( ADC_GetResetCalibrationStatus( DUT_LOADS_ADC_PERIPH ));

  ADC_StartCalibration( DUT_LOADS_ADC_PERIPH );
  
  while( ADC_GetCalibrationStatus( DUT_LOADS_ADC_PERIPH ));

  delete ADC_InitStruct;
  return get_adc_value();
}

int dut_loads_driver::calibrate()
{  
  this->init();
  DAC_SetChannel1Data( DAC_Align_12b_R, 4095 );
  dac_calibrating_value = DAC_GetDataOutputValue( DAC_Channel_1 );
  adc_calibrating_value = this->read_adc();
  calibrating_diff_value = adc_calibrating_value - dac_calibrating_value;
  this->deinit();
  return 0;
}

int dut_loads_driver::set_current( double current_ma )
{
  if( current_ma == 0.0f ) this->deinit();
  else {

	this->init();
	DAC_SetChannel1Data( DAC_Align_12b_R, static_cast< uint16_t >((( current_ma * this->ref_ohms ) / ( this->ref_voltage * 1000.0f )) * std::pow( 2, 12 )));
  }
  return 0;
}

int dut_loads_driver::deinit()
{
  GPIO_InitTypeDef * GPIO_InitStruct = new GPIO_InitTypeDef;
  GPIO_StructInit( GPIO_InitStruct );
  GPIO_InitStruct->GPIO_Pin = DUT_LOADS_GPIO_PIN;
  GPIO_InitStruct->GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStruct->GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( DUT_LOADS_GPIO_PORT, GPIO_InitStruct );  
  delete GPIO_InitStruct;
  DAC_DeInit();
  state = false;
  return 0;
}
