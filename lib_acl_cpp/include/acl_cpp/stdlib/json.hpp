#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include <vector>
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/pipe_stream.hpp"

struct ACL_JSON_NODE;
struct ACL_JSON;
struct ACL_ITER;

/**
 * �� ACL ���� json ������ķ�װ������ C++ �û�ʹ�ã������̫ע���������أ�
 * ����ֱ��ʹ�ø��࣬����ڷ����ִ���ҷǳ�ע�����ܣ�����ֱ��ʹ�� ACL ���
 * json ����������Ϊ����Ҳ�ǵ����� ACL ���е� json �������̣������ж��ο���
 * ���̣����ܻ���΢Ӱ��һЩ���ܣ�������һ���Ӧ�����Ӱ����΢�������
 */

namespace acl {

class string;
class json;

/**
 * json �ڵ㣬������������ json.create_node() ��ʽ����
 */
class ACL_CPP_API json_node : public dbuf_obj
{
public:
	/**
	 * ȡ�ñ� json �ڵ�ı�ǩ��
	 * @return {const char*} ���� json �ڵ��ǩ����������ؿգ���˵��
	 *  ��������Ҫ�жϷ���ֵ
	 */
	const char* tag_name(void) const;

	/**
	 * ���ظ� json �ڵ���ı���ǩֵ������ֵΪ�����ͻ���ֵ��ʱ�����߿�
	 * ���н���ת��
	 * @return {const char*} ���ؿ�˵��û���ı���ǩֵ
	 */
	const char* get_text(void) const;

	/**
	 * ���� json �ڵ�����ӽڵ�ʱ�����ر� json �ڵ��ǩ��Ӧ�� json �ӽڵ�
	 * @param {const json_node*} ���� NULL ˵���������ӽڵ�
	 *  ע��get_text �� get_obj ����ͬʱ���ط� NULL
	 */
	json_node* get_obj(void) const;

	/**
	 * �� json �ڵ�Ϊ�ַ�������ʱ���ú��������ַ�������
	 * @return {const char*} ���� NULL ��ʾ�ýڵ���ַ�������
	 */
	const char* get_string(void) const;

	/**
	 * �� json �ڵ�Ϊ����������ʱ���ú������س�����ֵ��ָ���ַ
	 * @return {const long long int*} ������ NULL ʱ��ʾ�ö���ǳ���������
	 */
#if defined(_WIN32) || defined(_WIN64)
	const __int64* get_int64(void) const;
#else
	const long long int* get_int64(void) const;
#endif

	/**
	 * �� json �ڵ�Ϊ��������ʱ���ú������س�����ֵ��ָ���ַ
	 * @return {const double*} ������ NULL ʱ��ʾ�ö���Ǹ�������
	 */
	const double *get_double(void) const;

	/**
	 * �� json �ڵ�Ϊ��������ʱ���ú������ز���ֵ��ָ���ַ
	 * @return {bool*} ������ NULL ʱ��ʾ�ö���ǲ�������
	 */
	const bool* get_bool(void) const;

	/**
	 * �жϱ��ڵ������Ƿ�Ϊ�ַ�������
	 * @return {bool}
	 */
	bool is_string(void) const;

	/**
	 * �жϱ��ڵ������Ƿ�Ϊ��������
	 * @return {bool}
	 */
	bool is_number(void) const;

	/**
	 * �жϱ��ڵ������Ƿ�Ϊ��������
	 * @return {bool}
	 */
	bool is_double(void) const;

	/**
	 * �жϱ��ڵ������Ƿ�Ϊ��������
	 * @return {bool}
	 */
	bool is_bool(void) const;

	/**
	 * �жϱ��ڵ������Ƿ�Ϊ null ����
	 * @return {bool}
	 */
	bool is_null(void) const;

	/**
	 * �жϱ��ڵ��Ƿ�Ϊ��������
	 * @return {bool}
	 */
	bool is_object(void) const;

	/**
	 * �жϱ��ڵ��Ƿ�Ϊ��������
	 * @return {bool}
	 */
	bool is_array(void) const;

