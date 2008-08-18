// $Id: Property.h,v 1.25 2007/09/25 10:31:43 marcocle Exp $
// ============================================================================
// CVS tag $Name:  $ 
// ============================================================================
#ifndef GAUDIKERNEL_PROPERTY_H
#define GAUDIKERNEL_PROPERTY_H
// ============================================================================
// STD & STL 
// ============================================================================
#include <string>
#include <stdexcept>
// ============================================================================
// Application C++ Class Headers
// ============================================================================
#include "GaudiKernel/Kernel.h"
#include "GaudiKernel/PropertyVerifier.h"
#include "GaudiKernel/Parsers.h"
#include "GaudiKernel/ToStream.h"
#include "GaudiKernel/SmartIF.h"
// ============================================================================

// ============================================================================
class Property   ;
class PropertyCallbackFunctor ;
class IProperty  ;
class IInterface ;
// ============================================================================

// ============================================================================
/// The output operator for friendly printout 
// ============================================================================
std::ostream& 
operator<<
  ( std::ostream&   stream , 
    const Property& prop   ) ;
// ============================================================================
/** @class Property Property.h GaudiKernel/Property.h
 *
 *  Property base class allowing Property* collections to be "homogeneus"
 *
 * @author Paul Maley
 * @author CTDay
 * @author Vanya BELYAEV ibelyaev@physics.syr.edu
 */
class Property 
{
public:
  /// property name 
  const std::string&    name      () const { return m_name             ; } ;
  /// property documentation
  const   std::string&    documentation() const { return m_documentation; };
  /// property type-info
  const std::type_info* type_info () const { return m_typeinfo         ; } ;
  /// property type  
  std::string           type      () const { return m_typeinfo->name() ; } ;  
  ///  export the property value to the destination 
  virtual bool load   (       Property& dest   ) const = 0 ;
  /// import the property value form the source 
  virtual bool assign ( const Property& source )       = 0 ;
public:
  /// value  -> string 
  virtual std::string  toString   ()  const = 0 ;
  /// string -> value 
  virtual StatusCode   fromString ( const std::string& value ) = 0 ;
public:
  /// Call-back functor at reading: the functor is ownered by property!
  const PropertyCallbackFunctor* readCallBack   () const ;
  /// Call-back functor for update: the funtor is ownered by property!
  const PropertyCallbackFunctor* updateCallBack () const ;  
  /// set new callback for reading 
  virtual void  declareReadHandler   ( PropertyCallbackFunctor* pf ) ;
  /// set new callback for update  
  virtual void  declareUpdateHandler ( PropertyCallbackFunctor* pf ) ;
  template< class HT >
  void declareReadHandler  
  ( void ( HT::* MF ) ( Property& ) , HT* instance ) ;
  template< class HT >
  void declareUpdateHandler
  ( void ( HT::* MF ) ( Property& ) , HT* instance ) ;
  /// use the call-back function at reading 
  virtual void useReadHandler   () const ;
  /// use the call-back function at update
  virtual void useUpdateHandler ()       ;
public:
  /// virtual destructor 
  virtual ~Property() ;
  /// clone: "virtual constructor"
  virtual Property*          clone     () const = 0 ;
  /// set the new value for the property name 
  void setName ( const std::string& value ) { m_name = value ; }
  /// set the documentation string
  void setDocumentation( const std::string& documentation ) { 
    m_documentation = documentation; };
  /// the printout of the property value 
  virtual std::ostream& fillStream ( std::ostream& ) const ;
protected:  
  /// constructor from the property name and the type 
  Property
  ( const std::type_info& type      , 
    const std::string&    name = "" ) ;
  /// constructor from the property name and the type 
  Property
  ( const std::string&    name      ,
    const std::type_info& type      ) ;
  /// copy constructor 
  Property           ( const Property& right ) ;
  /// assignement operator 
  Property& operator=( const Property& right ) ;
private:  
  // the default constructor is disabled 
  Property() ;
private:
  // property name 
  std::string              m_name           ;
  // property doc string
  std::string              m_documentation;
  // property type 
  const std::type_info*    m_typeinfo       ;
protected:
  // call back funtor for reading 
  mutable PropertyCallbackFunctor* m_readCallBack   ;
  // call back funtor for update 
  PropertyCallbackFunctor* m_updateCallBack ;
};
// ============================================================================
#include "GaudiKernel/PropertyCallbackFunctor.h"
// ============================================================================
template< class HT >
inline void Property::declareReadHandler  
( void ( HT::* MF ) ( Property& ) , HT* obj )
{ declareReadHandler ( new PropertyCallbackMemberFunctor< HT >( MF , obj ) ) ; }  
// ============================================================================
template< class HT >
inline void Property::declareUpdateHandler
( void ( HT::* MF ) ( Property& ) , HT* obj )
{ declareUpdateHandler ( new PropertyCallbackMemberFunctor< HT >( MF , obj ) ) ; }
// ============================================================================
/** @class PropertyWithValue 
 *  Helper intermediate class which
 *  represent partly implemented property 
 *  with value of concrete type 
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 *  @date 2006-02-27
 */
