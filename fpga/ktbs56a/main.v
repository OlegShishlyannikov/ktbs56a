`timescale 1ns / 1ps
`include "spi_slave.v"

module main( adc_ncs, rleds, gleds, spi_din, spi_sck, spi_ncs, nrst, en );

	input wire spi_din;
	input wire spi_ncs;
	input wire spi_sck;
	input wire nrst;
	input wire en;
	output reg [ 34 : 0 ] rleds;
	output reg [ 34 : 0 ] gleds;
	output reg [ 17 : 0 ] adc_ncs;
	
	wire mlb, spi_done, t_enable;
	wire [ 15 : 0 ] spi_tdata;
	wire [ 15 : 0 ] spi_rdata;
	wire spi_dout;
	
	localparam green_led_on_command = "1";
	localparam green_led_off_command = "2";
	localparam red_led_on_command = "3";
	localparam red_led_off_command = "4";
	localparam adc_select_command = "5";
	localparam adc_deselect_command = "6";
	localparam adc_deselect_all_command = "7";
	localparam reset_all_command = "8";
	localparam report_query
	
	reg [ 15 : 0 ] cmd_data_buffer;
	wire [ 7 : 0 ] cmd_bus;
	wire [ 7: 0 ] data_bus;
	assign cmd_bus = cmd_data_buffer[ 15 : 8 ];
	assign data_bus = cmd_data_buffer[ 7 : 0 ];
	
	wire green_led_on_cmd_rcvd;
	wire green_led_off_cmd_rcvd;
	wire red_led_on_cmd_rcvd;
	wire red_led_off_cmd_rcvd;
	wire adc_select_cmd_rcvd;
	wire adc_deselect_cmd_rcvd;
	wire adc_deselect_all_cmd_rcvd;
	wire reset_all_cmd_rcvd;
	
	assign green_led_on_cmd_rcvd = ( cmd_bus == green_led_on_command );
	assign green_led_off_cmd_rcvd = ( cmd_bus == green_led_off_command );
	assign red_led_on_cmd_rcvd = ( cmd_bus == red_led_on_command );
	assign red_led_off_cmd_rcvd = ( cmd_bus == red_led_off_command );
	assign adc_select_cmd_rcvd = ( cmd_bus == adc_select_command );
	assign adc_deselect_cmd_rcvd = ( cmd_bus == adc_deselect_command );
	assign adc_deselect_all_cmd_rcvd = ( cmd_bus == adc_deselect_all_command );
	assign reset_all_cmd_rcvd = ( cmd_bus == reset_all_command );
	
	wire [ 7 : 0 ] commands_bus;
	wire bus_edges;	
	assign commands_bus = { green_led_on_cmd_rcvd, green_led_off_cmd_rcvd, red_led_on_cmd_rcvd, 
		red_led_off_cmd_rcvd, adc_select_cmd_rcvd, adc_deselect_cmd_rcvd, adc_deselect_all_cmd_rcvd, reset_all_cmd_rcvd };
	assign bus_edges = green_led_on_cmd_rcvd || green_led_off_cmd_rcvd || red_led_on_cmd_rcvd || 
		red_led_off_cmd_rcvd || adc_select_cmd_rcvd || adc_deselect_cmd_rcvd || adc_deselect_all_cmd_rcvd || reset_all_cmd_rcvd;
	
	assign mlb = 1'b1;
	assign t_enable = 1'b1;
	
	initial begin
		adc_ncs = 18'b111111111111111111;
		rleds = 35'b00000000000000000000000000000000000;
		gleds = 35'b00000000000000000000000000000000000;
	end
	
spi_slave spi_slave_inst( 
	.rstb( nrst ), 
	.ten( t_enable ), 
	.tdata( spi_tdata ), 
	.mlb( mlb ), 
	.ss( spi_ncs ), 
	.sck( spi_sck ), 
	.sdin( spi_din ), 
	.sdout( spi_dout ), 
	.done( spi_done ), 
	.rdata( spi_rdata ));
	
	function [ 34 : 0 ] green_led_on_func;
		input [ 7 : 0 ] led_num;
		input [ 34 : 0 ] initial_value;
		green_led_on_func = initial_value | ( 1 << led_num );
	endfunction
	
	function [ 34 : 0 ] green_led_off_func;
		input [ 7 : 0 ] led_num;
		input [ 34 : 0 ] initial_value;
		green_led_off_func = initial_value & ~( 1 << led_num );
	endfunction
	
	function [ 34 : 0 ] red_led_on_func;
		input [ 7 : 0 ] led_num;
		input [ 34 : 0 ] initial_value;
		red_led_on_func = initial_value | ( 1 << led_num );
	endfunction
	
	function [ 34 : 0 ] red_led_off_func;
		input [ 7 : 0 ] led_num;
		input [ 34 : 0 ] initial_value;
		red_led_off_func = initial_value & ~( 1 << led_num );
	endfunction
	
	function [ 17 : 0 ] adc_select_func;
		input [ 7 : 0 ] adc_num;
		input [ 17 : 0 ] initial_value;
		adc_select_func = ~( 1 << adc_num );
	endfunction
	
	function [ 17 : 0 ] adc_deselect_func;
		input [ 7 : 0 ] adc_num;
		input [ 17 : 0 ] initial_value;
		adc_deselect_func = initial_value | ( 1 << adc_num );
	endfunction
	
	function [ 17 : 0 ] adc_deselect_all_func;
		input [ 7 : 0 ] adc_num;
		input [ 17 : 0 ] initial_value;
		adc_deselect_all_func = 18'b111111111111111111;
	endfunction

	always @ ( posedge en ) begin
		if( green_led_on_cmd_rcvd ) begin
		
			gleds <= green_led_on_func( data_bus, gleds );
		
		end else if( green_led_off_cmd_rcvd ) begin
		
			gleds <= green_led_off_func( data_bus, gleds );
		
		end else if( red_led_on_cmd_rcvd ) begin
			
			rleds <= red_led_on_func( data_bus, rleds );	
			
		end else if( red_led_off_cmd_rcvd ) begin
		
			rleds <= red_led_off_func( data_bus, rleds );
		
		end else if( adc_select_cmd_rcvd ) begin
		
			adc_ncs <= adc_select_func( data_bus, adc_ncs );
		
		end else if( adc_deselect_cmd_rcvd ) begin 
		
			adc_ncs <= adc_deselect_func( data_bus, adc_ncs );
		
		end else if( adc_deselect_all_cmd_rcvd ) begin
		
			adc_ncs <= adc_deselect_all_func( data_bus, adc_ncs );
		
		end else if( reset_all_cmd_rcvd ) begin
		
			adc_ncs <= 18'b111111111111111111;
			rleds <= 35'b00000000000000000000000000000000000;
			gleds <= 35'b00000000000000000000000000000000000;
		
		end
	end
	
	always @ ( posedge spi_done ) begin
		cmd_data_buffer <= spi_rdata;	
	end
endmodule