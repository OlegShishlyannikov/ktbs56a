#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <functional>
#include <map>

template< typename ... Args > class signal {

public:
  signal() : current_id_( 0 ){}
  signal( signal const & other ) : current_id_( 0 ){}
  
  template< typename T > int connect( T * inst, void( T::* func )( Args ... ))
  {	
    return connect([ = ]( Args ... args ){ 
		( inst->* func )( args ... ); 
	  });
  }

  template< typename T > int connect( T * inst, void( T::* func )( Args ... ) const )
  {
    return connect([ = ]( Args ... args ){
		( inst->* func )( args ... ); 
	  });
  }
  
  int connect( std::function< void( Args ... )> const & slot ) const
  {
    slots_.insert( std::make_pair( ++ current_id_, slot ));
    return current_id_;
  }
  
  void disconnect( int id ) const
  {
    slots_.erase( id );
  }

  void disconnect_all() const
  {
    slots_.clear();
  }

  void emit( Args ... p )
  {
    for( auto it : slots_ ){
	  
      it.second( p ... );

	}
  }

  signal & operator=( signal const & other )
  {
    disconnect_all();
  }

private:

  mutable std::map< int, std::function< void( Args ... )> > slots_;
  mutable int current_id_;
};

#endif /* SIGNAL_HPP */
