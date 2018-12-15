#ifndef PROPERTY_HPP
#define PROPERTY_HPP

#define READ_ONLY 0x00
#define WRITE_ONLY 0x01
#define READ_WRITE 0x02
#define NO_PERMISSIONS 0x03

#include "signal.hpp"

template< typename T, int permissions > class property
{
public:

  typedef T value_type;
  property( T const & val ) : value_( val ){}
  
  virtual signal< T > const & on_write() const
  {
    return this->on_write_;
  }

  virtual signal< T > const & on_read() const
  {
    return this->on_read_;
  }
  
  virtual void set( T const & value)
  {
	if(( permissions == READ_WRITE ) || ( permissions == WRITE_ONLY )){

	  if( value != this->value_ ){
	  
		this->value_ = value;
		on_write_.emit( this->value_ );

	  }
	} else {

	  __asm__( "nop" );
	  
	}    
  }

  virtual T const get()
  {
	if(( permissions == READ_WRITE ) || ( permissions == READ_ONLY )){

	  on_read_.emit( value_ );
	  return value_;

	} else {

	  return T( 0 );
	  
	}
  }

  virtual void disconnect_auditors()
  {
    on_write_.disconnect_all();
	on_read_.disconnect_all();
  }

  virtual property< T, permissions > & operator=( property< T, permissions > const & rhs )
  {
    set( rhs.value_ );
    return * this;
  }

  virtual property< T, permissions > & operator=( T const & rhs )
  {
    set( rhs );
    return * this;
  }

  virtual operator T()
  {
	return property< T, permissions >::get();
  }
  
  T const & operator()() const
  {
    return property< T, permissions >::get();
  }

private:
  
  signal< T > on_read_;
  signal< T > on_write_;
  T value_;
};

#endif /* PROPERTY_HPP */