	/**
	 * ��øýڵ����͵�����
	 * @return {const char*}
	 */
	const char* get_type(void) const;

	/**
	 * ���� json �ڵ��б�ǩʱ�������������µı�ǩֵ���Ǿɵı�ǩ��
	 * @param name {const char*} �µı�ǩֵ��Ϊ�ǿ��ַ���
	 * @return {bool} ���� false ��ʾ�ýڵ�û�б�ǩ������մ���û�н����滻
	 */
	bool set_tag(const char* name);

	/**
	 * ���� json �ڵ�ΪҶ�ڵ�ʱ�������������滻�ڵ���ı�ֵ
	 * @param text {const char*} �µ�Ҷ�ڵ��ı�ֵ��Ϊ�ǿ��ַ���
	 * @return {bool} ���� false ��ʾ�ýڵ��Ҷ�ڵ������Ƿ�
	 */
	bool set_text(const char* text);

	/**
	 * ����ǰ json �ڵ�ת���� json �ַ���(������ json �ڵ㼰���ӽڵ�)
	 * @return {const char*}
	 */
	const string& to_string(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * ���� json �ڵ���� json_node �ӽڵ����
	 * @param child {json_node*} �ӽڵ����
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�����ӽڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_child(json_node* child, bool return_child = false);

	/**
	 * ���� json �ڵ���� json_node �ӽڵ����
	 * @param child {json_node&} �ӽڵ����
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�����ӽڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_child(json_node& child, bool return_child = false);

	/**
	 * ����һ�� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param as_array {bool} �Ƿ��������
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_array(bool return_child = false);
	json_node& add_child(bool as_array = false, bool return_child = false);

	/**
	 * ����һ�� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_child(const char* tag, bool return_child = false);

	/**
	 * ����һ�� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param node {json_node*} ��ǩֵָ��
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_child(const char* tag, json_node* node,
		bool return_child = false);

	/**
	 * ����һ�� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param node {json_node&} ��ǩֵ����
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_child(const char* tag, json_node& node,
		bool return_child = false);

	/**
	 * ����һ���ַ������͵� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param value {const char*} ��ǩֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 * ע���˴��� add_text �� add_child ��ͬ���Ĺ���
	 */
	json_node& add_text(const char* tag, const char* value,
		bool return_child = false);

	/**
	 * ����һ��int64 ���͵� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param value {int64} ��ǩֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& add_number(const char* tag, __int64 value,
		bool return_child = false);
#else
	json_node& add_number(const char* tag, long long int value,
		bool return_child = false);
#endif

	/**
	 * ����һ�� double ���͵� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param value {double} ��ǩֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_double(const char* tag, double value,
		bool return_child = false);

	/**
	 * ����һ���������͵� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param value {bool} ��ǩֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_bool(const char* tag, bool value,
		bool return_child = false);

	/**
	 * ����һ�� null ���͵� json �ڵ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param tag {const char*} ��ǩ��
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_null(const char* tag, bool return_child = false);

	/**
	 * ����һ�� json �ַ������󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param text {const char*} �ı��ַ���
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_array_text(const char* text,
		bool return_child = false);

	/**
	 * ����һ�� json ���ֶ��󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param value {acl_int64} ����ֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& add_array_number(__int64 value,
		bool return_child = false);
#else
	json_node& add_array_number(long long int value,
		bool return_child = false);
#endif

	/**
	 * ����һ�� json double ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param value {double} ֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_array_double(double value, bool return_child = false);

	/**
	 * ����һ�� json �������󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param value {bool} ����ֵ
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_array_bool(bool value, bool return_child = false);

	/**
	 * ����һ�� json null ���󣬲���֮���Ϊ�� json �ڵ���ӽڵ�
	 * @param return_child {bool} �Ƿ���Ҫ�����������´������ӽڵ������
	 * @return {json_node&} return_child Ϊ true ʱ�������½ڵ�����ã�
	 *  ���򷵻ر� json �ڵ���������
	 */
	json_node& add_array_null(bool return_child = false);

	/**
	 * @return {json_node&} ���ر��ڵ�ĸ��ڵ����ã��ڲ��ü�����ʽ���� json
	 *  ����ʱ���������������ڷ��ظ��ڵ�
	 */
	json_node& get_parent(void) const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * ��ñ��ڵ�ĵ�һ���ӽڵ㣬��Ҫ�����ӽڵ�ʱ�������ȵ��ô˺���
	 * @return {json_node*} ���ؿձ�ʾû���ӽڵ㣬���صķǿն�����
	 *  ���ⲿ delete����Ϊ�ڲ����Զ��ͷ�
	 */
	json_node* first_child(void);

	/**
	 * ��ñ��ڵ����һ���ӽڵ�
	 * @return {json_node*} ���ؿձ�ʾ�������̽��������صķǿն�����
	 *  ���ⲿ delete����Ϊ�ڲ����Զ��ͷ�
	 */
	json_node* next_child(void);

	/**
	 * �ӵ�ǰ json �ڵ���ӽڵ�����ȡ��Ӧ��ǩ�� json �ӽڵ�
	 * @param tag {const char*} json �ӽڵ�ı�ǩ��
	 * @return {json_node*} ���� NULL ��ʾ������
	 */
	json_node* operator[] (const char* tag);

	/**
	 * ���ظ� json �ڵ������� json ���е����
	 * @return {int}
	 */
	int   depth(void) const;

	/**
	 * ���ظ� json �ڵ����һ���ӽڵ�ĸ���
	 * @return {int} ��Զ >= 0
	 */
	int   children_count(void) const;

	/**
	 * �����ڵ㼰���ӽڵ�� json ����ɾ�������ڴ潫�� json ����ͳһ�ͷ�
	 * @return {int} ���ͷŵĽڵ�����
	 */
	int detach(void);

	/**
	 * ���ڱ����� json �ڵ�ʱ���ڲ��ᶯ̬����һЩ��ʱ json_node ���󣬵���
	 * �˺������������Щ����һ�����ô˺������������������ first_child,
	 * next_child ���ص� json_node �ڵ���󽫲��ٿ��ã����������ڴ�Ƿ�
	 * ����
	 */
	void clear(void);

	/**
	 * ��� json ���������
	 * @return {json&}
	 */
	json& get_json(void) const;

	/**
	 * ȡ����Ӧ�� ACL ���е� json �ڵ����
	 * @return {ACL_JSON_NODE*} ���ؽڵ����ע���ýڵ��û����ܵ����ͷ�
	 */
	ACL_JSON_NODE* get_json_node(void) const;

private:
	friend class json;
	/**
	 * ���캯����Ҫ��ö������ֻ���� json ���󴴽�
	 * @param node {ACL_JSON_NODE*} ACL ���е� ACL_JSON_NODE �ṹ����
	 */
	json_node(ACL_JSON_NODE* node, json* json_ptr);

	/**
	 * Ҫ��ö�������Ƕ�̬������
	 */
	~json_node(void);

	/**
	 * ���� json �ڵ�
	 * @param node {ACL_JSON_NODE*}
	 */
	void set_json_node(ACL_JSON_NODE* node);

private:
	ACL_JSON_NODE* node_me_;
	json* json_;
	json_node* parent_;
	json_node* parent_saved_;
	std::vector<json_node*>* children_;
	ACL_ITER* iter_;
	string* buf_;
	json_node* obj_;

	union
	{
#if defined(_WIN32) || defined(_WIN64)
		__int64 n;
#else
		long long int n;
#endif
		bool   b;
		double d;
	} node_val_;

	void prepare_iter(void);
};

class ACL_CPP_API json : public pipe_stream, public dbuf_obj
{
public:
	/**
	 * ���캯���������ڽ��� json �ַ��������� json ����
	 * @param data {const char*} json ��ʽ���ַ�����������������
	 *  json �ַ�����Ҳ�����ǲ��ֵ� json �ַ�����Ҳ�����ǿ�ָ�룬
	 *  ������Σ��û���Ȼ�����ò��ֻ������� json �ַ������� update
	 *  �������ڵ��� update �����н��� json����ʵ�������캯����
	 *  �� data �����ǿ�ʱ����Ҳ����� update
	 */
	json(const char* data = NULL);

