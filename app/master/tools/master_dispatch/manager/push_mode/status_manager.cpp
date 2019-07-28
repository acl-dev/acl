#include "stdafx.h"
#include "push_mode/status_manager.h"

status_manager::status_manager()
{
}

status_manager::~status_manager()
{
	std::map<acl::string, server_status*>::iterator it =
		servers_status_.begin();
	for (; it != servers_status_.end(); ++it)
	{
		server_ttl* ttl = &it->second->get_ttl();
		delete ttl;
		delete it->second;
	}
}

void status_manager::set_status(const char* key, const char* data)
{
	server_ttl* ttl;
	lock_.lock();
	std::map<acl::string, server_status*>::iterator it =
		servers_status_.find(key);
	if (it != servers_status_.end())
	{
		ttl = &it->second->get_ttl();
		del_server_ttl(*ttl);
		delete ttl;
		delete it->second;
		servers_status_.erase(it);
	}
	ttl = new server_ttl(key);
	servers_ttl_.insert(ttl);
	servers_status_[key] = new server_status(*ttl, key, data);
	lock_.unlock();
}

void status_manager::del_status(const char* key)
{
	acl_assert(key && *key);
	std::map<acl::string, server_status*>::iterator it =
		servers_status_.find(key);
	if (it == servers_status_.end())
	{
		logger_warn("key: %s not found", key);
		return;
	}

	delete it->second;
	servers_status_.erase(it);
}

acl::string& status_manager::get_status(acl::string& out)
{
	lock_.lock();
	std::map<acl::string, server_status*>::const_iterator cit =
		servers_status_.begin();
	for (; cit != servers_status_.end(); ++cit)
		out << cit->second->get_data();
	lock_.unlock();

	return out;
}

int status_manager::check_timeout()
{
	lock_.lock();
	time_t now = time(NULL);

	std::multiset<server_ttl*, server_ttl_comp>::iterator it, tmp;

#if 0
	logger("------------------------------------");
	for (it = servers_ttl_.begin(); it != servers_ttl_.end();)
	{
		logger("key: %s, when: %ld, now: %ld, %ld",
			(*it)->get_key(), (*it)->get_when(),
			now, (*it)->get_when() - now);
		++it;
	}
	logger("------------------------------------\n");
#endif

	for (it = servers_ttl_.begin(); it != servers_ttl_.end();)
	{
		tmp = it;
		++it;
		if ((*tmp)->get_when() > now)
			break;

		logger("DELETE TIMEOUT SERVER: %s", (*tmp)->get_key());
		del_status((*tmp)->get_key());
		servers_ttl_.erase(tmp);
	}
	lock_.unlock();
	return 0;
}

void status_manager::del_server_ttl(server_ttl& ttl)
{
	typedef std::multiset<server_ttl*, server_ttl_comp>::iterator iterator;
	std::pair<iterator, iterator> pos = servers_ttl_.equal_range(&ttl);
	for (; pos.first != pos.second;)
	{
		iterator it = pos.first;
		++pos.first;
		if (strcasecmp((*it)->get_key(), ttl.get_key()) == 0)
		{
#if 0
			logger(">>>delete key: %s, t: %ld",
				(*it)->get_key(), (*it)->get_when());
#endif
			servers_ttl_.erase(it);
		}
	}
}
