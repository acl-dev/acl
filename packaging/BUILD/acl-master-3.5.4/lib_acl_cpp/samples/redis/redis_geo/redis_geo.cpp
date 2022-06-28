#include "stdafx.h"

#define MAX_MEMBER	1000

static acl::string __keypre("geo_key");

static bool test_geoadd(acl::redis& redis, int n)
{
	acl::string key;
	acl::string member;
	std::vector<acl::string> members;
	std::vector<double> longitudes;
	std::vector<double> latitudes;

	members.reserve(MAX_MEMBER);

	for (int i = 0; i < MAX_MEMBER; i++)
	{
		member.format("member_%d", i);
		members.push_back(member);
		longitudes.push_back(48 + 0.01 * i);
		latitudes.push_back(45 + 0.01 * i);
	}

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);

		redis.clear();
		int ret = redis.geoadd(key, members, longitudes, latitudes);
		if (ret < 0)
		{
			printf("geoadd key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("geoadd ok, key: %s, ret: %d\r\n", key.c_str(), ret);
	}

	return true;
}

static bool test_geohash(acl::redis& redis, int n)
{
	acl::string key;
	acl::string member;
	std::vector<acl::string> members;
	std::vector<acl::string> results;

	members.reserve(MAX_MEMBER);

	for (int i = 0; i < MAX_MEMBER; i++)
	{
		member.format("member_%d", i);
		members.push_back(member);
	}

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		redis.clear();
		bool ret = redis.geohash(key.c_str(), members, results);
		if (!ret)
		{
			printf("geohash key: %s error: %s\r\n",
				key.c_str(), redis.result_error());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("geohash ok, key: %s, count: %d\r\n", key.c_str(), ret);

		for (size_t j = 0; j  < members.size(); j ++)
		{
			printf(">> %s: %s\r\n", members[j].c_str(),
				results[j].c_str());
		}
	}

	return true;
}

static bool test_geopos(acl::redis& redis, int n)
{
	acl::string key;
	acl::string member;
	std::vector<acl::string> members;
	std::vector<std::pair<double, double> > results;

	members.reserve(MAX_MEMBER);

	for (int i = 0; i < MAX_MEMBER; i++)
	{
		member.format("member_%d", i);
		members.push_back(member);
	}

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		results.clear();

		key.format("%s_%d", __keypre.c_str(), i);
		bool ret = redis.geopos(key.c_str(), members, results);
		if (!ret)
		{
			printf("geopos error: %s\r\n", redis.result_error());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("key: %s\r\n", key.c_str());
		for (size_t j = 0; j < members.size(); j++)
		{
			printf(">>%s: longitude = %.8f, latitude = %.8f\r\n",
				members[j].c_str(), results[j].first,
				results[j].second);
		}
	}

	return true;
}

static bool test_geodist(acl::redis& redis, int n)
{
	acl::string key;
	acl::string member1("member_0"), member2("member_1");

	for (int i = 0; i < n; i++)
	{
		redis.clear();
		key.format("%s_%d", __keypre.c_str(), i);
		double ret = redis.geodist(key.c_str(), member1.c_str(),
			member2.c_str());
		if (ret < 0)
		{
			printf("geodist error: %s, key: %s\r\n",
				redis.result_error(), key.c_str());
			return false;
		}
		else if (i >= 10)
			continue;

		printf("ok, key: %s, dist: %.8f between %s and %s\r\n",
			key.c_str(), ret, member1.c_str(), member2.c_str());
	}

	return true;
}

static bool test_georadius(acl::redis& redis, int n)
{
	acl::string key;
	double longitude = 48, latitude = 45, radius = 10000;

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		const std::vector<acl::geo_member>& members = redis.georadius(
			key.c_str(), longitude, latitude, radius);
		if (members.empty())
			continue;
		if (i >= 10)
			continue;

		std::vector<acl::geo_member>::const_iterator cit;
		for (cit = members.begin(); cit != members.end(); ++cit)
		{
			printf(">>%s: longitude: %.8f, latitude: %.8f, dist: %.8f\r\n",
				(*cit).get_name(), (*cit).get_longitude(),
				(*cit).get_latitude(), (*cit).get_dist());
		}
	}

	return true;
}

static bool test_georadiusbymember(acl::redis& redis, int n)
{
	acl::string key;
	double radius = 10;
	const char* member = "member_1";

	for (int i = 0; i < n; i++)
	{
		key.format("%s_%d", __keypre.c_str(), i);
		const std::vector<acl::geo_member>& members =
			redis.georadiusbymember(key.c_str(), member,
				radius, acl::GEO_UNIT_KM);
		if (members.empty())
			continue;
		if (i >= 10)
			continue;

		std::vector<acl::geo_member>::const_iterator cit;
		for (cit = members.begin(); cit != members.end(); ++cit)
		{
			printf(">>%s: longitude: %.8f, latitude: %.8f, dist: %.8f\r\n",
				(*cit).get_name(), (*cit).get_longitude(),
				(*cit).get_latitude(), (*cit).get_dist());
		}
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"-s redis_addr[127.0.0.1:6379]\r\n"
		"-n count\r\n"
		"-C connect_timeout[default: 10]\r\n"
		"-T rw_timeout[default: 10]\r\n"
		"-a cmd[geoadd|geohash|geopos|geodist|georadius|georadiusbymember\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch, n = 1, conn_timeout = 10, rw_timeout = 10;
	acl::string addr("127.0.0.1:6379"), cmd;

	while ((ch = getopt(argc, argv, "hs:n:C:T:a:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'C':
			conn_timeout = atoi(optarg);
			break;
		case 'T':
			rw_timeout = atoi(optarg);
			break;
		case 'a':
			cmd = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	acl::redis_client_cluster cluster;
	cluster.set(addr.c_str(), 100, conn_timeout, rw_timeout);

	acl::redis redis;
	redis.set_cluster(&cluster);

	bool ret;

	if (cmd == "geoadd")
		ret = test_geoadd(redis, n);
	else if (cmd == "geohash")
		ret = test_geohash(redis, n);
	else if (cmd == "geopos")
		ret = test_geopos(redis, n);
	else if (cmd == "geodist")
		ret = test_geodist(redis, n);
	else if (cmd == "georadius")
		ret = test_georadius(redis, n);
	else if (cmd == "georadiusbymember")
		ret = test_georadiusbymember(redis, n);
	else if (cmd == "all")
	{
		ret = test_geoadd(redis, n)
			&& test_geohash(redis, n)
			&& test_geopos(redis, n)
			&& test_geodist(redis, n)
			&& test_georadius(redis, n)
			&& test_georadiusbymember(redis, n);
	}
	else
	{
		ret = false;
		printf("unknown cmd: %s\r\n", cmd.c_str());
	}

	if (ret == true)
		printf("test OK!\r\n");
	else
		printf("test failed!\r\n");

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}