	/**
	 * ����һ�� json �����е�һ�� json �ڵ㹹��һ���µ� json ����
	 * @param node {const json_node&} Դ json �����е�һ�� json �ڵ�
	 */
	json(const json_node& node);

	~json(void);

	/**
	 * �����Ƿ��ڽ���ʱ�Զ����������ֵ�����
	 * @param on {bool}
	 * @return {json&}
	 */
	json& part_word(bool on);

	/**
	 * ����ʽ��ʽѭ�����ñ�������� json ���ݣ�Ҳ����һ�������
	 * ������ json ���ݣ�������ظ�ʹ�ø� json ������������� json
	 * ������Ӧ���ڽ�����һ�� json ����ǰ���� reset() ��������
	 * ����һ�εĽ������
	 * @param data {const char*} json ����
	 @return {const char*} �����������󣬸÷���ֵ��ʾʣ�����ݵ�ָ���ַ
	 */
	const char* update(const char* data);

	/**
	 * �ж��Ƿ�������
	 * @return {bool}
	 */
	bool finish(void);

	/**
	 * ���� json ������״̬���� json ������������Զ�� json ����
	 * ���н������ڷ���ʹ�ñ� json ������ǰ����Ҫ���ñ���������
	 * �ڲ� json ������״̬�������һ�εĽ������
	 */
	void reset(void);

