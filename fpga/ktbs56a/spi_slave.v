`timescale 1ns / 1ps
module spi_slave( rstb, ten, tdata, mlb, ss, sck, sdin, sdout, done, rdata );
  
  input                                                    rstb; // Reset
  input 												   ss; // Chip select
  input 												   sck; // Clock
  input 												   sdin; // Serial data input
  input 												   ten; // Target enable
  input 												   mlb; // LSB( 0 )/MSB( 1 ) first
  input [ 15 : 0 ] 										   tdata; // Transmit data
  output 												   sdout; // Serial data output
  output reg 											   done; // Ready signal
  output reg [ 15 : 0 ] 									   rdata; // Received data

  reg [ 15 : 0 ] 										   treg = 16'h0000; 
  reg [ 15 : 0 ] 										   rreg = 16'h0000; // Registers to transmit and receive data
  reg [ 4 : 0 ] 										   nb = 5'h00; // Bit counter
  wire 													   sout; // Serial out wire

  assign sout = mlb ? treg[ 15 ] : treg[ 0 ]; // Select first bit
  assign sdout = (( !ss ) && ten ) ? sout : 1'bz; //If 1 => send data, else TRI-STATE sdout

  always @ ( posedge sck or negedge rstb ) begin

	/* If this a falling edge of reset, then reset all */
    if( !rstb )	begin 

	  rreg = 16'h0000;  
	  rdata = 16'h0000; 
	  done = 1'b0; 
	  nb = 5'h00;
	  
	end	else if( !ss ) begin 
	  
	  if( !mlb ) begin //LSB first, in@msb -> right shift

		rreg = { sdin, rreg[ 15 : 1 ]}; 
	  
	  end else begin //MSB first, in@lsb -> left shift

		rreg = { rreg[ 14 : 0 ], sdin }; 

	  end 
	  
	  nb = nb + 5'h01;
	  
	  if( nb != 16 ) done = 1'b0;
	  else  begin 

		rdata = rreg; 
		done = 1'b1; 
		nb = 5'h00; 

	  end
	end
  end

  //send to  sdout
  always @ ( negedge sck or negedge rstb ) begin
	
	if( rstb == 1'b0 )	begin 

	  treg = 16'hffff; 

	end	else begin
	  
	  if( !ss ) begin
			
		if( nb == 5'h00 ) treg = tdata;
		else begin
		  
		  if( mlb == 1'b0 ) begin //LSB first, out = lsb -> right shift

			treg = { 1'b1, treg[ 15 : 1 ]}; 

		  end else begin //MSB first, out = msb -> left shift

			treg = { treg[ 14 : 0 ], 1'b1 }; 

		  end			
		end
	  end
	end	
  end
  
endmodule // spi_slave