// ============================================================================
template <class TYPE>
class PropertyWithValue 
  : public Property
{
protected:
  /// the constructor with property name and value 
  PropertyWithValue 
  ( const std::string& name  , 
    TYPE*              value , 
    const bool         owner ) ;
  /// copy constructor (don't let the compiler generate a buggy one)
  PropertyWithValue ( const PropertyWithValue& rhs ) ;
  /// copy constructor from any other type 
  template <class OTHER>
  PropertyWithValue ( const PropertyWithValue<OTHER>& right ) ;
  /// virtual destructor 
  virtual ~PropertyWithValue() ;  
  /// assignement operator 
  PropertyWithValue& operator=( const TYPE& value ) ;
  // assignement operator (don't let the compiler generate a buggy one)
  PropertyWithValue& operator=( const PropertyWithValue& rhs ) ;
  // assignement operator 
  template <class OTHER>
  PropertyWithValue& operator=( const PropertyWithValue<OTHER>& right ) ;
public:
  operator const TYPE&      () const { return value() ;}
  const TYPE& value() const ;
public:
  // NB: abstract : to be implemented when verifier is available 
  virtual bool setValue ( const TYPE&     value  )  = 0  ;
  /// get the value from another property
  virtual bool assign   ( const Property& source )       ;
  /// set value for another property
  virtual bool load     (       Property& dest   ) const ;  
  /// string -> value 
  virtual StatusCode fromString ( const std::string& s )  ;
  /// value  -> string 
  virtual std::string  toString   () const  ;
protected:
  void  i_set ( const TYPE& value ) ;
  TYPE* i_get () const ; 
private:
  TYPE* m_value ;
  bool  m_own ;
};
// ============================================================================
/// the constructor with property name and value 
// ============================================================================
template <class TYPE>
inline PropertyWithValue<TYPE>::PropertyWithValue 
( const std::string& name  , 
  TYPE*              value , 
  const bool         own   ) 
  : Property ( typeid( TYPE ) , name )
  , m_value  ( value ) 
  , m_own    ( own   ) 
{}
// ============================================================================
// copy constructor
// ============================================================================
template <class TYPE>
inline PropertyWithValue<TYPE>::PropertyWithValue 
( const PropertyWithValue& right ) 
  : Property( right         )
  , m_value ( right.m_value )
  , m_own   ( right.own     ) 
{ if ( m_own ) { m_value = new TYPE(right.value()) ; } }
// ============================================================================
// "copy" constructor form any other type 
// ============================================================================
template <class TYPE>
template <class OTHER>
inline PropertyWithValue<TYPE>::PropertyWithValue 
( const PropertyWithValue<OTHER>& right ) 
  : Property( right         )
  , m_value ( right.m_value )
  , m_own   ( right.own     ) 
{ if ( m_own ) { m_value = new TYPE(right.value()) ; } }
// ============================================================================
/// virtual destructor 
// ============================================================================
template <class TYPE>
inline PropertyWithValue<TYPE>::~PropertyWithValue() 
{ if ( m_own ) { delete m_value ; } ;  m_value = 0 ; }
// ============================================================================
/// assignement operator 
// ============================================================================
template <class TYPE>
inline PropertyWithValue<TYPE>&
PropertyWithValue<TYPE>::operator=( const TYPE& value ) 
{
  if ( !setValue ( value ) ) 
  { throw std::out_of_range( "Value not verified" ) ; }
  return *this ;
}
// ============================================================================
/// implementation of Property::assign 
// ============================================================================
template <class TYPE>
inline bool
PropertyWithValue<TYPE>::assign ( const Property& source ) 
{
  // 1) Is the property of "the same" type? 
  const PropertyWithValue<TYPE>* p = 
    dynamic_cast<const PropertyWithValue<TYPE>*>       ( &source ) ;
  if ( 0 != p ) { return setValue ( p->value() ) ; }       // RETURN 
  // 2) Else use the string representation 
  return this->fromString( source.toString() ).isSuccess() ;
}
// ============================================================================
/// implementation of Property::load 
// ============================================================================
template <class TYPE>
inline bool
PropertyWithValue<TYPE>::load( Property& dest ) const 
{
  // gelegate to the 'opposite' method ;
  return dest.assign( *this ) ;
}
// ============================================================================
/// Implementation of PropertyWithValue::toString 
// ============================================================================
template <class TYPE>
inline std::string 
PropertyWithValue<TYPE>::toString () const 
{
  useReadHandler();
  return Gaudi::Utils::toString( *m_value ) ;
}
// ============================================================================
/// Implementation of PropertyWithValue::fromString 
// ============================================================================
template <class TYPE>
inline StatusCode
PropertyWithValue<TYPE>::fromString ( const std::string& source )  
{
  TYPE tmp ;
  StatusCode sc = Gaudi::Parsers::parse ( tmp , source ) ;
  if ( sc.isFailure() ) { return sc ; }
  return setValue ( tmp ) ? StatusCode::SUCCESS : StatusCode::FAILURE ;
}
// ============================================================================
/// full specializataions for std::string
// ============================================================================
template <>
inline std::string 
PropertyWithValue<std::string>::toString () const 
{ return this->value() ; }
// ============================================================================
template <>
inline bool PropertyWithValue<std::string>::assign ( const Property& source ) 
{ return this->fromString( source.toString() ).isSuccess() ; }
// ============================================================================

// ============================================================================
/// get the access to the storage 
// ============================================================================
template <class TYPE>
inline const TYPE& 
PropertyWithValue<TYPE>::value() const 
{ useReadHandler() ; return *m_value ; }
// ============================================================================
// the actual modification of internale data 
// ============================================================================
template <class TYPE>
inline  void PropertyWithValue<TYPE>::i_set 
( const TYPE& value ) { *m_value = value ; }
// ============================================================================
// protected accessor to internal data 
// ============================================================================
template <class TYPE>
inline TYPE* PropertyWithValue<TYPE>::i_get() const { return m_value ; }
// ============================================================================
// assignement operator 
// ============================================================================
template <class TYPE> 
PropertyWithValue<TYPE>& PropertyWithValue<TYPE>::operator=
( const PropertyWithValue& right ) 
{
  // assign the base class 
  Property::operator=( right ) ;
  // assign the value 
  PropertyWithValue<TYPE>::operator=( right.value() ) ;
  return *this ;
}
// ============================================================================
// templated assignement operator 
// ============================================================================
template <class TYPE> 
template <class OTHER>
PropertyWithValue<TYPE>& PropertyWithValue<TYPE>::operator=
( const PropertyWithValue<OTHER>& right ) 
{
  // assign the base class 
  Property::operator=( right ) ;
  // assign the value 
  PropertyWithValue<TYPE>::operator=( right.value() ) ;
  return *this ;
}
// ============================================================================

