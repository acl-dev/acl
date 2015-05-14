#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_set : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_set();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_set(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*�� size_t)
	 */
	redis_set(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_set();

	/////////////////////////////////////////////////////////////////////

	/**
	 * ��һ������ member Ԫ�ؼ��뵽���� key ���У��Ѿ������ڼ��ϵ� member Ԫ��
	 * ��������;
	 * 1) ���� key �����ڣ��򴴽�һ��ֻ���� member Ԫ������Ա�ļ���
	 * 2) �� key ���Ǽ�������ʱ������һ������
	 * add one or more members to a set stored at a key
	 * 1) if the key doesn't exist, a new set by the key will be created,
	 *    and add the members to the set
	 * 2) if the key exists and not a set's key, then error happened
	 * @param key {const char*} ���϶���ļ�
	 *  the key of a set
	 * @param first_member {const char*} ��һ���� NULL �ĳ�Ա
	 *  the first member of a variable args which isn't NULL, the last
	 *  arg of the args must be NULL indicating the end of args
	 * @return {int} ����ӵ������е���Ԫ�ص������������������Ե�Ԫ��
	 *  the number of elements that were added to the set, not including
	 *  all the elements already present into the set. -1 if error
	 *  happened or it isn't a set stored by the key.
	 */
	int sadd(const char* key, const char* first_member, ...);
	int sadd(const char* key, const std::vector<const char*>& memsbers);
	int sadd(const char* key, const std::vector<string>& members);
	int sadd(const char* key, const char* argv[], size_t argc);
	int sadd(const char* key, const char* argv[], const size_t lens[],
		size_t argc);

	/**
	 * �Ӽ��϶���������Ƴ�������ĳ����Ա
	 * remove and get one member from the set
	 * @param key {const char*} ���϶���ļ�
	 *  the key of the set
	 * @param buf {string&} �洢���Ƴ��ĳ�Ա
	 *  store the member removed from the set
	 * @return {bool} �� key �����ڻ� key �ǿռ�ʱ���� false
	 *  true if one member has been removed and got, false if the key
	 *  doesn't exist or it isn't a set stored at the key.
	 */
	bool spop(const char* key, string& buf);

	/**
	 * ��ü��϶����г�Ա������
	 * get the number of members in a set stored at the key
	 * @param key {const char*} ���϶���ļ�
	 *  the key of the set
	 * @return {int} ���ظü��϶����г�Ա�������������£�
	 *  return int value as below:
	 *  -1�������Ǽ��϶���
	 *      error or it's not a set by the key
	 *   0����Ա����Ϊ�ջ�� key ������
	 *      the set is empty or the key doesn't exist
	 *  >0����Ա�����ǿ�
	 *      the number of members in the set
	 */
	int scard(const char* key);

	/**
	 * ���ؼ��� key �е����г�Ա
	 * get all the members in a set stored at a key
	 * @param key {const char*} ���϶���ļ�ֵ
	 *  the key of the set
	 * @param members {std::vector<string>*} �ǿ�ʱ�洢�����
	 *  if not NULL, it will store the members.
	 * @return {int} ��������������� -1 ��ʾ�������һ�� key �Ǽ��϶���
	 *  the number of elements got, -1 if error happened or it't not
	 *  a set by the key.
	 *
	 *  �����ɹ������ͨ��������һ��ʽ�������
	 *  if successul, one of below ways can be used to get the result:
	 *  1���ڵ��÷����д���ǿյĴ洢�������ĵ�ַ
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function 
	 *  2�����෽�� get_value ���ָ���±��Ԫ������
	 *     call redis_command::result_value with the specified subscript
	 *  3�����෽�� get_child ���ָ���±��Ԫ�ض���(redis_result����Ȼ����ͨ��
	 *     redis_result::argv_to_string �������Ԫ������
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 *  4�����෽�� get_result ����ȡ���ܽ�������� redis_result��Ȼ����ͨ��
	 *     redis_result::get_child ���һ��Ԫ�ض���Ȼ����ͨ����ʽ 2 ��ָ��
	 *     �ķ�����ø�Ԫ�ص�����
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above
	 *  5�����෽�� get_children ��ý��Ԫ�����������ͨ�� redis_result ��
	 *     �ķ��� argv_to_string ��ÿһ��Ԫ�ض����л��Ԫ������
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string
	 */
	int smembers(const char* key, std::vector<string>* members);

	/**
	 * �� member Ԫ�ش� src �����ƶ��� dst ����
	 * move a member from one set to another
	 * @param src {const char*} Դ���϶���ļ�ֵ
	 *  the source key of a set
	 * @param dst {const char*} Ŀ�꼯�϶���ļ�ֵ
	 *  the destination key of a set
	 * @param member {const char*} Դ���϶���ĳ�Ա
	 *  the member in the source set
	 * @return {int} ����ֵ�������£�
	 *  return int value as below:
	 *  -1�������Դ/Ŀ�������һ���Ǽ��϶���
	 *      error happened, or one of source and destination isn't a set
	 *   0��Դ���󲻴��ڻ��Ա��Դ�����в�����
	 *     the source set or the member doesn't exist
	 *   1���ɹ���Դ�����н�һ����Ա�ƶ���Ŀ�������
	 *      move successfully the member from source set to
	 *      the destination set
	 */
	int smove(const char* src, const char* dst, const char* member);
	int smove(const char* src, const char* dst, const string& member);
	int smove(const char* src, const char* dst,
		const char* member, size_t len);

	/**
	 * ����һ�����ϵ�ȫ����Ա���ü��������и�������֮��Ĳ
	 * return the members of the set resulting from the difference
	 * between the first set and all the successive sets.
	 * @param members {std::vector<string>*} �ǿ�ʱ�洢�����
	 *  if not NULL, it will store the members.
	 * @param first_key {const char*} ��һ���ǿյļ��϶��� key
	 *  the key of the first set in a variable sets list, the last one
	 *  must be NULL indicating the end of the sets list.
	 * @return {int} ��������������� -1 ��ʾ�������һ�� key �Ǽ��϶���
	 *  the number of elements got, -1 if error happened or it't not
	 *  a set by the key.
	 *  �����ɹ������ͨ��������һ��ʽ�������
	 *  if successul, one of below ways can be used to get the result:
	 *  1���ڵ��÷����д���ǿյĴ洢�������ĵ�ַ
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function
	 *  2�����෽�� get_value ���ָ���±��Ԫ������
	 *     get the specified subscript's element by redis_command::get_value
	 *  3�����෽�� get_child ���ָ���±��Ԫ�ض���(redis_result����Ȼ����ͨ��
	 *     redis_result::argv_to_string �������Ԫ������
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 *  4�����෽�� get_result ����ȡ���ܽ�������� redis_result��Ȼ����ͨ��
	 *     redis_result::get_child ���һ��Ԫ�ض���Ȼ����ͨ����ʽ 2 ��ָ��
	 *     �ķ�����ø�Ԫ�ص�����
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 *  5�����෽�� get_children ��ý��Ԫ�����������ͨ�� redis_result ��
	 *     �ķ��� argv_to_string ��ÿһ��Ԫ�ض����л��Ԫ������
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string
	 */
	int sdiff(std::vector<string>* members, const char* first_key, ...);
	int sdiff(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sdiff(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * ����һ�����ϵ�ȫ����Ա���ü��������и������ϵĽ���
	 * return the members of a set resulting from the intersection of
	 * all the give sets.
	 * @param members {std::vector<string>*} �ǿ�ʱ�洢�����
	 *  if not NULL, it will store the result
	 * @param first_key {const char*} ��һ�����϶��� key����NULL��
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  the last one must be NULL in the set list.
	 * @return {int} ��������������� -1 ��ʾ�������һ�� key �Ǽ��϶���
	 *  return the number of the members, -1 if error happened or
	 *  it't not a set by the key.
	 */
	int sinter(std::vector<string>* members, const char* first_key, ...);
	int sinter(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sinter(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * ����һ�����ϵ�ȫ����Ա���ü��������и������ϵĲ���
	 * return the members of a set resulting from the union of all the
	 * given sets.
	 * @param members {std::vector<string>*} �ǿ�ʱ�洢�����
	 *  if not NULL, it will store the result
	 * @param first_key {const char*} ��һ�����϶��� key����NULL��
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} ��������������� -1 ��ʾ�������һ�� key �Ǽ��϶���
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sunion(std::vector<string>* members, const char* first_key, ...);
	int sunion(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sunion(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * �����������ú� SDIFF ���ƣ�������������浽 dst ���ϣ������Ǽ򵥵ط��ؽ����
	 * This command is equal to SDIFF, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} Ŀ�꼯�϶����ֵ
	 *  the key of the destination set
	 * @param first_key {const char*} ��һ���ǿյļ��϶����ֵ
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list. 
	 * @return {int} ������еĳ�Ա����
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sdiffstore(const char* dst, const char* first_key, ...);
	int sdiffstore(const char* dst, const std::vector<const char*>& keys);
	int sdiffstore(const char* dst, const std::vector<string>& keys);

	/**
	 * ������������� SINTER ���������������浽 dst ���ϣ������Ǽ򵥵ط��ؽ����
	 * This command is equal to SINTER, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} Ŀ�꼯�϶����ֵ
	 *  the key of the destination set
	 * @param first_key {const char*} ��һ���ǿյļ��϶����ֵ
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} ������еĳ�Ա����
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sinterstore(const char* dst, const char* first_key, ...);
	int sinterstore(const char* dst, const std::vector<const char*>& keys);
	int sinterstore(const char* dst, const std::vector<string>& keys);

	/**
	 * ������������� SUNION ���������������浽 dst ���ϣ������Ǽ򵥵ط��ؽ����
	 * This command is equal to SUNION, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} Ŀ�꼯�϶����ֵ
	 *  the key of the destination set
	 * @param first_key {const char*} ��һ���ǿյļ��϶����ֵ
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} ������еĳ�Ա����
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sunionstore(const char* dst, const char* first_key, ...);
	int sunionstore(const char* dst, const std::vector<const char*>& keys);
	int sunionstore(const char* dst, const std::vector<string>& keys);

	/**
	 * �ж� member Ԫ���Ƿ񼯺� key �ĳ�Ա
	 * determine if a given value is a member of a set
	 * @param key {const char*} ���϶���ļ�ֵ
	 *  the key of a set
	 * @param member {const char*} ����ֵ
	 *  the given value
	 * @return {bool} ���� true ��ʾ�ǣ������������Ϊ���ǻ������ key ����
	 *  �Ǽ��϶���
	 *  true if the given is a member of the set, false if it's not a
	 *  member of the set, or error, or it's not a set by the key.
	 */
	bool sismember(const char* key, const char* member);
	bool sismember(const char* key, const char* member, size_t len);

	/**
	 * �������ִ��ʱ��ֻ�ṩ�� key ��������ô���ؼ����е�һ�����Ԫ�أ������ͬʱָ��
	 * ��Ԫ�ظ�������᷵��һ���������ø������ƵĽ����
	 * get one or multiple memebers from a set
	 * @param key {const char*} ���϶���ļ�ֵ
	 *  the key of a set
	 * @param out �洢���������
	 *  store the result
	 * @return {int} ����ĸ�����Ϊ -1 ��ʾ����0 ��ʾû�г�Ա
	 *  the number of members, 0 if the set by the key is empty,
	 *  -1 if error happened.
	 */
	int srandmember(const char* key, string& out);
	int srandmember(const char* key, size_t n, std::vector<string>& out);

	/**
	 * �Ƴ����� key �е�һ������ member Ԫ�أ������ڵ� member Ԫ�ػᱻ����
	 * Remove the specified members from the set stored at key. if the
	 * member doesn't exist, it will be ignored.
	 * @param key {const char*} ���϶���ļ�ֵ
	 *  the key of the set
	 * @param first_member {const char*} ��Ҫ���Ƴ��ĳ�Ա�б�ĵ�һ���� NULL��Ա��
	 *  �ڱ�ε���������Ҫ�����һ�����д NULL
	 *  the first non-NULL member to be removed in a variable member list,
	 *  and the last one must be NULL indicating the end of the list.
	 * @retur {int} ���Ƴ��ĳ�ԱԪ�صĸ������������Ǽ��϶���ʱ���� -1���� key ��
	 *  ���ڻ��Ա������ʱ���� 0
	 *  the number of members be removed, 0 if the set is empty or the
	 *  key doesn't exist, -1 if error happened or it's not a set by key
	 */
	int srem(const char* key, const char* first_member, ...);
	int srem(const char* key, const std::vector<string>& members);
	int srem(const char* key, const std::vector<const char*>& members);
	int srem(const char* key, const char* members[],
		size_t lens[], size_t argc);

	/**
	 * �������ڵ�����ǰ���ݿ��е����ݿ��
	 * scan the members in a set stored at key
	 * @param key {const char*} ��ϣ��ֵ
	 *  the key of a set
	 * @param cursor {int} �α�ֵ����ʼ����ʱ��ֵд 0
	 *  the cursor value, which is 0 at begin
	 * @param out {std::vector<string>&} �洢��������ڲ���׷�ӷ�ʽ�����α���
	 *  ���������ӽ��������У�Ϊ��ֹ���ܽ���������¸�����������û����ڵ��ñ�
	 *  ����ǰ��������������
	 *  store result in appending mode.
	 * @param pattern {const char*} ƥ��ģʽ��glob ��񣬷ǿ�ʱ��Ч
	 *  match pattern, effective only on no-NULL
	 * @param count {const size_t*} �޶��Ľ�����������ǿ�ָ��ʱ��Ч
	 *  the max count of one scan process, effective only on no-NULL
	 * @return {int} ��һ���α�λ�ã��������£�
	 *  return the next cursor position, as below:
	 *   0����������
	 *     scan finish
	 *  -1: ����
	 *     some error happened
	 *  >0: �α����һ��λ��
	 *     the next cursor postion to scan
	 */
	int sscan(const char* key, int cursor, std::vector<string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl
