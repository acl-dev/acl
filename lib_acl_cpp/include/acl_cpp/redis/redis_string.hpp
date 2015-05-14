#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class string;
class redis_client;
class redis_result;

/**
 * ���е��ַ�������������ʵ��
 * all the commands in redis Strings are be implemented.
 */
class ACL_CPP_API redis_string : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_string();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_string(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*�� size_t)
	 */
	redis_string(redis_client_cluster* cluster, size_t max_conns);
	virtual ~redis_string();

	/////////////////////////////////////////////////////////////////////

	/**
	 * ���ַ���ֵ value ������ key
	 * set the string value of a key
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @param value {const char*} �ַ�������� value
	 *  the value of a string
	 * @return {bool} �����Ƿ�ɹ������� false ��ʾ������ key ������ַ�������
	 *  true if SET was executed correctly, false if error happened or
	 *  the key's object isn't a string.
	 */
	bool set(const char* key, const char* value);
	bool set(const char* key, size_t key_len,
		const char* value, size_t value_len);

	/**
	 * ��ֵ value ������ key ������ key ������ʱ����Ϊ timeout (����Ϊ��λ)��
	 * ��� key �Ѿ����ڣ� SETEX �����д��ֵ
	 * set key to hold the strnig value, and set key to timeout after
	 * a given number of seconds.
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @param value {const char*} �ַ�������� value
	 *  the value of a string
	 * @param timeout {int} ����ֵ����λΪ��
	 *  the timeout in seconds of a string
	 * @return {bool} �����Ƿ�ɹ������� false ��ʾ������ key ������ַ�������
	 *  true if SETEX was executed correctly, false if error happened
	 *  or the object specified by the key is not a string
	 */
	bool setex(const char* key, const char* value, int timeout);
	bool setex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * ��ֵ value ������ key ������ key ������ʱ����Ϊ timeout (�Ժ���Ϊ��λ)��
	 * ��� key �Ѿ����ڣ� SETEX �����д��ֵ
	 * set key to hold the string value, and set key to timeout after
	 * a given number of milliseconds.
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @param value {const char*} �ַ�������� value
	 *  the value of a string
	 * @param timeout {int} ����ֵ����λΪ����
	 *  the timeout in milliseconds of a string
	 * @return {bool} �����Ƿ�ɹ������� false ��ʾ������ key ������ַ�������
	 *  true if SETEX was executed correctly, false if error happened
	 *  or the object specified by the key is not a string
	 */
	bool psetex(const char* key, const char* value, int timeout);
	bool psetex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * �� key ��ֵ��Ϊ value �����ҽ��� key �����ڣ��������� key �Ѿ����ڣ�
	 * �� SETNX �����κζ���
	 * set the value of a key, only if the key does not exist.
	 * @param key {const char*} �ַ�������� key
	 *  the key of the string
	 * @param value {const char*} �ַ�������� value
	 *  the value of the string
	 * @return {int} ����ֵ�������£�
	 *  return the value as below:
	 *  -1������� key ���ַ�������
	 *      error happened or the object by the key isn't a string
	 *   0������ key �Ķ������
	 *      the string of the key already exists
	 *   1����ӳɹ�
	 *      the command was executed correctly
	 */
	int setnx(const char* key, const char* value);
	int setnx(const char* key, size_t key_len,
		const char* value, size_t value_len);

	/**
	 * ��� key �Ѿ����ڲ�����һ���ַ����� APPEND ��� value ׷�ӵ� key ԭ��
	 * ��ֵ��ĩβ����� key �����ڣ� APPEND �ͼ򵥵ؽ����� key ��Ϊ value
	 * append a value to a key
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @param value {const char*} �ַ��������ֵ
	 *  the value to be appended to a key
	 * @return {int} ���ص�ǰ���ַ����ĳ��ȣ�-1 ��ʾ����� key ���ַ�������
	 *  return the length of the string after appending, -1 if error
	 *  happened or the key's object isn't a string
	 */
	int append(const char* key, const char* value);
	int append(const char* key, const char* value, size_t size);

	/**
	 * ���� key ���������ַ���ֵ
	 * get the value of a key 
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @param buf {string&} �����ɹ���洢�ַ��������ֵ
	 *  store the value of a key after GET executed correctly
	 * @return {bool} �����Ƿ�ɹ������� false ��ʾ����� key ���ַ�������
	 *  if the GET was executed correctly, false if error happened or
	 *  is is not a string of the key
	 */
	bool get(const char* key, string& buf);
	bool get(const char* key, size_t len, string& buf);

	/**
	 * ���� key ���������ַ���ֵ�������ص��ַ���ֵ�Ƚϴ�ʱ���ڲ����Զ�������Ƭ������
	 * һ�����ڴ��г�һЩ��������С�ڴ棬ʹ������Ҫ���ݷ��صĽ���������¶Խ�����ݽ���
	 * ��װ��������Ե��ã� redis_result::get(size_t, size_t*) �������ĳ����
	 * Ƭ��Ƭ�����ݣ����� redis_result::get_size() ��÷�Ƭ����ĳ���
	 * @param key {const char*} �ַ�������� key
	 * @param buf {string&} �����ɹ���洢�ַ��������ֵ
	 * @return {bool} �����Ƿ�ɹ������� false ��ʾ����� key ���ַ�������
	 */
	const redis_result* get(const char* key);
	const redis_result* get(const char* key, size_t len);

	/**
	 * ������ key ��ֵ��Ϊ value �������� key �ľ�ֵ���� key ���ڵ�����
	 * �ַ�������ʱ������һ������
	 * set the string value of a key and and return its old value
	 * @param key {const char*} �ַ�������� key
	 *  the key of string
	 * @param value {const char*} �趨���µĶ����ֵ
	 *  the new string value of the key
	 * @param buf {string&} �洢����ľɵ�ֵ
	 *  store the old string value of the key
	 * @return {bool} �Ƿ�ɹ�
	 *  if GETSET was executed correctly.
	 */
	bool getset(const char* key, const char* value, string& buf);
	bool getset(const char* key, size_t key_len, const char* value,
		size_t value_len, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * ���ָ�� key ���ַ��������ֵ�����ݳ���
	 * get the length of value stored in a key
	 * @param key {const char*} �ַ�������� key
	 *  the key of the string
	 * @return {int} ����ֵ�������£�
	 *  return value as below:
	 *  -1���������ַ�������
	 *      error happened or the it isn't a string of the key
	 *   0���� key ������
	 *      the key doesn't exist
	 *  >0�����ַ������ݵĳ���
	 *      the length of the value stored in a key
	 */
	int get_strlen(const char* key);
	int get_strlen(const char* key, size_t key_len);

	/**
	 * �� value ������д(overwrite)���� key ��������ַ���ֵ����ƫ���� offset ��ʼ��
	 * �����ڵ� key �����հ��ַ�������
	 * overwrite part of a string at key starting at the specified offset
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @param offset {unsigned} ƫ������ʼλ�ã���ֵ���Դ����ַ��������ݳ��ȣ���ʱ
	 *  �Ǽ�Ŀն����� \0 ����
	 *  the specified offset of the string
	 * @param value {const char*} ���ǵ�ֵ
	 *  the value to be set
	 * @return {int} ��ǰ�ַ�����������ݳ���
	 *  the length of the string after SETRANGE
	 */
	int setrange(const char* key, unsigned offset, const char* value);
	int setrange(const char* key, size_t key_len, unsigned offset,
		const char* value, size_t value_len);

	/**
	 * ���� key ���ַ���ֵ�����ַ������ַ����Ľ�ȡ��Χ�� start �� end ����ƫ��������
	 * (���� start �� end ����)
	 * get substring of the string stored at a key
	 * @param key {const char*} �ַ�������� key
	 *  the key of string
	 * @param start {int} ��ʼ�±�ֵ
	 *  the starting offset of the string
	 * @param end {int} �����±�ֵ
	 *  the ending offset of the string
	 * @param buf {string&} �ɹ�ʱ�洢���
	 *  store the substring result
	 * @return {bool} �����Ƿ�ɹ�
	 *  if GETRANGE was executed correctly.
	 *  ע���±�λ�ÿ���Ϊ��ֵ����ʾ���ַ���β����ǰ��ʼ�������� -1 ��ʾ���һ��Ԫ��
	 */
	bool getrange(const char* key, int start, int end, string& buf);
	bool getrange(const char* key, size_t key_len,
		int start, int end, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * �� key ��������ַ���ֵ�����û����ָ��ƫ�����ϵ�λ(bit)��
	 * λ�����û����ȡ���� value ������������ 0 Ҳ������ 1
	 * set or clear the bit at offset in the string value stored at key
	 * @param key {const char*} �ַ�������� key
	 *  the key of the string
	 * @param offset {unsigned} ָ��ƫ������λ��
	 *  the offset at the string value
	 * @param bit {bool} Ϊ true ��ʾ���ñ�־λ������Ϊȡ����־λ
	 *  set bit if true, or clear bit if false at the specified offset
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the command was executed correctly
	 */
	bool setbit(const char* key, unsigned offset, bool bit);
	bool setbit(const char* key, size_t len, unsigned offset, bool bit);

	/**
	 * �� key ��������ַ���ֵ����ȡָ��ƫ�����ϵ�λ(bit)���� offset ���ַ���ֵ
	 * �ĳ��ȴ󣬻��� key ������ʱ������ 0
	 * get the bit at offset in the string value stored at key
	 * @param key {const char*} �ַ�������� key
	 *  the key of the string
	 * @param offset {unsigned} ָ��ƫ������λ��
	 *  the offset in the string value
	 * @param bit {int&} �ɹ���洢ָ��λ�õı�־λ
	 *  on success it will stored the bit at the specified offset
	 * @return {bool} �����Ƿ�ɹ������� false ��ʾ������ key ���ַ�������
	 *  if the GETBIT was executed correctly, false if error happened,
	 *  or the key doesn't store a string object
	 */
	bool getbit(const char* key, unsigned offset, int& bit);
	bool getbit(const char* key, size_t len, unsigned offset, int& bit);

	/**
	 * ��������ַ����У�������Ϊ 1 �ı���λ����������ָ���� start/end���������ָ��
	 * �����ڽ���
	 * count set bits in a string
	 * @param key {const char*} �ַ�������� key
	 *  the key of a string
	 * @return {int} ��־λΪ 1 ��������-1 ��ʾ�������ַ�������
	 *  the count of bits been set, -1 if error happened or it's not
	 *  a string
	 */
	int bitcount(const char* key);
	int bitcount(const char* key, size_t len);
	int bitcount(const char* key, int start, int end);
	int bitcount(const char* key, size_t len, int start, int end);

	/**
	 * ��һ������ key ���߼���������������浽 destkey
	 * BITOP AND on multiple source keys and save the result to another key
	 * @param destkey {const char*} Ŀ���ַ�������� key
	 *  the key storing the result
	 * @param keys Դ�ַ������󼯺�
	 *  the source keys
	 * @return {int} �洢��Ŀ�� key �е��ַ����ĳ���
	 *  the size of the string stored in the destination key, that is
	 *  equal to the size of the longest input string
	 */
	int bitop_and(const char* destkey, const std::vector<string>& keys);
	int bitop_and(const char* destkey, const std::vector<const char*>& keys);
	int bitop_and(const char* destkey, const char* key, ...);
	int bitop_and(const char* destkey, const char* keys[], size_t size);

	/**
	 * ��һ������ key ���߼��򣬲���������浽 destkey
	 * BITOP OR on multiple source keys and save the result to another key
	 * @param destkey {const char*} Ŀ���ַ�������� key
	 *  the destination key
	 * @param keys Դ�ַ������󼯺�
	 *  the source keys
	 * @return {int}
	 *  the size of the string stored in the destination key
	 */
	int bitop_or(const char* destkey, const std::vector<string>& keys);
	int bitop_or(const char* destkey, const std::vector<const char*>& keys);
	int bitop_or(const char* destkey, const char* key, ...);
	int bitop_or(const char* destkey, const char* keys[], size_t size);

	/**
	 * ��һ������ key ���߼���򣬲���������浽 destkey
	 * BITOP XOR on multiple source keys and save the result to another key
	 * @param destkey {const char*} Ŀ���ַ�������� key
	 *  the destination key
	 * @param keys Դ�ַ������󼯺�
	 *  the source keys
	 * @return {int}
	 *  the size of the string stored in the destination key
	 */
	int bitop_xor(const char* destkey, const std::vector<string>& keys);
	int bitop_xor(const char* destkey, const std::vector<const char*>& keys);
	int bitop_xor(const char* destkey, const char* key, ...);
	int bitop_xor(const char* destkey, const char* keys[], size_t size);

	/////////////////////////////////////////////////////////////////////

	/**
	 * ͬʱ����һ������ key-value ��
	 * set multiple key-value pair
	 * @param objs key-value �Լ���
	 *  the collection of multiple key-value pair
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the command was executed correctly
	 */
	bool mset(const std::map<string, string>& objs);
	bool mset(const std::vector<string>& keys,
		const std::vector<string>& values);
	bool mset(const char* keys[], const char* values[], size_t argc);
	bool mset(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * ���ҽ������и��� key ��������ʱͬʱ����һ������ key-value ��
	 * set multiple keys to multiple values only if none of the keys exist
	 * @param objs key-value �Լ���
	 *  the collection of multile key-value pair
	 * @return {int} ����ֵ�������£�
	 *  return value as below:
	 *  -1���������ַ�������
	 *      error happened or there were a object of not a string.
	 *   0����ӵ� key ������������һ���Ѿ�����
	 *     none be set because some of the keys already exist
	 *   1����ӳɹ�
	 *     add ok.
	 */
	int msetnx(const std::map<string, string>& objs);
	int msetnx(const std::vector<string>& keys,
		const std::vector<string>& values);
	int msetnx(const char* keys[], const char* values[], size_t argc);
	int msetnx(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * ��������(һ������)���� key ��ֵ����������� key ���棬��ĳ�� key �����ڣ�
	 * ��ô��� key ���ؿմ���ӽ����������
	 * get the values of the given keys
	 * @param keys {const std::vector<string>&} �ַ��� key ����
	 *  the given keys
	 * @param out {std::vector<acl::string>*} �ǿ�ʱ�洢�ַ���ֵ�������飬
	 *  ���ڲ����ڵ� key Ҳ��洢һ���մ�����
	 *  acl::string array storing the result. if one key not exists,
	 *  a empty string "" will also be stored in the array.
	 * @return {bool} �����Ƿ�ɹ��������ɹ������ͨ��������һ�ַ�ʽ������ݣ�
	 *  if successul, one of below ways can be used to get the result:
	 *
	 *  1���ڵ��÷����д���ǿյĴ洢�������ĵ�ַ
	 *     input the no-NULL result parameter when call hmget, when
	 *     success, the result will store the values of the given fileds
	 *
	 *  2�����෽�� get_value ���ָ���±��Ԫ������
	 *     call redis_command::result_value with the specified subscript
	 *
	 *  3�����෽�� get_child ���ָ���±��Ԫ�ض���(redis_result����Ȼ����ͨ��
	 *     redis_result::argv_to_string �������Ԫ������
	 *     redis_result::argv_to_string �������Ԫ������
	 *     call redis_command::result_child with specified subscript to
	 *     get redis_result object, then call redis_result::argv_to_string
	 *     with above result to get the values of the give fileds
	 *
	 *  4�����෽�� get_result ����ȡ���ܽ�������� redis_result��Ȼ����ͨ��
	 *     redis_result::get_child ���һ��Ԫ�ض���Ȼ����ͨ����ʽ 2 ��ָ��
	 *     �ķ�����ø�Ԫ�ص�����
	 *     call redis_command::get_result with the specified subscript to
	 *     get redis_result object, and use redis_result::get_child to
	 *     get one result object, then call redis_result::argv_to_string
	 *     to get the value of one filed.
	 *
	 *  5�����෽�� get_children ��ý��Ԫ�����������ͨ�� redis_result ��
	 *     �ķ��� argv_to_string ��ÿһ��Ԫ�ض����л��Ԫ������
	 *     use redis_command::get_children to get the redis_result array,
	 *     then use redis_result::argv_to_string to get every value of
	 *     the given fileds
	 */
	bool mget(const std::vector<string>& keys,
		std::vector<string>* out = NULL);
	bool mget(const std::vector<const char*>& keys,
		std::vector<string>* out = NULL);

	bool mget(std::vector<string>* result, const char* first_key, ...);
	bool mget(const char* keys[], size_t argc,
		std::vector<string>* out = NULL);
	bool mget(const char* keys[], const size_t keys_len[], size_t argc,
		std::vector<string>* out = NULL);

	/////////////////////////////////////////////////////////////////////

	/**
	 * �� key �д��������ֵ��һ
	 * 1����� key �����ڣ���ô key ��ֵ���ȱ���ʼ��Ϊ 0 ��Ȼ����ִ�� INCR ������
	 * 2�����ֵ������������ͣ����ַ������͵�ֵ���ܱ�ʾΪ���֣���ô����һ������
	 * 3����������ֵ������ 64 λ(bit)�з������ֱ�ʾ֮��
	 * increment the integer value of a key by one
	 * 1) if key not exists, the key's value will be set 0 and INCR
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} �ַ�������� key
	 *  the given key
	 * @param result {long long int*} �ǿ�ʱ�洢�������
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the INCR was executed correctly
	 */
	bool incr(const char* key, long long int* result = NULL);

	/**
	 * �� key �������ֵ�������� increment
	 * 1����� key �����ڣ���ô key ��ֵ���ȱ���ʼ��Ϊ 0 ��Ȼ����ִ�� INCRBY ����
	 * 2�����ֵ������������ͣ����ַ������͵�ֵ���ܱ�ʾΪ���֣���ô����һ������
	 * 3����������ֵ������ 64 λ(bit)�з������ֱ�ʾ֮��
	 * increment the integer value of a key by a given amount
	 * 1) if key not exists, the key's value will be set 0 and INCRBY
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} �ַ�������� key
	 *  the given key
	 * @param inc {long long int} ����ֵ
	 *  the given amount
	 * @param result {long long int*} �ǿ�ʱ�洢�������
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the INCRBY was executed correctly
	 */
	bool incrby(const char* key, long long int inc,
		long long int* result = NULL);

	/**
	 * Ϊ key ���������ֵ���ϸ���������
	 * 1) ��� key �����ڣ���ô INCRBYFLOAT ���Ƚ� key ��ֵ��Ϊ 0 ����ִ�мӷ�����
	 * 2) �������ִ�гɹ�����ô key ��ֵ�ᱻ����Ϊ��ִ�мӷ�֮��ģ���ֵ��������ֵ��
	 *    ���ַ�������ʽ���ظ�������
	 * 3) ������Ҳ���ֻ�ܱ�ʾС����ĺ�ʮ��λ
	 * increment the float value of a key by the given amount
	 * 1) if key not exists, the key's value will be set 0 and INCRBYFLOAT
	 * 2) if key's value is not a float an error will be returned
	 * @param key {const char*} �ַ�������� key
	 *  the given key
	 * @param inc {double} ����ֵ
	 *  the given amount
	 * @param result {double*} �ǿ�ʱ�洢�������
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the INCRBYFLOAT was executed correctly
	 */
	bool incrbyfloat(const char* key, double inc, double* result = NULL);

	/**
	 * �� key �д��������ֵ��һ
	 * 1) ��� key �����ڣ���ô key ��ֵ���ȱ���ʼ��Ϊ 0 ��Ȼ����ִ�� DECR ����
	 * 2) ���ֵ������������ͣ����ַ������͵�ֵ���ܱ�ʾΪ���֣���ô����һ������
	 * 3) ��������ֵ������ 64 λ(bit)�з������ֱ�ʾ֮��
	 * decrement the integer value of a key by one
	 * 1) if key not exists, the key's value will be set 0 and DECR
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} �ַ�������� key
	 *  the given key
	 * @param result {long long int*} �ǿ�ʱ�洢�������
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the DECR was executed correctly
	 */
	bool decr(const char* key, long long int* result = NULL);

	/**
	 * �� key �������ֵ��ȥ���� decrement
	 * 1) ��� key �����ڣ���ô key ��ֵ���ȱ���ʼ��Ϊ 0 ��Ȼ����ִ�� DECRBY ����
	 * 2) ���ֵ������������ͣ����ַ������͵�ֵ���ܱ�ʾΪ���֣���ô����һ������
	 * 3) ��������ֵ������ 64 λ(bit)�з������ֱ�ʾ֮��
	 * decrement the integer value of a key by the given amount
	 * @param key {const char*} �ַ�������� key
	 *  the given key
	 * @param dec {long long int} ����ֵ
	 *  the given amount
	 * @param result {long long int*} �ǿ�ʱ�洢�������
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} �����Ƿ�ɹ�
	 *  if the DECRBY was executed correctly
	 */
	bool decrby(const char* key, long long int dec,
		long long int* result = NULL);

private:
	int bitop(const char* op, const char* destkey,
		const std::vector<string>& keys);
	int bitop(const char* op, const char* destkey,
		const std::vector<const char*>& keys);
	int bitop(const char* op, const char* destkey,
		const char* keys[], size_t size);

	bool incoper(const char* cmd, const char* key, long long int inc,
		long long int* result);

};

} // namespace acl