// ============================================================================
/** @class PropertyWithVerifier 
 *  Helper intermediate class which
 *  represent partly implemented property 
 *  with value of concrete type and concrete verifier 
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 *  @date 2006-02-27
 */
// ============================================================================
template<class TYPE,class VERIFIER> 
class PropertyWithVerifier 
  : public PropertyWithValue<TYPE>
{
protected:
  /// the constructor with property name and value 
  PropertyWithVerifier
  ( const std::string& name     ,  
    TYPE*              value    , 
    const bool         owner    , 
    const VERIFIER&    verifier ) 
    : PropertyWithValue<TYPE> ( name , value , owner ) 
    , m_verifier ( verifier ) 
  {}
  /// virtual destructor 
  virtual ~PropertyWithVerifier() {};
public:
  inline       VERIFIER& verifier()       { return m_verifier ; }
  inline const VERIFIER& verifier() const { return m_verifier ; }
  /// update the value of the property/check the verifier 
  bool set( const TYPE& value ) ;
  /// implementation of PropertyWithValue::setValue 
  virtual bool setValue( const TYPE& value ) { return set( value ) ; }
  /// templated assignement 
  template <class OTHER,class OTHERVERIFIER>
  PropertyWithVerifier& operator=
  ( const PropertyWithVerifier<OTHER,OTHERVERIFIER>& right ) ;
  /// templated assignement 
  template <class OTHER>
  PropertyWithVerifier& operator=( const PropertyWithValue<OTHER>& right ) ;
  /// assignement 
  PropertyWithVerifier& operator=( const TYPE& right ) ;
private:
  /// the default constructot is disabled 
  PropertyWithVerifier() ;
  /// the copy constructor is disabled 
  PropertyWithVerifier( const  PropertyWithVerifier& right );
private:
  VERIFIER m_verifier ;
} ;
// ============================================================================
/// implementaion of PropertyWithVerifier::set
// ============================================================================
template <class TYPE,class VERIFIER>
inline bool 
PropertyWithVerifier<TYPE,VERIFIER>::set( const TYPE& value ) 
{
  /// use verifier!
  if ( !m_verifier.isValid( &value ) ) { return false ; }    
  /// update the value 
  i_set( value ) ;
  /// invoke the update handler
  this->useUpdateHandler() ;
  return true ;
}
// ============================================================================
/// assignement 
// ============================================================================
template <class TYPE,class VERIFIER>
inline PropertyWithVerifier<TYPE,VERIFIER>& 
PropertyWithVerifier<TYPE,VERIFIER>::operator=( const TYPE& right ) 
{
  PropertyWithValue<TYPE>::operator=( right ) ;
  return *this ;
}
// ============================================================================
/// template assignement 
// ============================================================================
template <class TYPE,class VERIFIER>
template <class OTHER>
inline PropertyWithVerifier<TYPE,VERIFIER>& 
PropertyWithVerifier<TYPE,VERIFIER>::operator=( const PropertyWithValue<OTHER>& right ) 
{
  PropertyWithValue<TYPE>::operator=(right) ;
  return *this ;
}
// ============================================================================
/// template assignement 
// ============================================================================
template <class TYPE,class VERIFIER>
template <class OTHER,class OTHERVERIFIER>
inline PropertyWithVerifier<TYPE,VERIFIER>& 
PropertyWithVerifier<TYPE,VERIFIER>::operator=
( const PropertyWithVerifier<OTHER,OTHERVERIFIER>& right ) 
{
  PropertyWithValue<TYPE>::operator=(right) ;
  return *this ;
}
// ============================================================================

// ============================================================================
/** @class SimpleProperty Property.h GaudiKernel/Property.h
 *
 *  SimpleProperty concrete class which implements the full 
 *  Property interface 
 *
 *  @author Paul Maley
 *  @author CTDay
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 */
// ============================================================================
template <class TYPE,class VERIFIER = BoundedVerifier<TYPE> >
class SimpleProperty 
  : public PropertyWithVerifier<TYPE,VERIFIER> 
{
public:
  /// "Almost default" constructor from verifier 
  SimpleProperty 
  ( VERIFIER           verifier = VERIFIER() ) ;
   /// The constructor from the value and verifier (ATLAS needs it!) 
  SimpleProperty 
  ( const TYPE&        value                 , 
    VERIFIER           verifier = VERIFIER() ) ;
  /// The constructor from the name, value and verifier 
  SimpleProperty 
  ( const std::string& name                  , 
    const TYPE&        value                 , 
    VERIFIER           verifier = VERIFIER() ) ;
  /// constructor from other property type 
  template <class OTHER>
  SimpleProperty ( const PropertyWithValue<OTHER>& right ) ;
  /// copy constructor (must be!)
  SimpleProperty ( const SimpleProperty& right ) ;
  /// virtual Destructor
  virtual ~SimpleProperty() ;
  /// implementation of Property::clone 
  virtual SimpleProperty* clone() const ; 
  /// assignement form the value 
  SimpleProperty& operator=( const TYPE& value ) ;
  /// assignement form the other property type 
  template <class OTHER>
  SimpleProperty& operator=( const PropertyWithValue<OTHER>& right ) ;
};
// ============================================================================
/// The constructor from verifier 
// ============================================================================
template <class TYPE,class VERIFIER>
SimpleProperty<TYPE,VERIFIER>::SimpleProperty 
( VERIFIER           verifier ) 
  : PropertyWithVerifier<TYPE,VERIFIER> 
