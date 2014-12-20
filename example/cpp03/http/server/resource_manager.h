#pragma once
//#define BOOST_THREAD_USE_DLL

#include <boost/detail/lightweight_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
//#include <hash_map>
#include <unordered_map>


template<typename element> 
class resource_manager
{
public:

	typedef boost::shared_ptr<element> element_ptr;
	typedef std::tr1::unordered_map<long,element_ptr> resource_pool;
	typedef boost::shared_ptr<resource_pool> resource_pool_ptr;

	virtual ~resource_manager()
	{
		stop_all();
	}

	virtual void start(element_ptr c) = 0;

	virtual void stop(element_ptr c)
	{
		if(c)
		{
			remove(c);
			c->stop();
		}
	}

	void stop_all()
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		if(!resource_pool_.empty())
		{
			resource_pool backup_resource_pool(resource_pool_);
			std::pair<long,element_ptr> value;
			BOOST_FOREACH(value, backup_resource_pool)
			{
				value.second->stop();
			}
			backup_resource_pool.clear();
			resource_pool_.clear();

		}
	}

	void add(element_ptr c)
	{
		if(c)
			add_by_id(c->getObjectID(),c);
	}

	void remove(element_ptr c)
	{
		if(c)
			remove_by_id(c->getObjectID());
	}

	void add_by_id(long id, element_ptr c)
	{
		if(c)
		{
			boost::detail::lightweight_mutex::scoped_lock guard(lock);
			resource_pool_.insert(std::make_pair(id, c));
		}
	}

	void remove_by_id(long id)
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		if(!resource_pool_.empty())
			resource_pool_.erase(id);
	}

	bool exist(long id)
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		if(!resource_pool_.empty() && resource_pool_.count(id))
			return true;
		else
			return false;
	}


	element_ptr get(long id)
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		if(!resource_pool_.empty() && resource_pool_.count(id))
			return resource_pool_.find(id)->second;
		else
			return element_ptr();
	}

	resource_pool_ptr get_resource_pool()
	{
		boost::detail::lightweight_mutex::scoped_lock guard(lock);
		resource_pool_ptr backup_resource_pool = boost::make_shared<resource_pool>(boost::ref(resource_pool_));
		return backup_resource_pool;
	}

private:
	/// The managed connections.
	resource_pool resource_pool_;
	boost::detail::lightweight_mutex	lock;
};
