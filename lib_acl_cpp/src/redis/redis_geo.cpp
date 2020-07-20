#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_geo.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define BUFLEN	32

/////////////////////////////////////////////////////////////////////////////

geo_member::geo_member(const char* name)
: name_(name)
{
	dist_ = -1;
	hash_ = -1;
	longitude_ = GEO_INVALID;
	latitude_ = GEO_INVALID;
}

geo_member::geo_member(const geo_member& member)
{
	name_ = member.name_.c_str();
	dist_ = member.dist_;
	hash_ = member.hash_;
	longitude_ = member.longitude_;
	latitude_ = member.latitude_;
}

geo_member::~geo_member()
{
}

void geo_member::set_dist(double dist)
{
	dist_ = dist;
}

void geo_member::set_hash(acl_int64 hash)
{
	hash_ = hash;
}

void geo_member::set_coordinate(double longitude, double latitude)
{
	if (longitude >= GEO_LONGITUDE_MIN
		&& longitude <= GEO_LONGITUDE_MAX
		&& latitude >= GEO_LATITUDE_MIN
		&& latitude <= GEO_LATITUDE_MAX)
	{
		longitude_ = longitude;
		latitude_ = latitude;
	}
}

/////////////////////////////////////////////////////////////////////////////

redis_geo::redis_geo()
{
}

redis_geo::redis_geo(redis_client* conn)
: redis_command(conn)
{
}

redis_geo::redis_geo(redis_client_cluster* cluster)
: redis_command(cluster)
{
}

