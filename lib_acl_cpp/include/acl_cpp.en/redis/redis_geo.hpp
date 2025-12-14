#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

#define GEO_INVALID		360
#define GEO_LONGITUDE_MIN	-180
#define GEO_LONGITUDE_MAX	180
#define GEO_LATITUDE_MIN	-85.05112878
#define GEO_LATITUDE_MAX	85.05112878

enum {
	GEO_UNIT_FT,
	GEO_UNIT_M,
	GEO_UNIT_MI,
	GEO_UNIT_KM,
};

enum {
	GEO_WITH_COORD = 1 << 0,
	GEO_WITH_DIST  = 1 << 1,
	GEO_WITH_HASH  = 1 << 2,
};

enum {
	GEO_SORT_NONE,
	GEO_SORT_ASC,
	GEO_SORT_DESC,
};

class ACL_CPP_API geo_member {
public:
	geo_member(const char* name);
	geo_member(const geo_member& member);
	~geo_member();

	void set_name(const char* name);
	const char* get_name() const
	{
		return name_.c_str();
	}

	void set_dist(double dist);
	double get_dist() const
	{
		return dist_;
	}

#if defined(_WIN32) || defined(_WIN64)
	void set_hash(__int64 hash);
	__int64 get_hash() const
#else
	void set_hash(long long int hash);
	long long int get_hash() const
#endif // defined(_WIN32) || defined(_WIN64)
	{
		return hash_;
	}

	void set_coordinate(double longitude, double latitude);
	double get_longitude() const
	{
		return longitude_;
	}

	double get_latitude() const
	{
		return latitude_;
	}

private:
	string name_;
	double dist_;
#if defined(_WIN32) || defined(_WIN64)
	__int64 hash_;
#else
	long long int hash_;
#endif // defined(_WIN32) || defined(_WIN64)

	double longitude_;
	double latitude_;
};

