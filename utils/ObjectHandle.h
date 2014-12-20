/*
 * ObjectHandle.h
 *
 *  Created on: 2009-3-31
 *      Author: fenggang
 */

#ifndef OBJECTHANDLE_H_
#define OBJECTHANDLE_H_

#include <boost/smart_ptr.hpp>
#include <boost/atomic.hpp>
#ifdef __Use_Boost_Malloc__
#include <boost/pool/singleton_pool.hpp>
#endif
using namespace std;


#if 0
#define __Use_Boost_Malloc__
#endif

template<typename IDType = long>
class AtomicGuard
{
public:
	typedef boost::shared_ptr<AtomicGuard> AtomicGuardPtr;
	typedef boost::weak_ptr<AtomicGuard> AtomicGuardWeakPtr;


	typedef boost::atomic<IDType> AtomicType;
	typedef boost::shared_ptr<AtomicType> AtomicTypePtr;
	typedef boost::weak_ptr<AtomicType> AtomicTypeWeakPtr;

	AtomicGuard(AtomicTypeWeakPtr renter_count,IDType value = 1)
		:counter_(renter_count),value_(value)
	{
		if(!counter_.expired())
		{
			AtomicTypePtr spCount = counter_.lock();
			if(spCount)
			{
				if(value_ == 1)
					(*spCount)++;
				else
					(*spCount).fetch_add(value_);
			}
		}
	}

	static AtomicTypePtr CreateCounter(IDType value = 0)
	{
		return boost::make_shared<AtomicType>(value);
	}

	static AtomicGuardPtr CreateGuard(AtomicTypeWeakPtr renter_count,IDType value = 1)
	{
		return  boost::make_shared<AtomicGuard>(renter_count,value);
	}




	~AtomicGuard()
	{
		if(!counter_.expired())
		{
			AtomicTypePtr spCount = counter_.lock();
			if(spCount)
			{
				if(value_ == 1)
					(*spCount)--;
				else
					(*spCount).fetch_sub(value_);
			}
		}
	}
	operator IDType()
	{
		if(!counter_.expired())
		{
			return *counter_.lock();
		}
		else
			return -1;
	}

	IDType get_increment()
	{
		return value_;
	}
private:
	AtomicTypeWeakPtr counter_;
	IDType value_;
};






template<typename derived, typename IDType = long>
class ObjectHandle 
{
public:
	static boost::atomic<IDType> objCounter,objSequence;
	IDType m_ID;
	ObjectHandle();
	virtual ~ObjectHandle();
	static IDType getObjectCount();
	static IDType getMaxObjectID();
	IDType getObjectID() const;

	operator IDType() const;

#ifdef __Use_Boost_Malloc__
	typedef boost::singleton_pool<derived,sizeof(char)> alloc;

	void *operator new(size_t);
	void operator delete(void *,size_t);

	void *operator new[](size_t);
	void operator delete[](void *);
#endif
};

template<typename derived, typename IDType>
boost::atomic<IDType> ObjectHandle<derived,IDType>::objCounter(0);

template<typename derived, typename IDType>
boost::atomic<IDType> ObjectHandle<derived,IDType>::objSequence(0);


template<typename derived, typename IDType>
ObjectHandle<derived,IDType>::ObjectHandle()
{
	++objCounter;
	++objSequence;
	m_ID = objSequence;
}

template<typename derived, typename IDType>
ObjectHandle<derived,IDType>::~ObjectHandle()
{
	--objCounter;
#ifdef __Use_Boost_Malloc__
	if((objSequence - objCounter) % 64 == 0 || objCounter == 0)
		alloc::release_memory();
#endif
}

template<typename derived, typename IDType>
ObjectHandle<derived,IDType>::operator IDType() const
{
	return m_ID;
}

#ifdef __Use_Boost_Malloc__

template<typename derived, typename IDType>
void *ObjectHandle<derived,IDType>::operator new(size_t n)
{
	return alloc::ordered_malloc(n);
}

template<typename derived, typename IDType>
void ObjectHandle<derived,IDType>::operator delete(void *ptr,size_t n)
{
	alloc::ordered_free(ptr,n);
}

template<typename derived, typename IDType>
void *ObjectHandle<derived,IDType>::operator new[](size_t n)
{
	return alloc::ordered_malloc(n);

}

template<typename derived, typename IDType>
void ObjectHandle<derived,IDType>::operator delete[](void *ptr)
{
	alloc::ordered_free(ptr);
}
#endif





template<typename derived, typename IDType>
IDType ObjectHandle<derived,IDType>::getObjectCount()
{
	return objCounter;

}

template<typename derived, typename IDType>
IDType ObjectHandle<derived,IDType>::getMaxObjectID()
{
	return objSequence;

}

template<typename derived, typename IDType>
IDType ObjectHandle<derived,IDType>::getObjectID() const
{
	return m_ID;

}







#endif /* OBJECTHANDLE_H_ */