( "" , new TYPE() , true , verifier ) 
{}
// ============================================================================
/// The constructor from the value and verifier 
// ============================================================================
template <class TYPE,class VERIFIER>
SimpleProperty<TYPE,VERIFIER>::SimpleProperty 
( const TYPE&        value    , 
  VERIFIER           verifier ) 
  : PropertyWithVerifier<TYPE,VERIFIER> 
( "" , new TYPE(value) , true , verifier ) 
{}
// ============================================================================
/// The constructor from the name, value and verifier 
// ============================================================================
template <class TYPE,class VERIFIER>
SimpleProperty<TYPE,VERIFIER>::SimpleProperty 
( const std::string& name     , 
  const TYPE&        value    , 
  VERIFIER           verifier ) 
  : PropertyWithVerifier<TYPE,VERIFIER> 
( name , new TYPE(value) , true , verifier ) 
{}
// ============================================================================
/// constructor from other property type 
// ============================================================================
template <class TYPE,class VERIFIER>
template <class OTHER>
SimpleProperty<TYPE,VERIFIER>::SimpleProperty 
( const PropertyWithValue<OTHER>& right ) 
  : PropertyWithVerifier<TYPE,VERIFIER> 
( right.name() , new TYPE( right.value() ) , true , VERIFIER() ) 
{}
// ============================================================================
/// copy constructor (must be!)
// ============================================================================
template <class TYPE,class VERIFIER>
SimpleProperty<TYPE,VERIFIER>::SimpleProperty
( const SimpleProperty& right ) 
  : PropertyWithVerifier<TYPE,VERIFIER> 
( right.name() , new TYPE( right.value() ) , true , right.verifier() ) 
{}
// ============================================================================
/// virtual Destructor
// ============================================================================
template <class TYPE,class VERIFIER>
SimpleProperty<TYPE,VERIFIER>::~SimpleProperty(){}
// ============================================================================
/// implementation of Property::clone 
// ============================================================================
template <class TYPE,class VERIFIER>
inline 
SimpleProperty<TYPE,VERIFIER>*
SimpleProperty<TYPE,VERIFIER>::clone() const 
{ return new SimpleProperty(*this) ; }
// ============================================================================
/// assignement form the value 
// ============================================================================
template <class TYPE,class VERIFIER>
inline 
SimpleProperty<TYPE,VERIFIER>& 
SimpleProperty<TYPE,VERIFIER>::operator=( const TYPE& value ) 
{
  PropertyWithVerifier<TYPE,VERIFIER>::operator=( value ); 
  return *this ; 
}
// ============================================================================
/// assignement form the other property type 
// ============================================================================
template <class TYPE,class VERIFIER>
template <class OTHER>
inline 
SimpleProperty<TYPE,VERIFIER>& 
SimpleProperty<TYPE,VERIFIER>::operator=
( const PropertyWithValue<OTHER>& right ) 
{
  PropertyWithVerifier<TYPE,VERIFIER>::operator=( right ); 
  return *this ; 
}
// ============================================================================

// ============================================================================
/** @class SimplePropertyRef Property.h GaudiKernel/Property.h
 *
 *  SimplePropertyRef templated class 
 *
 *  @author Paul Maley
 *  @author CTDay
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 */
// ============================================================================
template< class TYPE, class VERIFIER = NullVerifier<TYPE> >
class SimplePropertyRef : 
  public PropertyWithVerifier<TYPE,VERIFIER> 
{
public:
  /// Constructor from the name, the value and the verifier
  SimplePropertyRef
  ( const std::string& name                  , 
    TYPE&              value                 ,  ///< NB! non-const reference 
    VERIFIER           verifier = VERIFIER() ) ;
  /// copy constructor (must be!)
  SimplePropertyRef ( const SimplePropertyRef& right ) ;
  /// virtual Destructor
  virtual ~SimplePropertyRef() ;
  /// implementation of Property::clone 
  virtual SimplePropertyRef* clone() const ;
  /// assignement form the value 
  SimplePropertyRef& operator=( const TYPE& value ) ;
  /// assignement form the other property type 
  template <class OTHER>
  SimplePropertyRef& operator=( const PropertyWithValue<OTHER>& right ) ;
private:
  // the default constructor is disabled 
  SimplePropertyRef() ;
};
// ============================================================================
/// Constructor from the name, the value and the verifier
// ============================================================================
template <class TYPE,class VERIFIER>
SimplePropertyRef<TYPE,VERIFIER>::SimplePropertyRef
( const std::string& name     , 
  TYPE&              value    ,  ///< NB! non-const reference 
  VERIFIER           verifier ) 
  : PropertyWithVerifier<TYPE,VERIFIER> ( name , &value , false , verifier ) 
{}
// ============================================================================
/// copy constructor (must be!)
// ============================================================================
template <class TYPE,class VERIFIER>
SimplePropertyRef<TYPE,VERIFIER>::SimplePropertyRef
( const SimplePropertyRef& right ) 
  : PropertyWithVerifier<TYPE,VERIFIER> 