	/**
	 * �� json ������ȡ��ĳ����ǩ���ĵ�һ���ڵ�
	 * @param tag {const char*} ��ǩ��(�����ִ�Сд)
	 * @return {json_node*} ���� json �ڵ���󣬲������򷵻� NULL
	 *  ע�����ص� json_node �ڵ�����޸ģ�������ɾ���ڵ㣬�ڲ����Զ�ɾ�����ƣ�
	 *  �����÷��� clear/getElementsByTagName/getElementsByTags �󣬽ڵ�
	 *  �����ٱ����ã���Ϊ�ڵ���ڴ汻�Զ��ͷ�
	 */
	json_node* getFirstElementByTagName(const char* tag) const;

	/**
	 * �����������ֱ�ӻ�ö�Ӧ��ǩ���ĵ�һ���ڵ�
	 * @param tag {const char*} ��ǩ��(�����ִ�Сд)
	 * @return {json_node*} ���� json �ڵ���󣬲������򷵻� NULL
	 *  ע�����ص� json_node �ڵ�����޸ģ�������ɾ���ڵ㣬�ڲ����Զ�ɾ�����ƣ�
	 *  �����÷��� clear/getElementsByTagName/getElementsByTags �󣬽ڵ�
	 *  �����ٱ����ã���Ϊ�ڵ���ڴ汻�Զ��ͷ�
	 */
	json_node* operator[](const char* tag) const;

	/**
	 * �� json ������ȡ��ĳ����ǩ�������нڵ㼯��
	 * @param tag {const char*} ��ǩ��(�����ִ�Сд)
	 * @return {const std::vector<json_node*>&} ���ؽ�����Ķ������ã�
	 *  �����ѯ���Ϊ�գ���ü���Ϊ�գ�����empty() == true
	 *  ע�����ص� json_node �ڵ�����޸ģ�������ɾ���ڵ㣬�ڲ����Զ�ɾ�����ƣ�
	 *  �����÷��� clear/getElementsByTagName/getElementsByTags �󣬽ڵ�
	 *  �����ٱ����ã���Ϊ�ڵ���ڴ汻�Զ��ͷ�
	 */
	const std::vector<json_node*>&
		getElementsByTagName(const char* tag) const;