class ACL_CPP_API redis_geo : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_geo();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_geo(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_geo(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_geo(redis_client_cluster* cluster, size_t max_conns);

	redis_geo(redis_client_pipeline* pipeline);

	virtual ~redis_geo();

	/////////////////////////////////////////////////////////////////////

	/**
	 * Add a specified geospatial coordinate to specified key
	 * Add the specified geospatial item (latitude, logitude, name)
	 * to the specified key.
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param member {const char*} Identifier of this geospatial coordinate
	 *  the geospatial's identifier
	 * @param longitude {double} Longitude
	 *  the geospatial's loginitude
	 * @param latitude {double} Latitude
	 *  the geospatial's latitude
	 * @return {int} 1: Add successful, 0: This geospatial coordinate identifier
	 * already exists. Even if its value was modified,
	 *  will also return 0. -1: Indicates error.
	 *  the return value as below:
	 *  1: add one new member successfully
	 *  0: the member already existed, and the geospatial may be changed
	 * -1: some erro happened
	 */
	int geoadd(const char* key, const char* member,
		double longitude, double latitude);

	/**
	 * Add a group of geospatial coordinate data to specified key
	 * Add the specified geospatial items (latitude, logitude, name)
	 * to the specified key.
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param size {size_t} Array length
	 *  the array's size
	 * @param members {const char* []} Member array, its length is specified by
	 * size
	 *  the members array, which's length was specified by size parameter
	 * @param longitudes {const double[]} Longitude data array, its length is
	 * specified by size
	 *  the logintitudes array, which's length was specifed by size parameter
	 * @param latitudes {const double[]} Latitude data array, its length is
	 * specified by size
	 *  the lattitudes array, which's length was specifed by size parameter
	 * @return {int} Number of successfully added members. Return value meanings:
	 *  return the successfully added members's count:
	 *  > 0: Indicates number of successfully added members;
	 *       represent the successfully added members's count
	 *    0: These members already exist
	 *       the members's belong the key already existing
	 *   -1: Indicates error. Can check error reason through result_error function
	 *       some error happened, the result_error function can be used
	 *       to find the error's reason
	 */
	int geoadd(const char* key, size_t size, const char* members[],
		const double longitudes[], const double latitudes[]);

	/**
	 * Add a group of geospatial coordinate data to specified key
	 * Add the specified geospatial items (latitude, logitude, name)
	 * to the specified key.
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param members {const std::vector<string>&} Member array
	 *  the members array
	 * @param longitudes {const std::vector<double>&} Longitude data array
	 *  the logintitudes array
	 * @param latitudes {const std::vector<double>&} Latitude data array
	 *  the lattitudes array
	 * @return {int} Number of successfully added members. Return value meanings:
	 *  return the successfully added members's count:
	 *  > 0: Indicates number of successfully added members;
	 *       represent the successfully added members's count
	 *    0: These members already exist
	 *       the members's belong the key already existing
	 *   -1: Indicates error. Can check error reason through result_error function
	 *       some error happened, the result_error function can be used
	 *       to find the error's reason
	 * Note: Three arrays (members, longitudes, latitudes) must have equal array
	 * lengths
	 *  Notice: the three array's length must be equal between members,
	 *    longitudes and latitudes
	 */
	int geoadd(const char* key, const std::vector<string>& members,
		const std::vector<double>& longitudes,
		const std::vector<double>& latitudes);

	/**
	 * Return GEOHASH value of specified members as strings
	 * Returns members of a geospatial index as standard geohash strings.
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param members {const std::vector<string>&} Member array
	 *  the members array
	 * @param results {std::vector<string>&} Store result set
	 *  store the result
	 * @return {bool} Whether operation was successful
	 *  if the operation was successful.
	 */
	bool geohash(const char* key, const std::vector<string>& members,
		std::vector<string>& results);

	/**
	 * Return GEOHASH value of specified member as string
	 * Returns members of a geospatial index as standard geohash strings.
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param member {const char*} Member name
	 *  the member of a geospatial index
	 * @param result {std::vector<string>&} Store result
	 *  store the result
	 * @return {bool} Whether operation was successful
	 *  if the operation was successful.
	 */
	bool geohash(const char* key, const char* member, string& result);

	/**
	 * Get geospatial coordinates of specified members
	 * Returns longitude and latitude of members of a geospatial index
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param members {const std::vector<string>&} Member array
	 *  the members array
	 * @param results {std::vector<std::pair<double, double> >&} Store result set
	 *  store the results
	 * @return {bool} Whether operation was successful
	 *  if the operation was successful. 
	 */
	bool geopos(const char* key, const std::vector<string>& members,
		std::vector<std::pair<double, double> >& results);

	/**
	 * Get geospatial coordinates of a specified member
	 * Returns longitude and latitude of the one member of
	 * a geospatial index
	 * @param key {const char*} Specified key value
	 *  the specifed key
	 * @param member {const char*} Specified member name
	 *  the specified member
	 * @param result {std::pair<double, double>&} Store coordinate point result
	 *  store the result of longitude and latitude of the member
	 * @return {bool} Whether operation was successful
	 *  if the operation was successful.
	 */
	bool geopos(const char* key, const char* member,
		std::pair<double, double>& result);

	/**
	 * Get distance between two geospatial coordinates
	 * Returns the distance between two members of a geospatial index
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param member1 {const char*} Geospatial coordinate member
	 *  one member of a geospatial index
	 * @param member2 {const char*} Geospatial coordinate member
	 *  another member of a geospatial index
	 * @param unit {int} Unit value of returned distance
	 * @return {double} Length between two coordinates. Return value < 0 indicates
	 * error
	 *  returns the distance between two members, which was less than 0
	 *  if some error happened.
	 */
	double geodist(const char* key, const char* member1,
		const char* member2, int unit = GEO_UNIT_M);

	/**
	 * Get all coordinate points within given distance range from a specified
	 * coordinate position
	 * Query a sorted set representing a geospatial index to fetch
	 * members matching a given maximum distance from a point
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param longitude {double} Longitude value of specified coordinate point
	 *  the longitude of the specified geospatial coordinate
	 * @param latitude {double} Latitude value of specified coordinate point
	 *  the latitude of the specified geospatial coordinate
	 * @param radius {double} Limited distance range size
	 *  the distance from the specified coordinate
	 * @param unit {int} Unit type of radius distance
	 *  the unit type of the raidus
	 * @param with {int} Query condition options. See definitions above:
	 * GEO_WITH_XXX
	 *  the serach operations, defined as GEO_WITH_XXX above
	 * @param sort {int} Sorting method of query results. See definitions:
	 * GEO_SORT_XXX
	 *  the sorted type of the results, defined as GEO_SORT_XXX above
	 * @return {const std::vector<geo_member>&} Result set of coordinate points
	 * meeting conditions
	 *  Returns the results according the searching conditions.
	 */
	const std::vector<geo_member>& georadius(const char* key,
		double longitude, double latitude, double radius,
		int unit = GEO_UNIT_M,
		int with = GEO_WITH_COORD | GEO_WITH_DIST,
		int sort = GEO_SORT_ASC);

	/**
	 * Get all coordinate points within given distance range from a specified
	 * coordinate position
	 * Query a sorted set representing a geospatial index to fetch
	 * members matching a given maximum distance from a member
	 * @param key {const char*} Corresponding key value
	 *  the specified key
	 * @param member {const char*} A specified coordinate point member
	 *  the specified member of a geospatial index
	 * @param radius {double} Limited distance range size
	 *  the distance from the specified coordinate
	 * @param unit {int} Unit type of radius distance
	 *  the unit type of the raidus
	 * @param with {int} Query condition options. See definitions above:
	 * GEO_WITH_XXX
	 *  the serach operations, defined as GEO_WITH_XXX above
	 * @param sort {int} Sorting method of query results. See definitions:
	 * GEO_SORT_XXX
	 *  the sorted type of the results, defined as GEO_SORT_XXX above
	 * @return {const std::vector<geo_member>&} Result set of coordinate points
	 * meeting conditions
	 *  Returns the results according the searching conditions.
	 */
	const std::vector<geo_member>& georadiusbymember(const char* key,
		const char* member, double radius,
		int unit = GEO_UNIT_M,
		int with = GEO_WITH_COORD | GEO_WITH_DIST,
		int sort = GEO_SORT_ASC);

private:
	std::vector<geo_member> positions_;

	void add_one_pos(const redis_result& rr);
	static const char* get_unit(int unit);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

