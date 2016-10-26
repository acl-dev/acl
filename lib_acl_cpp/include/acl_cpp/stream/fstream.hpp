#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/istream.hpp"
#include "acl_cpp/stream/ostream.hpp"

namespace acl {

class string;

class ACL_CPP_API fstream
	: public istream
	, public ostream
{
public:
	fstream(void);
	virtual ~fstream(void);

	/**
	 * �����ļ�·�����ļ���, ����������Ĵ��ļ��ķ�ʽ
	 * @param path {const char*} �ļ���
	 * @param oflags {unsigned int} ��־λ, We're assuming that
	 *  O_RDONLY: 0x0000, O_WRONLY: 0x0001, O_RDWR: 0x0002,
	 *  O_APPEND: 0x0008, O_CREAT: 0x0100, O_TRUNC: 0x0200,
	 *  O_EXCL: 0x0400; just for win32, O_TEXT: 0x4000,
	 *  O_BINARY: 0x8000, O_RAW: O_BINARY, O_SEQUENTIAL: 0x0020,
	 *  O_RANDOM: 0x0010.
	 * @param mode {int} ���ļ����ʱ��ģʽ(��: 0600)
	 * @return {bool} ���ļ��Ƿ�ɹ�
	 */
	bool open(const char* path, unsigned int oflags, int mode);

	/**
	 * �Զ�/д��ʽ���ļ��������ļ�������ʱ�򴴽����ļ������ļ�����ʱ��
	 * ���ļ����, �ļ�����Ϊ 0700
	 * @param path {const char*} �ļ���
	 * @return {bool} ���ļ��Ƿ�ɹ�
	 */
	bool open_trunc(const char* path);

	/**
	 * �Զ�/д��ʽ�����ļ����ļ�����Ϊ 0700, ���ļ��������򴴽����ļ���
	 * ��������򿪾��ļ�
	 * @return {bool} �ļ������Ƿ�ɹ�
	 */
	bool create(const char* path);

	/**
	 * ����������Ӧ���ļ��Ӵ�����ɾ�����ú���ֻ�е��ڲ�֪���ļ�·��
	 * ʱ������ȷɾ���ļ��������޷�ɾ��
	 * @return {bool} ɾ���ļ��Ƿ�ɹ�
	 */
	bool remove(void);

	/**
	 * ����ǰ�ļ�������Ϊָ���ļ�������� WINDOWS ƽ̨����Ҫ�ȹرյ�ǰ�ļ�
	 * ��������������ɹ��������´��µ�Ŀ���ļ�
	 * @param from_path {const char*} Դ�ļ���
	 * @param to_path {const char*} Ŀ���ļ���
	 * @return {bool} �����Ƿ�ɹ�
	 */
	bool rename(const char* from_path, const char* to_path);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * ����ϵͳ���ļ������ fstream �ļ�������
	 * @param fh ϵͳ�ļ����
	 * @param oflags �򿪱�־λ
	 * @param path {const char*} �� NULL ʱ������Ϊ���ļ�������ļ�·��
	 *  ���洢���Ա��� file_path, remove ʹ��
	 */
	void open(void* fh, unsigned int oflags, const char* path = NULL);

	/**
	 * �ƶ��ļ�ָ��λ��
	 * @param offset {__int64} ƫ����
	 * @param whence {int} �ƶ�����SEEK_SET�����ļ���ʼλ�ú��ƶ���,
	 *  SEEK_CUR(�ӵ�ǰ�ļ�ָ��λ������ƶ�), SEEK_END(���ļ�β��ǰ�ƶ�)
	 * @return {acl_off_t} ����ʱ����ֵ >= 0������ʱ���� -1
	 */
	__int64 fseek(__int64 offset, int whence);

	/**
	 * ��õ�ǰϵͳ�ļ�ָ�����ļ��е�ƫ��λ��
	 * @return {acl_off_t} ����ʱ����ֵ >= 0������ʱ���� -1
	 */
	__int64 ftell();

	/**
	 * ���ļ��ߴ�ض���ָ����С
	 * @param length {acl_off_t} �ļ��ضϺ�Ĵ�С�ߴ�
	 * @return {bool} �Ƿ�ɹ�
	 */
	bool ftruncate(__int64 length);

	/**
	 * ��õ�ǰ�ļ��Ĵ�С
	 * @return {acl_off_t} ����ʱ����ֵ >= 0�������� -1
	 */
	__int64 fsize(void) const;

	/**
	 * ����ϵͳ�ļ����
	 * @return ϵͳ�ļ����������ʱ���� ACL_FILE_INVALID
	 */
	void* file_handle() const;
#else
	void open(int fh, unsigned int oflags, const char* path = NULL);
	long long int fseek(long long int offset, int whence);
	long long int ftell();
	bool ftruncate(long long int length);
	long long int fsize(void) const;
	int file_handle() const;
#endif
	/**
	 * ����ļ���ȫ·��
	 * @return {const char*} �����ؿ����ʾ�ļ���δ�򿪻����
	 */
	const char* file_path() const;
};

} // namespace acl
