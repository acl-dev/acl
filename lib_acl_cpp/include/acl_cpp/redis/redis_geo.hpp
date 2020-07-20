#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define GEO_INVALID		360
#define GEO_LONGITUDE_MIN	-180
#define GEO_LONGITUDE_MAX	180
#define GEO_LATITUDE_MIN	-85.05112878
#define GEO_LATITUDE_MAX	85.05112878

enum
{
	GEO_UNIT_FT,
	GEO_UNIT_M,
	GEO_UNIT_MI,
	GEO_UNIT_KM,
};

enum
{
	GEO_WITH_COORD = 1 << 0,
	GEO_WITH_DIST  = 1 << 1,
	GEO_WITH_HASH  = 1 << 2,
};

enum
{
	GEO_SORT_NONE,
	GEO_SORT_ASC,
	GEO_SORT_DESC,
};

class ACL_CPP_API geo_member
{
public:
	geo_member(const char* name);
	geo_member(const geo_member& member);
	~geo_member(void);

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

class redis_client;

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
	virtual ~redis_geo();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 添加一个指定的地理位置坐标至指定的 key 中
	 * Add the specified geospatial item (latitude, logitude, name)
	 * to the specified key.
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param member {const char*} 该地理坐标的标识符
	 *  the geospatial's identifier
	 * @param longitude {double} 经度
	 *  the geospatial's loginitude
	 * @param latitude {double} 纬度
	 *  the geospatial's latitude
	 * @return {int} 1：添加成功，0：该地理坐标标识符已存在，即使对其值进行了修改，
	 *  也将返回 0，-1：表示出错。
	 *  the return value as below:
	 *  1: add one new member successfully
	 *  0: the member already existed, and the geospatial may be changed
	 * -1: some erro happened
	 */
	int geoadd(const char* key, const char* member,
		double longitude, double latitude);

	/**
	 * 给指定 key 添加一组地址位置坐标数据
	 * Add the specified geospatial items (latitude, logitude, name)
	 * to the specified key.
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param size {size_t} 数组的长度
	 *  the array's size
	 * @param members {const char* []} 成员数组，其长度由 size 指定
	 *  the members array, which's length was specified by size parameter
	 * @param longitudes {const double[]} 经度数据数组，其长度由 size 指定
	 *  the logintitudes array, which's length was specifed by size parameter
	 * @param latitudes {const double[]} 纬度数据数组，其长度由 size 指定
	 *  the lattitudes array, which's length was specifed by size parameter
	 * @return {int} 添加成功的成员数量，返回值含义如下：
	 *  return the successfully added members's count:
	 *  > 0: 表示成功添加的成员数量；
	 *       represent the successfully added members's count
	 *    0: 这些成员都已经存在
	 *       the members's belong the key already existing
	 *   -1: 表示出错，可以通过 result_error 函数查看出错原因
	 *       some error happened, the result_error function can be used
	 *       to find the error's reason
	 */
	int geoadd(const char* key, size_t size, const char* members[],
		const double longitudes[], const double latitudes[]);

	/**
	 * 给指定 key 添加一组地址位置坐标数据
	 * Add the specified geospatial items (latitude, logitude, name)
	 * to the specified key.
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param members {const std::vector<string>&} 成员数组
	 *  the members array
	 * @param longitudes {const std::vector<double>&} 经度数据数组
	 *  the logintitudes array
	 * @param latitudes {const std::vector<double>&} 纬度数据数组
	 *  the lattitudes array
	 * @return {int} 添加成功的成员数量，返回值含义如下：
	 *  return the successfully added members's count:
	 *  > 0: 表示成功添加的成员数量；
	 *       represent the successfully added members's count
	 *    0: 这些成员都已经存在
	 *       the members's belong the key already existing
	 *   -1: 表示出错，可以通过 result_error 函数查看出错原因
	 *       some error happened, the result_error function can be used
	 *       to find the error's reason
	 *  注意：三个数组(members, longitudes, latitudes)的数组长度必须相等
	 *  Notice: the three array's length must be equal between members,
	 *    longitudes and latitudes
	 */
	int geoadd(const char* key, const std::vector<string>& members,
		const std::vector<double>& longitudes,
		const std::vector<double>& latitudes);

	/**
	 * 以字符串方式返回指定成员的 GEOHASH 值
	 * Returns members of a geospatial index as standard geohash strings.
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param members {const std::vector<string>&} 成员数组
	 *  the members array
	 * @param results {std::vector<string>&} 存储结果集合
	 *  store the result
	 * @return {bool} 操作是否成功
	 *  if the operation was successful.
	 */
	bool geohash(const char* key, const std::vector<string>& members,
		std::vector<string>& results);

