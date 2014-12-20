/************************************************************
	Copyright (C), 2009, . Co. Ltd.
	FileName:      Singleton2.h
	Author:        fenggang
	Version :      1.0
	Date:          2012.9.6

	Function List:
	History:
	<author>    <time>     <version >   <desc>
	fenggang  2012.9.6     1.0        create

***********************************************************/

#ifndef __SINGLETON2_T_H__
#define __SINGLETON2_T_H__

/*
#ifndef SF_SYSTEM_COMPILERCONFIG_H

#if defined(__WIN32__) || defined(WIN32)
#     ifdef SF_SYSTEM_SOURCE
#		define SF_SYSTEM_API __declspec(dllexport)
#	  else
#		define SF_SYSTEM_API __declspec(dllimport)
#     endif  // SF_SYSTEM_SOURCE
#else
# define SF_SYSTEM_API
#endif

#if !defined(SF_SYSTEM_SOURCE)
#	define BOOST_LIB_NAME SF_System
#	include "../auto_link.h"
#endif


#endif
*/

//#define BOOST_THREAD_NO_LIB

#include <map>
#include <vector>
#include <boost/detail/lightweight_mutex.hpp>
#include <boost/move/move.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>
#include <boost/foreach.hpp>
#include <boost/current_function.hpp>
#include <stdexcept>
#include "ObjectHandle.h"

//namespace System
//{

	class SingletonBase
		: private boost::noncopyable
	{
	public:

		typedef enum SingletonStatus
		{
			E_UnInited,
			E_Inited,
			E_Destroyed,
		} SingletonStatus;


		class SingletonPrivateBase
			: public ObjectHandle<SingletonPrivateBase>
		{
		public:
			SingletonPrivateBase();
			virtual ~SingletonPrivateBase();
		};

		typedef boost::shared_ptr<SingletonPrivateBase> SingletonPrivateBasePtr;
		typedef boost::weak_ptr<SingletonPrivateBase> SingletonPrivateBaseWeakPtr;
		
		static void				Add(long longevity, const SingletonPrivateBasePtr& holder);
		static void				Release();
		static void				SetStopAll();
		static volatile SingletonStatus	s_singletonStatus;	

	private:

		class SingletonHolder
		{
		public:

			typedef std::multimap<long,SingletonPrivateBasePtr > SingletonPool;

			SingletonHolder() ;
			~SingletonHolder();
			
			void AddSingletonHolder(long longevity, const SingletonPrivateBasePtr& holder);

			void ReleaseAllSingletonHolder();

			SingletonPool	m_singletonPool;
			boost::detail::lightweight_mutex	m_mutex;
		};

		static SingletonHolder& GetHolder();

	protected:

		static volatile bool	s_Stop_All;

	};




	/**********************************************************
	*	注意：												  *
	*	longevity值比较小的Singleton先析构					  *
	**********************************************************/

	template<typename T, long longevity = 100>
	class Singleton2 : public SingletonBase
	{
	public:

		class SingletonPrivate
			: public SingletonBase::SingletonPrivateBase
		{
		public:
			SingletonPrivate() {};
			virtual ~SingletonPrivate() {};
			boost::shared_ptr<T> m_spInstance;
		};

		static bool expired()
		{
			if( //s_Stop_All  || 
				(s_singletonStatus == E_Destroyed 
				&& s_wpPrivateInstance.expired() ))
				return true;
			else
				return false;
		}
		static T& instance()
		{
			boost::call_once(m_flag, init);
			bool bDeadRef = expired();
			boost::shared_ptr<SingletonPrivate> ptr;

			if(!bDeadRef)
			{
				ptr = s_wpPrivateInstance.lock();
				if(!ptr || !(ptr->m_spInstance))
					bDeadRef = true;
			}
			
			if(bDeadRef)
			{
				std::string errMsg("Dead Reference Singleton [");
				errMsg += BOOST_CURRENT_FUNCTION;
				errMsg +=']';
				throw std::runtime_error(errMsg);
			}
			else
				return *(ptr->m_spInstance);
		}

		template<typename StaticConfig>
		static bool configure(StaticConfig staticConfig)
		{

			return instance().Configure(boost::move(staticConfig));
		}


	private:

		static void init()
		{
			boost::shared_ptr<SingletonPrivate> ptr(new SingletonPrivate);
			ptr->m_spInstance.reset(new T());
			s_wpPrivateInstance = ptr;
			Add(longevity,ptr);
		}

	private:

		static boost::once_flag m_flag;
		static boost::weak_ptr<SingletonPrivate> s_wpPrivateInstance;

	};

	template<typename T,long longevity>
	boost::once_flag Singleton2<T, longevity>::m_flag = BOOST_ONCE_INIT;

	template<typename T,long longevity>
	boost::weak_ptr<typename Singleton2<T,longevity>::SingletonPrivate> 
		Singleton2<T,longevity>::s_wpPrivateInstance;
//}

#endif /*__SINGLETON2_T_H__*/

/* end of file */
