( right.name() , right.i_get() , false , right.verifier() ) 
{}
// ============================================================================
/// virtual Destructor
// ============================================================================
template <class TYPE,class VERIFIER>
SimplePropertyRef<TYPE,VERIFIER>::~SimplePropertyRef(){}
// ============================================================================
/// implementation of Property::clone 
// ============================================================================
template <class TYPE,class VERIFIER>
inline 
SimplePropertyRef<TYPE,VERIFIER>*
SimplePropertyRef<TYPE,VERIFIER>::clone() const
{ return new SimplePropertyRef(*this) ; }
// ============================================================================
/// assignemet from the value
// ============================================================================
template <class TYPE,class VERIFIER>
inline 
SimplePropertyRef<TYPE,VERIFIER>&
SimplePropertyRef<TYPE,VERIFIER>::operator=( const TYPE& value ) 
{
  PropertyWithVerifier<TYPE,VERIFIER>::operator=( value ) ;
  return *this ;
}
// ============================================================================
/// assignement form the other property type 
// ============================================================================
template <class TYPE,class VERIFIER>
template <class OTHER>
inline 
SimplePropertyRef<TYPE,VERIFIER>&
SimplePropertyRef<TYPE,VERIFIER>::operator=
( const PropertyWithValue<OTHER>& right ) 
{
  PropertyWithVerifier<TYPE,VERIFIER>::operator=( right ); 
  return *this ; 
}
// ============================================================================




// Typedef Properties for built-in types
typedef SimpleProperty< bool >              BooleanProperty;
typedef SimpleProperty< char >              CharProperty;
typedef SimpleProperty< signed char >       SignedCharProperty;
typedef SimpleProperty< unsigned char >     UnsignedCharProperty;
typedef SimpleProperty< short >             ShortProperty;
typedef SimpleProperty< unsigned short >    UnsignedShortProperty;
typedef SimpleProperty< int >               IntegerProperty;
typedef SimpleProperty< unsigned int >      UnsignedIntegerProperty;
typedef SimpleProperty< long >              LongProperty;
typedef SimpleProperty< unsigned long >     UnsignedLongProperty;
typedef SimpleProperty< long long>          LongLongProperty;
typedef SimpleProperty< unsigned long long> UnsignedLongLongProperty;
typedef SimpleProperty< float >             FloatProperty;
typedef SimpleProperty< double >            DoubleProperty;
typedef SimpleProperty< long double >       LongDoubleProperty;

typedef SimpleProperty< std::string >       StringProperty;


// Typedef PropertyRefs for built-in types
typedef SimplePropertyRef< bool >                BooleanPropertyRef;
typedef SimplePropertyRef< char >                CharPropertyRef;
typedef SimplePropertyRef< signed char >         SignedCharPropertyRef;
typedef SimplePropertyRef< unsigned char >       UnsignedCharPropertyRef;
typedef SimplePropertyRef< short >               ShortPropertyRef;
typedef SimplePropertyRef< unsigned short >      UnsignedShortPropertyRef;
typedef SimplePropertyRef< int >                 IntegerPropertyRef;
typedef SimplePropertyRef< unsigned int >        UnsignedIntegerPropertyRef;
typedef SimplePropertyRef< long >                LongPropertyRef;
typedef SimplePropertyRef< unsigned long >       UnsignedLongPropertyRef;
typedef SimplePropertyRef< long long >           LongLongPropertyRef;
typedef SimplePropertyRef< unsigned long long >  UnsignedLongLongPropertyRef;
typedef SimplePropertyRef< float >               FloatPropertyRef;
typedef SimplePropertyRef< double >              DoublePropertyRef;
typedef SimplePropertyRef< long double >         LongDoublePropertyRef;

typedef SimplePropertyRef< std::string >         StringPropertyRef;


// Typedef "Arrays" of Properties for built-in types
typedef SimpleProperty< std::vector< bool > >                BooleanArrayProperty;
typedef SimpleProperty< std::vector< char > >                CharArrayProperty;
typedef SimpleProperty< std::vector< signed char > >         SignedCharArrayProperty;
typedef SimpleProperty< std::vector< unsigned char > >       UnsignedCharArrayProperty;
typedef SimpleProperty< std::vector< short > >               ShortArrayProperty;
typedef SimpleProperty< std::vector< unsigned short > >      UnsignedShortArrayProperty;
typedef SimpleProperty< std::vector< int > >                 IntegerArrayProperty;
typedef SimpleProperty< std::vector< unsigned int > >        UnsignedIntegerArrayProperty;
typedef SimpleProperty< std::vector< long > >                LongArrayProperty;
typedef SimpleProperty< std::vector< unsigned long > >       UnsignedLongArrayProperty;
typedef SimpleProperty< std::vector< long long > >           LongLongArrayProperty;
typedef SimpleProperty< std::vector< unsigned long long > >  UnsignedLongLongArrayProperty;
typedef SimpleProperty< std::vector< float > >               FloatArrayProperty;
typedef SimpleProperty< std::vector< double > >              DoubleArrayProperty;
typedef SimpleProperty< std::vector< long double > >         LongDoubleArrayProperty;

typedef SimpleProperty< std::vector< std::string > >         StringArrayProperty;


// Typedef "Arrays" of PropertyRefs for built-in types
typedef SimplePropertyRef< std::vector< bool > >                 BooleanArrayPropertyRef;
typedef SimplePropertyRef< std::vector< char > >                 CharArrayPropertyRef;
typedef SimplePropertyRef< std::vector< signed char > >          SignedCharArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned char > >        UnsignedCharArrayPropertyRef;
typedef SimplePropertyRef< std::vector< short > >                ShortArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned short > >       UnsignedShortArrayPropertyRef;
typedef SimplePropertyRef< std::vector< int > >                  IntegerArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned int > >         UnsignedIntegerArrayPropertyRef;
typedef SimplePropertyRef< std::vector< long > >                 LongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned long > >        UnsignedLongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< long long > >            LongLongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned long long > >   UnsignedLongLongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< float > >                FloatArrayPropertyRef;
typedef SimplePropertyRef< std::vector< double > >               DoubleArrayPropertyRef;
typedef SimplePropertyRef< std::vector< long double > >          LongDoubleArrayPropertyRef;