	/**
	 * 以字符串方式返回指定成员的 GEOHASH 值
	 * Returns members of a geospatial index as standard geohash strings.
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param member {const char*} 成员名
	 *  the member of a geospatial index
	 * @param result {std::vector<string>&} 存储结果
	 *  store the result
	 * @return {bool} 操作是否成功
	 *  if the operation was successful.
	 */
	bool geohash(const char* key, const char* member, string& result);

	/**
	 * 获得指定成员的地理位置坐标
	 * Returns longitude and latitude of members of a geospatial index
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param members {const std::vector<string>&} 成员数组
	 *  the members array
	 * @param results {std::vector<std::pair<double, double> >&} 存储结果集
	 *  store the results
	 * @return {bool} 操作是否成功
	 *  if the operation was successful. 
	 */
	bool geopos(const char* key, const std::vector<string>& members,
		std::vector<std::pair<double, double> >& results);

	/**
	 * 获得某个指定成员的地理位置坐标
	 * Returns longitude and latitude of the one member of
	 * a geospatial index
	 * @param key {const char*} 指定键值
	 *  the specifed key
	 * @param member {const char*} 指定成员名
	 *  the specified member
	 * @param result {std::pair<double, double>&} 存储坐标点结果
	 *  store the result of longitude and latitude of the member
	 * @return {bool} 操作是否成功
	 *  if the operation was successful.
	 */
	bool geopos(const char* key, const char* member,
		std::pair<double, double>& result);

	/**
	 * 获得两个地理位置坐标之间的距离
	 * Returns the distance between two members of a geospatial index
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param member1 {const char*} 地理坐标成员
	 *  one member of a geospatial index
	 * @param member2 {const char*} 地理坐标成员
	 *  another member of a geospatial index
	 * @param unit {int} 返回的距离的单位值
	 * @return {double} 两个坐标之间的长度，返回值 < 0 表示出错
	 *  returns the distance between two members, which was less than 0
	 *  if some error happened.
	 */
	double geodist(const char* key, const char* member1,
		const char* member2, int unit = GEO_UNIT_M);

	/**
	 * 获得距离某指定坐标位置在给定距离范围内的所有坐标点
	 * Query a sorted set representing a geospatial index to fetch
	 * members matching a given maximum distance from a point
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param longitude {double} 指定坐标点的经度值
	 *  the longitude of the specified geospatial coordinate
	 * @param latitude {double} 指定坐标点的纬度值
	 *  the latitude of the specified geospatial coordinate
	 * @param radius {double} 限定的距离范围大小
	 *  the distance from the specified coordinate
	 * @param unit {int} radius 距离的单位类型
	 *  the unit type of the raidus
	 * @param with {int} 查询条件选项，参见上面的定义：GEO_WITH_XXX
	 *  the serach operations, defined as GEO_WITH_XXX above
	 * @param sort {int} 查询结果的排序方式，定义参见：GEO_SORT_XXX
	 *  the sorted type of the results, defined as GEO_SORT_XXX above
	 * @return {const std::vector<geo_member>&} 符合条件的坐标点的结果集
	 *  Returns the results according the searching conditions.
	 */
	const std::vector<geo_member>& georadius(const char* key,
		double longitude, double latitude, double radius,
		int unit = GEO_UNIT_M,
		int with = GEO_WITH_COORD | GEO_WITH_DIST,
		int sort = GEO_SORT_ASC);

	/**
	 * 获得距离某指定坐标位置在给定距离范围内的所有坐标点
	 * Query a sorted set representing a geospatial index to fetch
	 * members matching a given maximum distance from a member
	 * @param key {const char*} 对应的键值
	 *  the specified key
	 * @param member {const char*} 某个指定的坐标点成员
	 *  the specified member of a geospatial index
	 * @param radius {double} 限定的距离范围大小
	 *  the distance from the specified coordinate
	 * @param unit {int} radius 距离的单位类型
	 *  the unit type of the raidus
	 * @param with {int} 查询条件选项，参见上面的定义：GEO_WITH_XXX
	 *  the serach operations, defined as GEO_WITH_XXX above
	 * @param sort {int} 查询结果的排序方式，定义参见：GEO_SORT_XXX
	 *  the sorted type of the results, defined as GEO_SORT_XXX above
	 * @return {const std::vector<geo_member>&} 符合条件的坐标点的结果集
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