redis_geo::redis_geo(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
{
}

redis_geo::~redis_geo()
{
}

int redis_geo::geoadd(const char* key, const char* member,
	double longitude, double latitude)
{
	size_t argc = 5;
	const char* argv[5];
	size_t lens[5];

	argv[0] = "GEOADD";
	lens[0] = sizeof("GEOADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	char* buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
	safe_snprintf(buf, BUFLEN, "%.8f", longitude);
	argv[2] = buf;
	lens[2] = strlen(buf);

	buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
	safe_snprintf(buf, BUFLEN, "%.8f", latitude);
	argv[3] = buf;
	lens[3] = strlen(buf);

	argv[4] = member;
	lens[4] = strlen(member);

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_geo::geoadd(const char* key, size_t size, const char* members[],
	const double longitudes[], const double latitudes[])
{
	size_t argc = 2 + 3 * size;
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t *lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "GEOADD";
	lens[0] = sizeof("GEOADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	for (size_t i = 0, n = 2; i < size; i++)
	{
		char* buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", longitudes[i]);
		argv[n] = buf;
		lens[n] = strlen(argv[n]);
		n++;

		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", latitudes[i]);
		argv[n] = buf;
		lens[n] = strlen(argv[n]);
		n++;

		argv[n] = members[i];
		lens[n] = strlen(argv[n]);
		n++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

int redis_geo::geoadd(const char* key, const std::vector<string>& members,
	const std::vector<double>& longitudes,
	const std::vector<double>& latitudes)
{
	if (members.empty())
	{
		logger_error("members empty");
		return -1;
	}
	if (members.size() != longitudes.size())
	{
		logger_error("longitudes's size(%d) != members's size(%d)",
			(int) longitudes.size(), (int) members.size());
		return -1;
	}
	if (latitudes.size() != longitudes.size())
	{
		logger_error("latitudes's size(%d) != longitudes's size(%d)",
			(int) latitudes.size(), (int) longitudes.size());
		return -1;
	}

	size_t argc = 2 + 3 * members.size();
	const char** argv = (const char**)
		dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t *lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "GEOADD";
	lens[0] = sizeof("GEOADD") - 1;

	argv[1] = key;
	lens[1] = strlen(key);

	size_t size = members.size();
	for (size_t i = 0, n = 2; i < size; i++)
	{
		char* buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", longitudes[i]);
		argv[n] = buf;
		lens[n] = strlen(argv[n]);
		n++;

		buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
		safe_snprintf(buf, BUFLEN, "%.8f", latitudes[i]);
		argv[n] = buf;
		lens[n] = strlen(argv[n]);
		n++;

		argv[n] = members[i].c_str();
		lens[n] = members[i].size();
		n++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	return get_number();
}

bool redis_geo::geohash(const char* key, const std::vector<string>& members,
	std::vector<string>& results)
{
	hash_slot(key);
	build("GEOHASH", key, members);
	if (get_strings(results) < 0)
		return false;
	return results.size() == members.size() ? true : false;
}

bool redis_geo::geohash(const char* key, const char* member, string& result)
{
	const char* names[1];
	names[0] = member;

	hash_slot(key);
	build("GEOHASH", key, names, 1);
	return get_string(result) < 0 ? false : true;
}

bool redis_geo::geopos(const char* key, const std::vector<string>& members,
	std::vector<std::pair<double, double> >& results)
{
	hash_slot(key);
	build("GEOPOS", key, members);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return false;

	string buf;

	for (size_t i = 0; i < size; i++)
	{
		const redis_result* child = children[i];
		if (child->get_type() != REDIS_RESULT_ARRAY)
		{
			results.push_back(std::make_pair(GEO_INVALID,
				GEO_INVALID));
			continue;
		}

		size_t n;
		const redis_result** xy = child->get_children(&n);
		if (xy == NULL || n != 2)
		{
			results.push_back(std::make_pair(GEO_INVALID,
				GEO_INVALID));
			continue;
		}
		const redis_result* rr_lo = xy[0], *rr_la = xy[1];
		if (rr_lo->get_type() != REDIS_RESULT_STRING
			|| rr_la->get_type() != REDIS_RESULT_STRING)
		{
			results.push_back(std::make_pair(GEO_INVALID,
				GEO_INVALID));
			continue;
		}

		double lo = rr_lo->get_double();
		if (lo < GEO_LONGITUDE_MIN || lo > GEO_LONGITUDE_MAX)
		{
			results.push_back(std::make_pair(GEO_INVALID,
				GEO_INVALID));
			continue;
		}

		double la = rr_la->get_double();
		if (la < GEO_LATITUDE_MIN || la > GEO_LATITUDE_MAX)
		{
			results.push_back(std::make_pair(GEO_INVALID,
				GEO_INVALID));
			continue;
		}

		results.push_back(std::make_pair(lo, la));
	}

	return true;
}

bool redis_geo::geopos(const char* key, const char* member,
	std::pair<double, double>& result)
{
	result.first = GEO_INVALID;
	result.second = GEO_INVALID;

	const char* names[1];
	names[0] = member;

	hash_slot(key);
	build("GEOHASH", key, names, 1);
	const redis_result* rr = run();
	if (rr == NULL || rr->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size;
	const redis_result** children = rr->get_children(&size);
	if (children == NULL || size != 1)
		return false;

	string buf;
	const redis_result* child = children[0];
	if (child->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t n;
	const redis_result** xy = child->get_children(&n);
	if (xy == NULL || n != 2)
		return false;

	const redis_result* rr_lo = xy[0], *rr_la = xy[1];
	double lo = rr_lo->get_double();
	if (lo < GEO_LONGITUDE_MIN || lo > GEO_LONGITUDE_MAX)
		return false;

	double la = rr_la->get_double();
	if (la < GEO_LATITUDE_MIN || la > GEO_LATITUDE_MAX)
		return false;

	result.first = lo;
	result.second = la;
	return true;
}

double redis_geo::geodist(const char* key, const char* member1,
	const char* member2, int unit /* = GEO_UNIT_M */)
{
	const char* names[3];
	names[0] = member1;
	names[1] = member2;

	size_t argc = 2;

	const char* unit_s = get_unit(unit);
	if (unit_s != NULL)
	{
		names[2] = unit_s;
		argc++;
	}

	hash_slot(key);
	build("GEODIST", key, names, argc);

	string buf;
	if (get_string(buf) == 0)
		return -1;
	return atof(buf.c_str());
}

const std::vector<geo_member>& redis_geo::georadius(const char* key,
	double longitude, double latitude, double radius,
	int unit /* = GEO_UNIT_M */,
	int with /* = GEO_WITH_COORD | GEO_WITH_DIST */,
	int sort /* = GEO_SORT_ASC */)
{
	positions_.clear();

	const char* argv[10];
	size_t lens[10];
	size_t argc = 0;

	argv[argc] = "GEORADIUS";
	lens[argc] = sizeof("GEORADIUS") - 1;
	argc++;

	argv[argc] = key;
	lens[argc] = strlen(key);
	argc++;

	char* buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
	safe_snprintf(buf, BUFLEN, "%.8f", longitude);
	argv[argc] = buf;
	lens[argc] = strlen(buf);
	argc++;

	buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
	safe_snprintf(buf, BUFLEN, "%.8f", latitude);
	argv[argc] = buf;
	lens[argc] = strlen(buf);
	argc++;

	buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
	safe_snprintf(buf, BUFLEN, "%.8f", radius);
	argv[argc] = buf;
	lens[argc] = strlen(buf);
	argc++;

	const char* unit_s = get_unit(unit);
	if (unit_s == NULL)
		unit_s = "m";
	argv[argc] = unit_s;
	lens[argc] = strlen(unit_s);
	argc++;

	if ((with & GEO_WITH_COORD) != 0)
	{
		argv[argc] = "WITHCOORD";
		lens[argc] = sizeof("WITHCOORD") - 1;
		argc++;
	}
	if ((with & GEO_WITH_DIST) != 0)
	{
		argv[argc] = "WITHDIST";
		lens[argc] = sizeof("WITHDIST") - 1;
		argc++;
	}
	if ((with & GEO_WITH_HASH) != 0)
	{
		argv[argc] = "WITHHASH";
		lens[argc] = sizeof("WITHHASH") - 1;
		argc++;
	}

	if (sort == GEO_SORT_ASC)
	{
		argv[argc] = "ASC";
		lens[argc] = sizeof("ASC") - 1;
		argc++;
	}
	else if (sort == GEO_SORT_DESC)
	{
		argv[argc] = "DESC";
		lens[argc] = sizeof("DESC") - 1;
		argc++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return positions_;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return positions_;

	for (size_t i = 0; i < size; i++)
		add_one_pos(*children[i]);

	return positions_;
}

const std::vector<geo_member>& redis_geo::georadiusbymember(const char* key,
	const char* member, double radius,
	int unit /* = GEO_UNIT_M */,
	int with /* = GEO_WITH_COORD | GEO_WITH_DIST */,
	int sort /* = GEO_SORT_ASC */)
{
	positions_.clear();

	const char* argv[9];
	size_t lens[9];
	size_t argc = 0;

	argv[argc] = "GEORADIUSBYMEMBER";
	lens[argc] = sizeof("GEORADIUSBYMEMBER") - 1;
	argc++;

	argv[argc] = key;
	lens[argc] = strlen(key);
	argc++;

	argv[argc] = member;
	lens[argc] = strlen(member);
	argc++;

	char* buf = (char*) dbuf_->dbuf_alloc(BUFLEN);
	safe_snprintf(buf, BUFLEN, "%.8f", radius);
	argv[argc] = buf;
	lens[argc] = strlen(buf);
	argc++;

	const char* unit_s = get_unit(unit);
	if (unit_s == NULL)
		unit_s = "m";
	argv[argc] = unit_s;
	lens[argc] = strlen(unit_s);
	argc++;

	if ((with & GEO_WITH_COORD) != 0)
	{
		argv[argc] = "WITHCOORD";
		lens[argc] = sizeof("WITHCOORD") - 1;
		argc++;
	}
	if ((with & GEO_WITH_DIST) != 0)
	{
		argv[argc] = "WITHDIST";
		lens[argc] = sizeof("WITHDIST") - 1;
		argc++;
	}
	if ((with & GEO_WITH_HASH) != 0)
	{
		argv[argc] = "WITHHASH";
		lens[argc] = sizeof("WITHHASH") - 1;
		argc++;
	}

	if (sort == GEO_SORT_ASC)
	{
		argv[argc] = "ASC";
		lens[argc] = sizeof("ASC") - 1;
		argc++;
	}
	else if (sort == GEO_SORT_DESC)
	{
		argv[argc] = "DESC";
		lens[argc] = sizeof("DESC") - 1;
		argc++;
	}

	hash_slot(key);
	build_request(argc, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return positions_;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL)
		return positions_;

	for (size_t i = 0; i < size; i++)
		add_one_pos(*children[i]);

	return positions_;
}

void redis_geo::add_one_pos(const redis_result& rr)
{
	string buf;
	acl::redis_result_t type = rr.get_type();
	if (type == REDIS_RESULT_STRING)
	{
		rr.argv_to_string(buf);
		positions_.push_back(geo_member(buf.c_str()));
		return;
	}
	if (type != REDIS_RESULT_ARRAY)
		return;

	size_t size;
	const redis_result** children = rr.get_children(&size);
	if (children == NULL || size == 0)
		return;

	if (children[0]->get_type() != REDIS_RESULT_STRING)
		return;
	children[0]->argv_to_string(buf);
	geo_member pos(buf.c_str());

	for (size_t i = 1; i < size; i++)
	{
		type = children[i]->get_type();
		if (type == REDIS_RESULT_STRING)
			pos.set_dist(children[i]->get_double());
		else if (type == REDIS_RESULT_INTEGER)
		{
			bool ok;
			acl_int64 hash = children[i]->get_integer64(&ok);
			if (ok)
				pos.set_hash(hash);
		}
		else if (type != REDIS_RESULT_ARRAY)
			continue;

		size_t n;
		const redis_result** results = children[i]->get_children(&n);
		if (results != NULL && n == 2
			&& results[0]->get_type() == REDIS_RESULT_STRING
			&& results[1]->get_type() == REDIS_RESULT_STRING)
		{
			pos.set_coordinate(results[0]->get_double(),
				results[1]->get_double());
		}
	}

	positions_.push_back(pos);
}

/////////////////////////////////////////////////////////////////////////////

typedef struct
{
	int unit;
	const char* str;
} UNIT_MAP;

const char* redis_geo::get_unit(int unit)
{
	static const UNIT_MAP _map[] = {
		{ GEO_UNIT_FT, "ft" },
		{ GEO_UNIT_M, "m" },
		{ GEO_UNIT_MI, "mi" },
		{ GEO_UNIT_KM, "km" },
	};

	if (unit < GEO_UNIT_FT || unit > GEO_UNIT_KM)
		return NULL;
	return _map[unit].str;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