typedef SimplePropertyRef< std::vector< std::string > >          StringArrayPropertyRef;

// pre-declaration is sufficient here
class GaudiHandleBase;

class GaudiHandleProperty : public Property {
public:
   GaudiHandleProperty( const std::string& name, GaudiHandleBase& ref );

  GaudiHandleProperty& operator=( const GaudiHandleBase& value );
  
  virtual GaudiHandleProperty* clone() const;

  virtual bool load( Property& destination ) const;
  
  virtual bool assign( const Property& source );

  virtual std::string toString() const;
  
  virtual StatusCode fromString(const std::string& s);

  const GaudiHandleBase& value() const;

  bool setValue( const GaudiHandleBase& value );

private:
  /** Pointer to the real property. Reference would be better, but Reflex does not
      support references yet */
   GaudiHandleBase* m_pValue;
};

// implementation in header file only where the GaudiHandleBase class
// definition is not needed. The rest goes into the .cpp file.
// The goal is to decouple the header files, to avoid that the whole
// world depends on GaudiHandle.h
inline GaudiHandleProperty& GaudiHandleProperty::operator=( const GaudiHandleBase& value ) {
      setValue( value );
      return *this;
}
  
inline GaudiHandleProperty* GaudiHandleProperty::clone() const {
  return new GaudiHandleProperty( *this ); 
}

inline bool GaudiHandleProperty::load( Property& destination ) const {
  return destination.assign( *this );
}

inline bool GaudiHandleProperty::assign( const Property& source ) {
  return fromString( source.toString() ).isSuccess();
}

inline const GaudiHandleBase& GaudiHandleProperty::value() const {
  useReadHandler();
  return *m_pValue;
}


// pre-declaration is sufficient here
class GaudiHandleArrayBase;

class GaudiHandleArrayProperty : public Property {
public:

  GaudiHandleArrayProperty( const std::string& name, GaudiHandleArrayBase& ref );

  GaudiHandleArrayProperty& operator=( const GaudiHandleArrayBase& value );

  virtual GaudiHandleArrayProperty* clone() const;

  virtual bool load( Property& destination ) const;

  virtual bool assign( const Property& source );

  virtual std::string toString() const;
  
  virtual StatusCode fromString(const std::string& s);

  const GaudiHandleArrayBase& value() const;

  bool setValue( const GaudiHandleArrayBase& value );

private:
  /** Pointer to the real property. Reference would be better, but Reflex does not
      support references yet */
   GaudiHandleArrayBase* m_pValue;

};

// implementation in header file only where the GaudiHandleBase class
// definition is not needed. The rest goes into the .cpp file.
// The goal is to decouple the header files, to avoid that the whole
// world depends on GaudiHandle.h
inline GaudiHandleArrayProperty& GaudiHandleArrayProperty::operator=( const GaudiHandleArrayBase& value ) {
  setValue( value );
  return *this;
}

inline GaudiHandleArrayProperty* GaudiHandleArrayProperty::clone() const {
  return new GaudiHandleArrayProperty( *this ); 
}

inline bool GaudiHandleArrayProperty::load( Property& destination ) const {
  return destination.assign( *this );
}

inline bool GaudiHandleArrayProperty::assign( const Property& source ) {
  return fromString( source.toString() ) != 0;
}

inline const GaudiHandleArrayBase& GaudiHandleArrayProperty::value() const {
  useReadHandler();
  return *m_pValue;
}


