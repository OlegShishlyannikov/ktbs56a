module spi_slave( rstb, ten, tdata, mlb, ss, sck, sdin, sdout, done, rdata );
  
  input                                                    rstb; // Reset
  input 												   ss; // Chip select
  input 												   sck; // Clock
  input 												   sdin; // Serial data input
  input 												   ten; // Target enable
  input 												   mlb; // LSB( 0 )/MSB( 1 ) first
  input [ 7 : 0 ] 										   tdata; // Transmit data
  output 												   sdout; // Serial data output
  output reg 											   done; // Ready signal
  output reg [ 7 : 0 ] 									   rdata; // Received data

  logic [ 7 : 0 ] 										   treg = 8'h00; 
  logic [ 7 : 0 ] 										   rreg = 8'h00; // Registers to transmit and receive data
  logic [ 3 : 0 ] 										   nb = 4'h0; // Bit counter
  wire 													   sout; // Serial out wire

  assign sout = mlb ? treg[ 7 ] : treg[ 0 ]; // Select first bit
  assign sdout = (( !ss ) && ten ) ? sout : 1'bz; //If 1 => send data, else TRI-STATE sdout

  always @ ( posedge sck or negedge rstb ) begin

	/* If this a falling edge of reset, then reset all */
    if( !rstb )	begin 

	  rreg = 8'h00;  
	  rdata = 8'h00; 
	  done = 1'b0; 
	  nb = 1'b0;
	  
	end	else if( !ss ) begin 
	  
	  if( !mlb ) begin //LSB first, in@msb -> right shift

		rreg = { sdin, rreg[ 7 : 1 ]}; 
	  
	  end else begin //MSB first, in@lsb -> left shift

		rreg = { rreg[ 6 : 0 ], sdin }; 

	  end nb = nb + 1;
	  
	  if( nb != 4'h8 ) done = 1'b0;
	  else  begin 

		rdata = rreg; 
		done = 1'b1; 
		nb = 1'b0; 

	  end
	end
  end

  //send to  sdout
  always @ ( negedge sck or negedge rstb ) begin
	
	if( rstb == 1'b0 )	begin 

	  treg = 8'hff; 

	end	else begin
	  
	  if( !ss ) begin
			
		if( nb == 1'b0 ) treg = tdata;
		else begin
		  
		  if( mlb == 1'b0 ) begin //LSB first, out = lsb -> right shift

			treg = { 1'b1, treg[ 7 : 1 ]}; 

		  end else begin //MSB first, out = msb -> left shift

			treg = { treg[ 6 : 0 ], 1'b1 }; 

		  end			
		end
	  end
	end	
  end
  
endmodule // spi_slave