	/**
	 * �� json �����л�����е�������༶��ǩ����ͬ�� json �ڵ�ļ���
	 * @param tags {const char*} �༶��ǩ������ '/' �ָ�������ǩ����
	 *  ����� json ���ݣ�
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  ����ͨ���༶��ǩ����root/first/second/third һ���Բ�����з���
	 *  �����Ľڵ�
	 * @return {const std::vector<json_node*>&} ���������� json �ڵ㼯��, 
	 *  �����ѯ���Ϊ�գ���ü���Ϊ�գ�����empty() == true
	 *  ע�����ص� json_node �ڵ�����޸ģ�������ɾ���ڵ㣬�ڲ����Զ�ɾ�����ƣ�
	 *  �����÷��� clear/getElementsByTagName/getElementsByTags �󣬽ڵ�
	 *  �����ٱ����ã���Ϊ�ڵ���ڴ汻�Զ��ͷ�
	 */
	const std::vector<json_node*>&
		getElementsByTags(const char* tags) const;

	/**
	 * �� json �����л�����е�������༶��ǩ����ͬ�� json �ڵ�ļ���
	 * @param tags {const char*} �༶��ǩ������ '/' �ָ�������ǩ����
	 *  ����� json ���ݣ�
	 *  { 'root': [
	 *      'first': { 'second': { 'third': 'test1' } },
	 *      'first': { 'second': { 'third': 'test2' } },
	 *      'first': { 'second': { 'third': 'test3' } }
	 *    ]
	 *  }
	 *  ����ͨ���༶��ǩ����root/first/second/third һ���Բ�����з���
	 *  �����Ľڵ�
	 * @return {json_node*} ���� NULL ��ʾ������
	 */
	json_node* getFirstElementByTags(const char* tags) const;

	/**
	 * ȡ�� acl ���е� ACL_JSON ����
	 * @return {ACL_JSON*} ��ֵ������Ϊ�գ�ע���û������޸ĸö����ֵ��
	 *  ���������ͷŸö���
	 */
	ACL_JSON* get_json(void) const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * ����һ�� json_node Ҷ�ڵ���󣬸ýڵ����ĸ�ʽΪ��
	 * "tag_name": "tag_value"
	 * @param tag {const char*} ��ǩ��
	 * @param value {const char*} ��ǩֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_node(const char* tag, const char* value);

	/**
	 * ����һ�� json_node Ҷ�ڵ���󣬸ýڵ����ĸ�ʽΪ��
	 * "tag_name": tag_value
	 * @param tag {const char*} ��ǩ��
	 * @param value {int64} ��ǩֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& create_node(const char* tag, __int64 value);
#else
	json_node& create_node(const char* tag, long long int value);
#endif

	/**
	 * ����һ�� json_node Ҷ�ڵ���󣬸ýڵ����ĸ�ʽΪ��
	 * "tag_name": tag_value
	 * @param tag {const char*} ��ǩ��
	 * @param value {double} ��ǩֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_double(const char* tag, double value);

	/**
	 * ����һ�� json_node Ҷ�ڵ���󣬸ýڵ����ĸ�ʽΪ��
	 * "tag_name": true|false
	 * @param tag {const char*} ��ǩ��
	 * @param value {bool} ��ǩֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_node(const char* tag, bool value);

	/**
	 * ����һ�� json_node null Ҷ�ڵ���󣬸ýڵ����ĸ�ʽΪ��
	 * "tag_name": null
	 * @param tag {const char*} ��ǩ��
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_null(const char* tag);

	/**
	 * ����һ�� json_node Ҷ�ڵ��ַ������󣬸ýڵ����ĸ�ʽΪ��"string"
	 * �� json �淶���ýڵ�ֻ�ܼ��������������
	 * @param text {const char*} �ı��ַ���
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_array_text(const char* text);

	/**
	 * ����һ�� json_node Ҷ�ڵ���ֵ����
	 * �� json �淶���ýڵ�ֻ�ܼ��������������
	 * @param value {acl_int64} ��ֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 * ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
#if defined(_WIN32) || defined(_WIN64)
	json_node& create_array_number(__int64 value);
#else
	json_node& create_array_number(long long int value);
#endif

	/**
	 * ����һ�� json_node Ҷ�ڵ���ֵ����
	 * �� json �淶���ýڵ�ֻ�ܼ��������������
	 * @param value {double} ֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 * ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_array_double(double value);

	/**
	 * ����һ�� json_node Ҷ�ڵ㲼������
	 * �� json �淶���ýڵ�ֻ�ܼ��������������
	 * @param value {bool} ����ֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 * ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_array_bool(bool value);

	/**
	 * ����һ�� json_node Ҷ�ڵ� null ����
	 * �� json �淶���ýڵ�ֻ�ܼ��������������
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 * ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_array_null(void);

	/**
	 * ����һ�� json_node �ڵ��������󣬸ö���û�б�ǩ,
	 * �ýڵ����ĸ�ʽΪ��"{}" ��������� "[]"
	 * @param as_array {bool} �Ƿ��������
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_node(bool as_array = false);
	json_node& create_array(void);

	/**
	 * ����һ�� json_node �ڵ���󣬸ýڵ����ĸ�ʽΪ��tag_name: {}
	 * �� tag_name: []
	 * @param tag {const char*} ��ǩ��
	 * @param node {json_node*} json �ڵ������Ϊ��ǩֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_node(const char* tag, json_node* node);

	/**
	 * ����һ�� json_node �ڵ���󣬸ýڵ����ĸ�ʽΪ��tag_name: {}
	 * �� tag_name: []
	 * @param tag {const char*} ��ǩ��
	 * @param node {json_node&} json �ڵ������Ϊ��ǩֵ
	 * @return {json_node&} �²����� json_node ������Ҫ�û��ֹ��ͷţ�
	 *  ��Ϊ�� json �����ͷ�ʱ��Щ�ڵ���Զ����ͷţ���Ȼ�û�Ҳ������
	 *  ����ʱ���� reset ���ͷ���Щ json_node �ڵ����
	 */
	json_node& create_node(const char* tag, json_node& node);

