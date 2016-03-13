#include "stdafx.h"
#include "redis_util.h"
#include "redis_migrate.h"
#include "redis_reshard.h"

redis_reshard::redis_reshard(const char* addr, const char* passwd)
	: addr_(addr)
{
	if (passwd && *passwd)
		passwd_ = passwd;
}

redis_reshard::~redis_reshard()
{
	std::vector<acl::redis_node*>::iterator it = masters_.begin();
	for (; it != masters_.end(); ++it)
		delete *it;
}

void redis_reshard::copy_slots(acl::redis_node& from, acl::redis_node& to)
{
	const std::vector<std::pair<size_t, size_t> >& slots = from.get_slots();
	std::vector<std::pair<size_t, size_t> >::const_iterator cit;
	for (cit = slots.begin(); cit != slots.end(); ++cit)
		to.add_slot_range(cit->first, cit->second);
}

acl::redis_node* redis_reshard::find_node(const char* id)
{
	std::vector<acl::redis_node*>::iterator it;
	for (it = masters_.begin(); it != masters_.end(); ++it)
	{
		if (strcmp(id, (*it)->get_id()) == 0)
			return *it;
	}
	return NULL;
}

void redis_reshard::copy_all(std::vector<acl::redis_node*>& src,
	const char* exclude)
{
	std::vector<acl::redis_node*>::iterator it;
	for (it = masters_.begin(); it != masters_.end(); ++it)
	{
		if (strcmp((*it)->get_id(), exclude) != 0)
			src.push_back(*it);
	}
}

void redis_reshard::run()
{
	if (get_masters_info() == false)
		return;

	show_nodes();
	fflush(stdout);
	char buf[1024];

	int nslots = 0;
	while (true)
	{
		printf("How many slots do you want to move (from 1 to 16384) ? ");
		fflush(stdout);
		int ret = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			exit(1);
		acl_mystr_trim(buf);
		nslots = atoi(buf);
		if (nslots > 0 && nslots < 16384)
			break;
		printf("invalid value: %d\r\n", ret);
	}

	acl::redis_node* target = NULL;
	while (true)
	{
		printf("What is the receiving node ID? ");
		fflush(stdout);
		int ret = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			exit(1);

		acl_mystr_trim(buf);
		target = find_node(buf);
		if (target != NULL)
			break;

		printf("...The specified node(%s) is not known or not "
			"a master, please try again.\r\n", buf);
	}
	assert(target != NULL);

	printf("Please input all the source node IDs.\r\n");
	printf("  Type 'all' to use all the nodes as source nodes for the hash slots\r\n");
	printf("  Type 'done' once you entered all the source node IDs.\r\n");

	std::vector<acl::redis_node*> sources;
	while (true)
	{
		printf("Source node #%d: ", (int) sources.size() + 1);
		fflush(stdout);
		int ret = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			exit(1);

		acl_mystr_trim(buf);
		if (strcasecmp(buf, "done") == 0)
			break;
		if (strcasecmp(buf, "all") == 0)
		{
			copy_all(sources, target->get_id());
			break;
		}

		acl::redis_node* source = find_node(buf);
		if (source == NULL)
		{
			printf("...The source node(%s) is not known\r\n", buf);
			continue;
		}
		if (strcmp(target->get_id(), buf) == 0)
		{
			printf("... It is not possible to use the target node as source node\r\n");
			continue;
		}
		
		sources.push_back(source);
	}
	if (sources.empty())
	{
		printf("*** No source nodes given, operation aborted\r\n");
		exit(1);
	}

	redis_migrate migrate(masters_, passwd_);
	migrate.move_slots(sources, *target, nslots);
}

bool redis_reshard::get_masters_info()
{
	acl::redis_client client(addr_, 30, 30);
	acl::redis redis(&client);

	const std::map<acl::string, acl::redis_node*>* masters;
	if ((masters = redis.cluster_nodes()) == NULL)
	{
		printf("%s: master nodes empty\r\n", __FUNCTION__);
		return false;
	}

	std::map<acl::string, acl::redis_node*>::const_iterator cit;
	for (cit = masters->begin(); cit != masters->end(); ++cit)
	{
		acl::redis_node* master = new acl::redis_node;
		master->set_id(cit->second->get_id());
		master->set_addr(cit->second->get_addr());
		copy_slots(*cit->second, *master);
		masters_.push_back(master);
	}

	return true;
}

void redis_reshard::show_nodes()
{
	std::vector<acl::redis_node*>::const_iterator cit;
	for (cit = masters_.begin(); cit != masters_.end(); ++cit)
	{
		printf("-----------------------------------------------\r\n");
		printf("addr: %s\r\nid: %s\r\n", (*cit)->get_addr(),
			(*cit)->get_id());
		show_slots(**cit);
	}
}

void redis_reshard::show_slots(const acl::redis_node& node)
{
	const std::vector<std::pair<size_t, size_t> > slots = node.get_slots();
	std::vector<std::pair<size_t, size_t> >::const_iterator cit;
	for (cit = slots.begin(); cit != slots.end(); ++cit)
		printf("slots: %d - %d\r\n",
			(int) cit->first, (int) cit->second);
}
