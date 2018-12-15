#include "adc_spi_driver.hpp"
#include "usart_driver.hpp"
#include "leds_driver.hpp"
#include "fpga_driver.hpp"
#include "buttons_driver.hpp"
#include "dut_loads_driver.hpp"
#include "gp_timers_driver.hpp"

#define REGISTER_DRIVER( x ) _drivers.insert( std::make_pair( #x, new x ))

typedef std::map< std::string, base_driver * > drivers_map;
static drivers_map _drivers;

base_driver * get_driver_ptr( std::string driver_name )
{
  if( _drivers.find( driver_name ) != _drivers.end() ) return _drivers.at( driver_name );
  else return nullptr;
}

void load_drivers()
{
  REGISTER_DRIVER( fpga_driver );
  REGISTER_DRIVER( usart_driver );
  REGISTER_DRIVER( adc_spi_driver );
  REGISTER_DRIVER( leds_driver );
  REGISTER_DRIVER( buttons_driver );
  REGISTER_DRIVER( dut_loads_driver );
  REGISTER_DRIVER( gp_timers_driver );
}