namespace Gaudi
{
  namespace Utils 
  {
    // ========================================================================
    /** simple function which check the existence of the property with 
     *  the given name. 
     *  
     *  @code 
     * 
     *  const IProperty* p = ... ;
     *  
     *  const bool = hasProperty( p , "Context" ) ;
     * 
     *  @endcode 
     *
     *  @param  p    pointer to IProperty object 
     *  @param  name property name (case insensitive) 
     *  @return true if "p" has a property with such name 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    bool hasProperty ( const IProperty*   p , const std::string& name ) ;
    // ========================================================================
    /** simple function which check the existence of the property with 
     *  the given name. 
     *  
     *  @code 
     * 
     *  IInterface* p = .
     *  
     *  const bool = hasProperty( p , "Context" ) ;
     * 
     *  @endcode 
     *
     *  @param  p    pointer to IInterface   object (any component) 
     *  @param  name property name (case insensitive) 
     *  @return true if "p" has a property with such name 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    bool hasProperty ( const IInterface*   p , const std::string& name ) ;
    // ========================================================================
    /** simple function which gets the property with given name 
     *  from the component 
     *  
     *  @code 
     * 
     *  const IProperty* p = ... ;
     *  
     *  const Property* pro = getProperty( p , "Context" ) ;
     * 
     *  @endcode 
     *
     *  @param  p    pointer to IProperty object 
     *  @param  name property name (case insensitive) 
     *  @return property with the given name (if exists), NULL otherwise
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    Property* getProperty 
    ( const IProperty*   p , const std::string& name ) ;
    // ========================================================================
    /** simple function which gets the property with given name 
     *  from the component 
     *  
     *  @code 
     * 
     *  const IInterface* p = ... ;
     *  
     *  const Property* pro = getProperty( p , "Context" ) ;
     * 
     *  @endcode 
     *
     *  @param  p    pointer to IInterface object 
     *  @param  name property name (case insensitive) 
     *  @return property with the given name (if exists), NULL otherwise
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    Property* getProperty 
    ( const IInterface*   p , const std::string& name ) ;
    // ========================================================================
    /** check  the property by name from  the list of the properties
     *  
     *  @code
     * 
     *   IJobOptionsSvc* svc = ... ;
     *  
     *   const std::string client = ... ;
     * 
     *  // get the property:
     *  bool context = 
     *      hasProperty ( svc->getProperties( client ) , "Context" ) 
     *
     *  @endcode 
     *
     *  @see IJobOptionsSvc 
     *
     *  @param  p    list of properties 
     *  @param  name property name (case insensitive) 
     *  @return true if the property exists 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    bool hasProperty 
    ( const std::vector<const Property*>* p    , 
      const std::string&                  name ) ;
    // ========================================================================
    /** get the property by name from  the list of the properties
     *  
     *  @code
     * 
     *   IJobOptionsSvc* svc = ... ;
     *  
     *   const std::string client = ... ;
     * 
     *  // get the property:
     *  const Property* context = 
     *      getProperty ( svc->getProperties( client ) , "Context" ) 
     *
     *  @endcode 
     *
     *  @see IJobOptionsSvc 
     *
     *  @param  p    list of properties 
     *  @param  name property name (case insensitive) 
     *  @return property with the given name (if exists), NULL otherwise
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    const Property* getProperty 
    ( const std::vector<const Property*>* p    , 
      const std::string&                  name ) ;
    // ========================================================================
    /** simple function to set the property of the given object from the value 
     * 
     *  @code
     * 
     *  IProperty* component = ... ;
     *  
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( componet , "Data" ,  data ) ;
     * 
     *  @endcode 
     *  
     *  Note: the interface IProperty allows setting of the properties either 
     *        directly from other properties or from strings only
     *
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     value of the property
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    template <class TYPE>
    StatusCode setProperty 
    ( IProperty*         component  ,  
      const std::string& name       , 
      const TYPE&        value      , 
      const std::string& doc        ) ;
    // ========================================================================
    /** simple function to set the property of the given object from the value 
     * 
     *  @code
     * 
     *  IProperty* component = ... ;
     *  
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( componet , "Data" ,  data ) ;
     * 
     *  @endcode 
     *  
     *  Note: the interface IProperty allows setting of the properties either 
     *        directly from other properties or from strings only
     *
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     value of the property
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    template <class TYPE>
    StatusCode setProperty 
    ( IProperty*         component  ,  
      const std::string& name       , 
      const TYPE&        value      ) 
    { return setProperty ( component , name , value , std::string() ) ; }
    // ========================================================================
    /** the full specialization of the 
     *  previous method setProperty( IProperty, std::string, const TYPE&) 
     *  for standard strings 
     *
     *  @param component component which needs to be configured 
     *  @param name      name of the property 
     *  @param value     value of the property
     *  @param doc       the new documentation string 
     *
     *  @see IProperty 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13 
     */
    StatusCode setProperty 
    ( IProperty*         component  , 
      const std::string& name       , 
      const std::string& value      ,
      const std::string& doc   = "" ) ;
    // ========================================================================
    /** the full specialization of the 
     *  method setProperty( IProperty, std::string, const TYPE&) 
     *  for C-strings 
     *
     *  @param component component which needs to be configured 
     *  @param name      name of the property 
     *  @param value     value of the property
     *  @param doc       the new documentation string 
     *
     *  @see IProperty 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13 
     */
    StatusCode setProperty 
    ( IProperty*         component  , 
      const std::string& name       , 
      const char*        value      ,
      const std::string& doc   = "" ) ;
    // ========================================================================
    /** the full specialization of the 
     *  method setProperty( IProperty, std::string, const TYPE&) 
     *  for C-arrays 
     *
     *  @param component component which needs to be configured 
     *  @param name      name of the property 
     *  @param value     value of the property
     *  @param doc       the new documentation string 
     *
     *  @see IProperty 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13 
     */
    template <unsigned N>
    StatusCode setProperty 
    ( IProperty*           component , 
      const std::string&   name      , 
      const char         (&value)[N] ,
      const std::string& doc   = ""  ) 
    { 
      if ( 0 == component                    ) { return StatusCode::FAILURE ; }
      const std::string val = std::string ( value , value + N ) ;
      return setProperty ( component , name , val , doc ) ; 
    }
    // ========================================================================
    /** simple function to set the property of the given object from the value 
     *   
     *  @code
     * 
     *  IProperty* component = ... ;
     *  
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  data ) ;
     *
     *  std::map<std::string,double> cuts = ... ;
     *  sc = setProperty ( component , "Cuts" , cuts ) ;
     *
     *  std::map<std::string,std::string> dict = ... ;
     *  sc = setProperty ( component , "Dictionary" , dict ) ;
     * 
     *  @endcode 
     *  
     *  Note: the native interface IProperty allows setting of the 
     *        properties either directly from other properties or 
     *        from strings only
     *
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     value of the property
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    template <class TYPE>
    StatusCode setProperty 
    ( IProperty*         component  ,  
      const std::string& name       , 
      const TYPE&        value      ,
      const std::string& doc        ) 
    {
      if ( 0 == component ) { return StatusCode::FAILURE ; }   // RETURN
      if ( !hasProperty ( component , name ) ) { return StatusCode::FAILURE ; }
      const std::string val = Gaudi::Utils::toString ( value ) ;
      return Gaudi::Utils::setProperty ( component , name , val , doc ) ;                        
    }
    // ========================================================================
    /** simple function to set the property of the given object from another 
     *  property 
     *   
     *  @code
     * 
     *  IProperty* component = ... ;
     *  
     *  const Property* prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param property  the property 
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    StatusCode 
    setProperty 
    ( IProperty*         component , 
      const std::string& name      , 
      const Property*    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another 
     *  property 
     *   
     *  @code
     * 
     *  IProperty* component = ... ;
     *  
     *  const Property& prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param property  the property 
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    StatusCode 
    setProperty 
    ( IProperty*         component , 
      const std::string& name      , 
      const Property&    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another 
     *  property 
     *   
     *  @code
     * 
     *  IProperty* component = ... ;
     *  
     *  SimpleProperty<std::vector<int> > m_data = ... ;
     *
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     the property 
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */    
    template <class TYPE>
    StatusCode 
    setProperty 
    ( IProperty*                  component , 
      const std::string&          name      , 
      const SimpleProperty<TYPE>& value     , 
      const std::string&          doc = ""  ) 
    {
      const Property* property = &value ;
      return setProperty ( component , name , property , doc ) ;
    }  
    // ========================================================================
    /** simple function to set the property of the given object from the value 
     * 
     *  @code
     * 
     *  IInterface* component = ... ;
     *  
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  data ) ;
     * 
     *  @endcode 
     *
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     value of the property
     * @param doc       the new documentation string 
     *
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    template <class TYPE>
    StatusCode setProperty 
    ( IInterface*        component ,  
      const std::string& name      , 
      const TYPE&        value     ,  
      const std::string& doc       ) ;
    // ========================================================================
    /** simple function to set the property of the given object from the value 
     * 
     *  @code
     * 
     *  IInterface* component = ... ;
     *  
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  data ) ;
     * 
     *  @endcode 
     *
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     value of the property
     *
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    template <class TYPE>
    StatusCode setProperty 
    ( IInterface*        component ,  
      const std::string& name      , 
      const TYPE&        value     ) 
    {
      return setProperty ( component , name , value , std::string() ) ;
    }
    // ========================================================================
    /** the full specialization of the 
     *  method setProperty( IInterface , std::string, const TYPE&) 
     *  for standard strings 
     *
     *  @param component component which needs to be configured 
     *  @param name      name of the property 
     *  @param value     value of the property
     *  @param doc       the new documentation string 
     *
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13 
     */
    StatusCode setProperty 
    ( IInterface*        component , 
      const std::string& name      , 
      const std::string& value     , 
      const std::string& doc  = "" ) ;
    // ========================================================================
    /** the full specialization of the 
     *  method setProperty( IInterface , std::string, const TYPE&) 
     *  for C-strings 
     *
     *  @param component component which needs to be configured 
     *  @param name      name of the property 
     *  @param value     value of the property
     *  @param doc       the new documentation string 
     *
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13 
     */
    StatusCode setProperty 
    ( IInterface*        component , 
      const std::string& name      , 
      const char*        value     ,
      const std::string& doc  = "" ) ;
    // ========================================================================
    /** the full specialization of the 
     *  method setProperty( IInterface, std::string, const TYPE&) 
     *  for C-arrays 
     *
     *  @param component component which needs to be configured 
     *  @param name      name of the property 
     *  @param value     value of the property
     *  @param doc       the new documentation string 
     *
     *  @see IProperty 
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13 
     */
    template <unsigned N>
    StatusCode setProperty 
    ( IInterface*          component , 
      const std::string&   name      , 
      const char         (&value)[N] ,
      const std::string& doc  = ""   ) 
    { 
      if ( 0 == component ) { return StatusCode::FAILURE ; }
      const std::string val = std::string ( value , value + N ) ;
      return setProperty ( component , name , val , doc ) ; 
    }
    // ========================================================================
    /** simple function to set the property of the given object from the value 
     * 
     *  @code
     * 
     *  IInterface* component = ... ;
     *  
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  data ) ;
     * 
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     value of the property
     *  @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    template <class TYPE>
    StatusCode setProperty 
    ( IInterface*        component ,  
      const std::string& name      , 
      const TYPE&        value     ,
      const std::string& doc  = "" ) 
    {
      if ( 0 == component ) { return StatusCode::FAILURE ; }
      SmartIF<IProperty> property ( component ) ;
      if ( !property      ) { return StatusCode::FAILURE ; }
      return setProperty ( property , name , value , doc ) ;
    }
    // ========================================================================
    /** simple function to set the property of the given object from another 
     *  property 
     *   
     *  @code
     * 
     *  IInterface* component = ... ;
     *  
     *  const Property* prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param property  the property 
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    StatusCode 
    setProperty 
    ( IInterface*        component , 
      const std::string& name      , 
      const Property*    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another 
     *  property 
     *   
     *  @code
     * 
     *  IInterface* component = ... ;
     *  
     *  const Property& prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param property  the property 
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */
    StatusCode 
    setProperty 
    ( IInterface*        component , 
      const std::string& name      , 
      const Property&    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another 
     *  property 
     *   
     *  @code
     * 
     *  IInterface* component = ... ;
     *  
     *  SimpleProperty<std::vector<int> > m_data = ... ;
     *
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode 
     *  
     * @param component component which needs to be configured 
     * @param name      name of the property 
     * @param value     the property 
     * @param doc       the new documentation string 
     *
     * @see IProperty 
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13 
     */    
    template <class TYPE>
    StatusCode 
    setProperty 
    ( IInterface*                 component , 
      const std::string&          name      , 
      const SimpleProperty<TYPE>& value     , 
      const std::string&          doc = ""  ) 
    {
      const Property* property = &value ;
      return setProperty ( component , name , property , doc ) ;
    }  
    // ========================================================================    
  } // end of namespace Gaudi::Utils 
} // end of namespace Gaudi 

  
// ============================================================================
// The END 
// ============================================================================
#endif // GAUDIKERNEL_PROPERTY_H
// ============================================================================