	/////////////////////////////////////////////////////////////////////

	/**
	 * ��һ�� json �����е�һ�� json �ڵ㸴������һ json �����е�һ��
	 * �� json �ڵ��в����ظ��µ� json �ڵ�
	 * @param node {json_node*} Դ json �����һ�� json �ڵ�
	 * @return {json_node&} ��ǰĿ�� json �������´����� json �ڵ�
	 */
	json_node& duplicate_node(const json_node* node);

	/**
	 * ��һ�� json �����е�һ�� json �ڵ㸴������һ json �����е�һ��
	 * �� json �ڵ��в����ظ��µ� json �ڵ�
	 * @param node {json_node&} Դ json �����һ�� json �ڵ�
	 * @return {json_node&} ��ǰĿ�� json �������´����� json �ڵ�
	 */
	json_node& duplicate_node(const json_node& node);

	/**
	 * ��ø��ڵ����
	 * @return {json_node&}
	 */
	json_node& get_root(void);

	/**
	 * ��ʼ������ json ���󲢻�õ�һ���ڵ�
	 * @return {json_node*} ���ؿձ�ʾ�� json ����Ϊ�սڵ�
	 *  ע�����صĽڵ�����û������ֹ��ͷţ���Ϊ�ö���
	 *  �ڲ����Զ��ͷ�
	 */
	json_node* first_node(void);

	/**
	 * ������ json �������һ�� json �ڵ�
	 * @return {json_node*} ���ؿձ�ʾ�������
	 *  ע�����صĽڵ�����û������ֹ��ͷţ���Ϊ�ö���
	 *  �ڲ����Զ��ͷ�
	 */
	json_node* next_node(void);

	/**
	 * �� json ������ת���ַ���
	 * @param out {string&} �洢ת������Ļ�����
	 */
	void build_json(string& out) const;

	/**
	 * �� json ������ת���� json �ַ���
	 * @return {const string&}
	 */
	const string& to_string(void);

	// pipe_stream �麯������

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

private:
	// ��Ӧ�� acl ���е� ACL_JSON ����
	ACL_JSON *json_;
	// json �������еĸ��ڵ����
	json_node* root_;
	// ��ʱ�� json �ڵ��ѯ�����
	std::vector<json_node*> nodes_query_;
	// �ɸ� json ��������� json �ڵ㼯��
	std::list<json_node*> nodes_tmp_;
	// ������
	string* buf_;
	ACL_ITER* iter_;
};

} // namespace acl
