// $Header: /tmp/svngaudi/tmp.jEpFh25751/Gaudi/GaudiKernel/GaudiKernel/ObjectContainerBase.h,v 1.4 2001/11/12 08:42:59 mato Exp $
#ifndef GAUDI_OBJECTCONTAINERBASE_H
#define GAUDI_OBJECTCONTAINERBASE_H 1

// Include files
#include "GaudiKernel/Kernel.h"
#include "GaudiKernel/DataObject.h"

// Forward declarations
class ContainedObject;

/** @class ObjectContainerBase ObjectContainerBase.h GaudiKernel/ObjectContainerBase.h
    ObjectContainerBase is the base class for Gaudi container classes. The main motivation
    is to allow contained object to be removed from the container on deletion and also for
    knowing it own index (e.g. distance, key, ...) in the container.
    
    @author Pavel Binko
    @author Pere Mato
*/
class ObjectContainerBase : public DataObject  {

protected:

  /// Constructor
  ObjectContainerBase() { }

  /// Destructor
  virtual ~ObjectContainerBase() { }

public:

  /// Distance of a given object from the beginning of its container
  virtual long index( const ContainedObject* obj ) const = 0;

  /// Pointer to an object of a given distance
  virtual ContainedObject* containedObject( long dist ) const = 0;

  /// Number of objects in the container
  virtual long numberOfObjects() const = 0;

  /** Virtual functions (forwards to the concrete container definitions)
      Add an object to the container. On success the object's index is 
      returned.                                                      */
  virtual long add(ContainedObject* pObject) = 0;

  /** Release object from the container (the pointer will be removed
      from the container, but the object itself will remain alive).
      If the object was fount it's index is returned.                */
  virtual long remove(ContainedObject* value) = 0;

private:
};

#endif    // GAUDI_OBJECTCONTAINERBASE_H