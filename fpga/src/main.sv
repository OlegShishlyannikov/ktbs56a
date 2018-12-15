`timescale 1ns/1ps

module spi( clk, sdo, sdi, ncs, nrst, rdy, busy, tc, rxne, spi_data );

  input logic clk, sdi, ncs, nrst;
  output logic [ 7 : 0 ] spi_data;
  ouput logic sdo;

  logic 				 rdy; /* Ready flag */
  logic 				 busy; /* Busy flag */
  logic 				 tc; /* Transmit complete flag */
  logic 				 rxne; /* Byte received flag */
  
endmodule // spi

module device( spi_data, en, nrst, gleds, rleds, adc_ncs, spi_rdy, spi_tc, spi_busy, spi_rxne );

  /* Global reset & enable signals */
  input logic en, nrst;

  /* Data from SPI module */
  input logic [ 7 : 0 ] spi_data;

  /* Green led outputs */
  output logic [ 34 : 0 ] gleds;

  /* Red led outputs */
  output logic [ 34 : 0 ] rleds;

  /* ADC NCS outputs */
  output logic [ 17 : 0 ] adc_ncs;

  /* SPI flags inputs */  
  input logic 			  spi_rdy, spi_tc, spi_busy, spi_rxne;

  /* Spi module */
  spi spi_inst( .spi_data( spi_data ), .nrst( nrst ), .spi_rdy( rdy ), .spi_busy( busy ), .spi_tc( tc ), .spi_rxne( rxne ));
  
  /* Commands */  
  localparam FPGA_ADC_SELECT_CMD =       "      adc_select";
  localparam FPGA_ADC_UNSELECT_CMD =     "    adc_unselect";
  localparam FPGA_ADC_UNSELECT_ALL_CMD = "adc_unselect_all";
  localparam FPGA_RED_LED_ON_CMD =       "         rled_on";   
  localparam FPGA_RED_LED_OFF_CMD =      "        rled_off";
  localparam FPGA_GREEN_LED_ON_CMD =     "         gled_on";   
  localparam FPGA_GREEN_LED_OFF_CMD =    "        gled_off";
  localparam FPGA_RESET_ADC_NCS =        "   adc_ncs_reset";
  localparam FPGA_RESET_RED_LEDS =       "  red_leds_reset";
  localparam FPGA_RESET_GREEN_LEDS =     "green_leds_reset";

  /* Buffers for receive commands & data from SPI module */
  logic [ 7 : 0 ] 		  command_register [ 15 : 0 ];
  logic [ 7 : 0 ] 		  data_register;
  
  /* Command match signals */
  logic 				  is_fpga_adc_select_cmd;
  logic 				  is_fpga_adc_unselect_cmd;
  logic 				  is_fpga_adc_unselect_all_cmd;
  logic 				  is_fpga_red_led_on_cmd;
  logic 				  is_fpga_red_led_off_cmd;
  logic 				  is_fpga_green_led_on_cmd;
  logic 				  is_fpga_green_led_off_cmd;
  logic 				  is_fpga_reset_adc_ncs;
  logic 				  is_fpga_reset_green_leds;
  logic 				  is_fpga_reset_red_leds;

  assign is_fpga_adc_select_cmd = ( command_register == FPGA_ADC_SELECT_CMD );
  assign is_fpga_adc_unselect_cmd = ( command_register == FPGA_ADC_UNSELECT_CMD );
  assign is_fpga_adc_unselect_all_cmd = ( command_register == FPGA_ADC_UNSELECT_ALL_CMD );
  assign is_fpga_red_led_on_cmd = ( command_register == FPGA_RED_LED_ON_CMD );
  assign is_fpga_red_led_off_cmd = ( command_register == FPGA_RED_LED_OFF_CMD );
  assign is_fpga_green_led_on_cmd = ( command_register == FPGA_GREEN_LED_ON_CMD );
  assign is_fpga_green_led_off_cmd = ( command_register == FPGA_GREEN_LED_OFF_CMD );
  assign is_fpga_reset_adc_ncs = ( command_register == FPGA_RESET_ADC_NCS );
  assign is_fpga_reset_green_leds = ( command_register == FPGA_RESET_GREEN_LEDS );
  assign is_fpga_reset_red_leds = ( command_register == FPGA_RESET_RED_LEDS );

  function [ 17 : 0 ] fpga_func_adc_select;

	input [ 7 : 0 ] 	  channel_number;
	input [ 17 : 0 ] 	  init_value;

	fpga_func_adc_select = ~( 18'b000000000000000001 << ( channel_number / 2 ));

  endfunction // fpga_func_adc_select
  
  function [ 17 : 0 ] fpga_func_adc_unselect;

  endfunction // fpga_func_adc_unselect

  function [ 17 : 0 ] fpga_func_adc_unselect_all;

  endfunction // fpga_func_adc_unselect_all

  function [ 34 : 0 ] fpga_func_red_led_on;

  endfunction // fpga_func_red_led_on
  
  function [ 34 : 0 ] fpga_func_red_led_off;

  endfunction // fpga_func_red_led_off

  function [ 34 : 0 ] fpga_func_green_led_on;

  endfunction // fpga_func_green_led_off

  function [ 34 : 0 ] fpga_func_green_led_off;

  endfunction // fpga_func_green_led_off
  
  function [ 17 : 0 ] fpga_func_reset_adcs_ncs;

	fpga_func_reset_adcs_ncs = 18'b111111111111111111;
	
  endfunction // fpga_func_reset_adcs_ncs

  function [ 34 : 0 ] fpga_func_reset_green_leds;

	fpga_func_reset_green_leds = 35'b00000000000000000000000000000000000;
	
  endfunction // fpga_func_reset_green_leds

  function [ 34 : 0 ] fpga_func_reset_red_leds;

	fpga_func_reset_red_leds = 35'b00000000000000000000000000000000000;
	
  endfunction // fpga_func_reset_red_leds

  function [ 7 : 0 ] fpga_func_reset_command_buffer [ 15 : 0 ];

	fpga_func_reset_command_buffer = "                ";
	
  endfunction // fpga_func_reset_command_buffer  

  /* Initial output values */  
  initial begin

	command_register = fpga_func_reset_command_buffer( command_register );
	rleds = fpga_func_reset_red_leds( rleds );
	gleds = fpga_func_reset_green_leds( gleds );
	adc_ncs = fpga_func_reset_adcs_ncs( adc_ncs );
	
  end
  
  always @ ( posedge clk or negedge nrst ) begin

	if( !nrst ) begin

	  command_register = fpga_func_reset_command_buffer( command_register );
	  rleds = fpga_func_reset_red_leds( rleds );
	  gleds = fpga_func_reset_green_leds( gleds );
	  adc_ncs = fpga_func_reset_adcs_ncs( adc_ncs );
	  
	end else begin

	  
	  
	end
  end
endmodule // device
