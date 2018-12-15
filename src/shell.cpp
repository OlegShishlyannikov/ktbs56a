#include "shell.hpp"
#include "events_worker.hpp"

static std::string padding;
decltype( shell::hardware ) shell::hardware;
decltype( shell::callback_signals ) shell::callback_signals;

static TaskHandle_t shell_task_handle;
static SemaphoreHandle_t shell_semaphore_handle;

shell::shell()
{
  events_worker _events_worker;
  load_drivers();
  shell_semaphore_handle = xSemaphoreCreateCounting( 16, 0 );
  xTaskCreate( &this->shell_task_code_wrapper, (( this->get_name() ) + "_task" ).c_str(), configMINIMAL_STACK_SIZE * 4, nullptr, configAPP_PRIORITY, &shell_task_handle );  
  vTaskStartScheduler();
}

void shell::handle_input_char( const char c )
{
  static std::vector< std::string > history;
  static std::string input_buffer_stream;
  static std::string input_buffer_stream_cache;
  static int history_iter = 0;
  struct cursor_pos { int x = 0; int y = 0; };
  static cursor_pos pos;
  static std::map< std::string, std::function< void( void * )>> ctrl_seq_responses;
	
  std::function< bool( const std::string &, const std::string & )> ends_with;
  std::function< bool( const std::string &, const std::string & )> starts_with;
  std::function< std::vector< std::string >( std::string )> split_string;
  std::function< void( const char )> handle_if_esc_seq;
  std::function< void( const char )> handle_if_ctrl_char;
  std::function< void( const char, std::string * padding )> handle_if_generic_ascii;
  std::function< bool( cursor_pos * )> update_cursor_pos;
  std::function< void( const char )> handle_if_kbd;
  
  ends_with = []( const std::string & str, const std::string & suffix ) -> bool {
    return ( str.size() >= suffix.size() ) && ( !str.compare( str.size() - suffix.size(), suffix.size(), suffix ));
  };

  starts_with = []( const std::string & str, const std::string & preffix ) -> bool {
    return ( str.size() >= preffix.size() ) && ( !str.compare( 0, preffix.size(), preffix ));
  };

  update_cursor_pos = [ this, starts_with, ends_with ]( cursor_pos * _pos ) -> bool {
	std::string esc_seq_str;
	bool ctrl_seq_completed = false;
	bool ctrl_seq_error = false;

	shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_QUERY_CURSOR_POS );
	char received_char;
	
	while( received_char != '\e' ) received_char = *static_cast< char * >( shell::hardware[ STD_IO_DRIVER ].read( portTERMINAL_MAX_DELAY ));

	esc_seq_str += received_char;
	
	while(( !ctrl_seq_completed ) && ( !ctrl_seq_error )){

	  char * p_received_char = static_cast< char * >( shell::hardware[ STD_IO_DRIVER ].read( portTERMINAL_MAX_DELAY ));
	  if( p_received_char != nullptr ){

		received_char = *p_received_char;
		esc_seq_str += received_char;
		ctrl_seq_completed = (( starts_with( esc_seq_str, ASCII_CONTROL_REPORT_CURSOR_POS.substr( 0, 2 ))) && ( ends_with( esc_seq_str, ASCII_CONTROL_REPORT_CURSOR_POS.substr( ASCII_CONTROL_REPORT_CURSOR_POS.length() - 1 ))));
		
	  } else ctrl_seq_error = true;	  
	}
	
	if(( ctrl_seq_completed ) && ( !ctrl_seq_error )){

	  int x, y;
	  if( std::sscanf( esc_seq_str.c_str(), ASCII_CONTROL_REPORT_CURSOR_POS.c_str(), &y, &x ) == 2 ){

		( *_pos ).x = x;
		( *_pos ).y = y;
		return true;
		
	  }	else return false;
	} else return false;
  };

  split_string = []( std::string received_char ) -> std::vector< std::string > {
  	std::vector< std::string > args_vector;
  	std::stringstream queue_stream( received_char ); 
  	args_vector.clear();
	
  	for( std::string str; queue_stream >> str; ) if( args_vector.size() <= MAX_ARGS ) args_vector.push_back( str );

  	return args_vector;
  };

  handle_if_esc_seq = [ this, update_cursor_pos ]( const char c ) -> void {

	if( c == ASCII_CTRL_CH_ESC.at( 0 )){
	  
	  std::string esc_seq_str;
	  esc_seq_str += c;
	  bool ctrl_seq_completed = false;
	  bool ctrl_seq_error = false;
	
	  while(( !ctrl_seq_completed ) && ( !ctrl_seq_error )){

		char received_char;
		char * p_received_char = static_cast< char * >( shell::hardware[ STD_IO_DRIVER ].read( portTERMINAL_MAX_DELAY ));

		if( p_received_char != nullptr ){
		  
		  if( esc_seq_str.length() < MAX_ESC_SEQ_BUFFER_LENGTH ){

			received_char = *p_received_char;
			esc_seq_str += received_char;
			ctrl_seq_completed = ( ctrl_seq_responses.find( esc_seq_str ) != ctrl_seq_responses.end() );
		
		  } else ctrl_seq_error = true;
		} else ctrl_seq_error = true;
	  }

	  std::tuple< std::string *, std::string * > args = { &input_buffer_stream, &padding };
	  if(( ctrl_seq_completed ) && ( !ctrl_seq_error )) ctrl_seq_responses.at( esc_seq_str )( &args );
	} 	
  };
 
  handle_if_kbd = [ this, update_cursor_pos ]( const char c ) -> void {
	
	if( c == ASCII_KBD_CTRL_C ){

	  
	} else if( c == ASCII_KBD_CTRL_A ){

	  
	} else if( c == ASCII_KBD_CTRL_D ){

	  
	} else if( c == ASCII_KBD_CTRL_Q ){

	  
	} else if( c == ASCII_KBD_CTRL_S ){

	  
	} else if( c == ASCII_KBD_CTRL_Z ){

	  
	} else if( c == ASCII_KBD_CTRL_X ){

	  
	} else if( c == ASCII_KBD_CTRL_R ){

	  
	}
  };
  
  handle_if_ctrl_char = [ this, update_cursor_pos ]( const char c ) -> void {

	if(( std::iscntrl( c )) && ( ctrl_seq_responses.find( std::string( 1, c )) != ctrl_seq_responses.end() )){
	  
	  std::tuple< std::string *, std::string * > args = { &input_buffer_stream, &padding };
	  ctrl_seq_responses.at( std::string( 1, c ))( &args );

	}
  };  

  handle_if_generic_ascii = [ this, update_cursor_pos ]( const char c, std::string * padding ) -> void {

	if((( !std::iscntrl( c )) && !( c == ASCII_CTRL_CH_ESC.at( 0 )) && (( std::isalnum( c ) || std::ispunct( c ) || std::isspace( c )))) && ( input_buffer_stream.length() < MAX_BUFFER_LENGTH )){

	  while( !update_cursor_pos( &pos ));

	  int input_iter;
	  int padding_length = padding->length();
	  ( pos.x >= padding_length ) ? input_iter = pos.x - ( padding_length + 1 ) : input_iter = pos.x + 1;
	  
	  try {

		input_buffer_stream.insert( input_iter, sizeof( c ), c );
		input_buffer_stream_cache = input_buffer_stream;
		shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_ERASE_END_OF_LINE + c + ASCII_CONTROL_SAVE_CURSOR + input_buffer_stream.substr( input_iter + 1 ) + ASCII_CONTROL_UNSAVE_CURSOR );

	  } catch( const std::out_of_range & e ){

		input_buffer_stream += c;
		input_buffer_stream_cache = input_buffer_stream;
		shell::hardware[ STD_IO_DRIVER ].write( "%c", c );
	
	  }
	}
  };
  
  ctrl_seq_responses = {
	{ ASCII_CONTROL_CURSOR_UP, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
		
		if( history.size() ){

		  if( history_iter >= 0 ){

			if( history_iter > 0 ) history_iter --;
			
			input_buffer_stream = history[ history_iter ];
			shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS + ASCII_CONTROL_ERASE_END_OF_LINE, pos.y, padding_length + 1 );
			shell::hardware[ STD_IO_DRIVER ].write( input_buffer_stream );
			
		  }
		}
	  }},

	{ ASCII_CONTROL_CURSOR_DOWN, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
		
		if( history.size() ){

		  if( history_iter < history.size() ){

			history_iter ++;

			if( history_iter < history.size() ){
			  
			  input_buffer_stream = history[ history_iter ];
			  shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS + ASCII_CONTROL_ERASE_END_OF_LINE, pos.y, padding_length + 1 );
			  shell::hardware[ STD_IO_DRIVER ].write( input_buffer_stream );

			} else if( history_iter == history.size() ){

			  input_buffer_stream = input_buffer_stream_cache;
			  shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS + ASCII_CONTROL_ERASE_END_OF_LINE, pos.y, padding_length + 1 );
			  shell::hardware[ STD_IO_DRIVER ].write( input_buffer_stream );
			  
			}
		  }
		}
	  }},

	{ ASCII_CONTROL_CURSOR_FORWARD, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		int input_iter;
		
		while( !update_cursor_pos( &pos ));

		( pos.x >= padding_length ) ? input_iter = pos.x - padding_length : input_iter = pos.x;
		
		if( input_iter <= input_buffer_length ) shell::hardware[ STD_IO_DRIVER ].write( "%s", ASCII_CONTROL_CURSOR_FORWARD.c_str() );
	  }},

	{ ASCII_CONTROL_CURSOR_BACKWARD, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		int input_iter;
		
		while( !update_cursor_pos( &pos ));

		( pos.x >= padding_length ) ? input_iter = pos.x - padding_length : input_iter = pos.x;
		
		if( input_iter > 1 ) shell::hardware[ STD_IO_DRIVER ].write( "%s", ASCII_CONTROL_CURSOR_BACKWARD.c_str() );
	  }},
	
	{ ASCII_CONTROL_KBD_CURSOR_CTRL_UP, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
	  }},

	{ ASCII_CONTROL_KBD_CURSOR_CTRL_DOWN, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
	  }},

	{ ASCII_CONTROL_KBD_CURSOR_CTRL_FORWARD, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		int input_iter;
		
		while( !update_cursor_pos( &pos ));

		( pos.x >= padding_length ) ? input_iter = pos.x - padding_length : input_iter = pos.x;
		int punct_space_pos = -1;
		
		for( int i = input_iter; i < input_buffer_stream.length(); i ++ )

		  if( std::isspace( input_buffer_stream[ i ]) ||
			  std::ispunct( input_buffer_stream[ i ])){

			punct_space_pos = i + padding_length + 1;
			break;

		  }

		if( punct_space_pos != -1 ) shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS, pos.y, punct_space_pos );
		else shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS, pos.y, padding_length + input_buffer_length + 1 );
	  }},

	{ ASCII_CONTROL_KBD_CURSOR_CTRL_BACKWARD, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		int input_iter;
		
		while( !update_cursor_pos( &pos ));

		( pos.x >= padding_length ) ? input_iter = pos.x - padding_length : input_iter = pos.x;
		int punct_space_pos = -1;
		
		for( int i = input_iter - 2; i > 0; i -- )

		  if( std::isspace( input_buffer_stream[ i ]) ||
			  std::ispunct( input_buffer_stream[ i ])){

			punct_space_pos = i + padding_length + 1;
			break;
			
		  }

		if( punct_space_pos != -1 ) shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS, pos.y, punct_space_pos );
		else shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS, pos.y, padding_length + 1 );
	  }},

	{ ASCII_CONTROL_CURSOR_LINE_BEGIN, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
		
		shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS.c_str(), pos.y, ( padding_length + 1 ));
	  }},

	{ ASCII_CONTROL_INS, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
	  }},

	{ ASCII_CONTROL_DELETE, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		int input_iter;
		
		while( !update_cursor_pos( &pos ));

		( pos.x >= padding_length ) ? input_iter = pos.x - ( padding_length + 1 ) : input_iter = pos.x + 1;
		
		try {

		  if( input_iter < input_buffer_length ){

			input_buffer_stream.erase( input_iter, 1 );
			input_buffer_stream_cache = input_buffer_stream;
			shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SAVE_CURSOR + ASCII_CONTROL_ERASE_END_OF_LINE + input_buffer_stream.substr( input_iter ) + ASCII_CONTROL_UNSAVE_CURSOR );

		  }
		} catch( const std::out_of_range & e ){

		  return;
		  
		}
	  }},

	{ ASCII_CONTROL_CURSOR_LINE_END, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
		
		shell::hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_CURSOR_POS.c_str(), pos.y, ( padding_length + 1 ) + input_buffer_length );
	  }},

	{ ASCII_CONTROL_CURSOR_PAGE_UP, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
	  }},

	{ ASCII_CONTROL_CURSOR_PAGE_DOWN, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();

		while( !update_cursor_pos( &pos ));
	  }},

	{ ASCII_CTRL_CH_DEL, [ this, update_cursor_pos ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		int input_iter;
		
		while( !update_cursor_pos( &pos ));

		( pos.x >= padding_length ) ? input_iter = pos.x - ( padding_length + 1 ) : input_iter = pos.x + 1;

		try {
		  
		  if( input_iter > 0 ){

			input_buffer_stream.erase( input_iter - 1, 1 );
			input_buffer_stream_cache = input_buffer_stream;
			shell::hardware[ STD_IO_DRIVER ].write( ASCII_CTRL_CH_DEL + ASCII_CONTROL_SAVE_CURSOR + ASCII_CONTROL_ERASE_END_OF_LINE + input_buffer_stream.substr( input_iter - 1 ) + ASCII_CONTROL_UNSAVE_CURSOR );
			
		  }
		} catch( const std::out_of_range & e ){

		  return;
		  
		}
	  }},

	{ ASCII_CTRL_CH_TAB, [ this, update_cursor_pos, starts_with ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		static bool tab_pressed = true;
		tab_pressed = !tab_pressed;
		
		while( !update_cursor_pos( &pos ));

		std::vector< std::string > app_names = *( get_app_names() );
		int matches = 0;
		std::string last_match;
		
		for( unsigned int i = 0; i < app_names.size(); i ++ ) if( starts_with( app_names[ i ], input_buffer_stream )){ matches ++; last_match = app_names[ i ]; }
		
		if( matches ){
			  
		  if( matches == 1 ){

			shell::hardware[ STD_IO_DRIVER ].write( last_match.substr( input_buffer_stream.length() ));
			input_buffer_stream = last_match;
			input_buffer_stream_cache = input_buffer_stream;
			tab_pressed = !tab_pressed;
			
		  } else if(( tab_pressed == true ) || (( matches > 1 ) && ( input_buffer_stream.length() ))){

			shell::hardware[ STD_IO_DRIVER ].write( "\r\n" );
			
			for( unsigned int i = 0; i < app_names.size(); i ++ ) if( starts_with( app_names[ i ], input_buffer_stream )) shell::hardware[ STD_IO_DRIVER ].write( app_names[ i ] + "\t" );

			hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_ATTRS, ASCII_TERM_ATTRS_BRIGHT, ASCII_TERM_ATTRS_BLACK_BG, ASCII_TERM_ATTRS_GREEN_FG );
			hardware[ STD_IO_DRIVER ].write( "\r\n" + padding );
			hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS );  
			hardware[ STD_IO_DRIVER ].write( input_buffer_stream );
		  }
		}
	  }},

	{ ASCII_CTRL_CH_CR, [ this, update_cursor_pos, split_string ]( void * args ){
		std::tuple< std::string *, std::string * > * args_container = static_cast< std::tuple< std::string *, std::string * > * >( args );
		std::string * p_input_buffer = std::get< 0 >( *args_container );
		std::string * p_padding = std::get< 1 >( *args_container );
		int input_buffer_length = p_input_buffer->length();
		int padding_length = p_padding->length();
		std::vector< std::string > argv = split_string( input_buffer_stream );

		while( !update_cursor_pos( &pos ));
		
		if( argv.size() ){

		  if( history.size() < MAX_HISTORY_LENGTH ){

			if( history.size() == 0 ) history.push_back( input_buffer_stream );
			else if( input_buffer_stream != history.at( history.size() - 1 )) history.push_back( input_buffer_stream );

		  }	else {

			if( input_buffer_stream != history.at( history.size() - 1 )){

			  history.erase( history.begin() );
			  history.push_back( input_buffer_stream );
			  
			}
		  }
		  
		  auto app_args = std::make_tuple( &argv, &shell_semaphore_handle, this );
		   
		  if( exec( argv[ 0 ], &app_args )) shell::callback_signals.disconnect_all();

		  argv.clear();
		  input_buffer_stream.clear();
		  input_buffer_stream_cache.clear();

		}

		history_iter = history.size();
		hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_ATTRS, ASCII_TERM_ATTRS_BRIGHT, ASCII_TERM_ATTRS_BLACK_BG, ASCII_TERM_ATTRS_GREEN_FG );
		hardware[ STD_IO_DRIVER ].write( "\r\n%s", padding.c_str() );
		hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS );  
	  }
	}
  };
  
  handle_if_esc_seq( c );
  handle_if_ctrl_char( c );
  handle_if_kbd( c );
  handle_if_generic_ascii( c, &padding );
}

void shell::shell_task_code( void * parameters )
{
  load_applications();
  padding = "root@ktbs56a#: ";
  std::vector< std::string > argv;
  auto app_args = std::make_tuple( &argv, &shell_semaphore_handle, this );
	
  for( std::string app : *( get_startup_applications() )){
	  
	if( exec( app, &app_args )) shell::callback_signals.disconnect_all();

  }

  hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_ATTRS, ASCII_TERM_ATTRS_BRIGHT, ASCII_TERM_ATTRS_BLACK_BG, ASCII_TERM_ATTRS_GREEN_FG );
  hardware[ STD_IO_DRIVER ].write( "\r\n%s", padding.c_str() );
  hardware[ STD_IO_DRIVER ].write( ASCII_CONTROL_SET_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS );
  
  while( true ) this->handle_input_char( *static_cast< char * >( hardware[ STD_IO_DRIVER ].read( portMAX_DELAY )));

  vTaskDelete( nullptr );
}

extern "C"
{
  void vApplicationTickHook(){}
  void vApplicationMallocFailedHook(){}
  void vApplicationIdleHook(){}
  void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName ){}
}
