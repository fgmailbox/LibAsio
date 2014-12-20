/************************************************************
	Copyright (C), 2009, Co. Ltd.
	FileName:      Singleton2.cpp
	Author:        fenggang
	Version :      1.0
	Date:          2012.9.6

	Function List:
	History:
	<author>    <time>     <version >   <desc>
	fenggang  2012.9.6     1.0        create

***********************************************************/


#include "Singleton2.h"

//namespace System
//{
	volatile SingletonBase::SingletonStatus	SingletonBase::s_singletonStatus = SingletonBase::E_UnInited;

	volatile bool	SingletonBase::s_Stop_All = false;

	void SingletonBase::SetStopAll()
	{
		s_Stop_All = true;
	};


	void SingletonBase::Release()
	{
		SetStopAll();
		s_singletonStatus = E_Destroyed;
		GetHolder().ReleaseAllSingletonHolder();
	}

	void SingletonBase::Add(long longevity, const SingletonBase::SingletonPrivateBasePtr& holder)
	{
		GetHolder().AddSingletonHolder(longevity,holder);
		s_singletonStatus = E_Inited;

	}

	SingletonBase::SingletonHolder& SingletonBase::GetHolder()
	{
		static SingletonHolder s_holder;
		return s_holder;
	}



	SingletonBase::SingletonPrivateBase::SingletonPrivateBase() 
	{

	};
	SingletonBase::SingletonPrivateBase::~SingletonPrivateBase() 
	{

	};



	SingletonBase::SingletonHolder::SingletonHolder() 
	{

	};

	SingletonBase::SingletonHolder::~SingletonHolder() 
	{ 
		s_singletonStatus = E_Destroyed;
		ReleaseAllSingletonHolder();
	};

	void SingletonBase::SingletonHolder::AddSingletonHolder(long longevity, const SingletonBase::SingletonPrivateBasePtr& holder)
	{
		boost::detail::lightweight_mutex::scoped_lock lock(m_mutex);
		m_singletonPool.insert(std::make_pair(longevity,holder));
	}

	void SingletonBase::SingletonHolder::ReleaseAllSingletonHolder()
	{
		boost::detail::lightweight_mutex::scoped_lock lock(m_mutex);
		for(SingletonPool::iterator it = m_singletonPool.begin();it != m_singletonPool.end();it = m_singletonPool.begin())
		{
			m_singletonPool.erase(it->first);

		}
	}
//}

/* end of file */
















