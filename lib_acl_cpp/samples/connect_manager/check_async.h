#pragma once

class check_async : public acl::aio_callback
{
public:
	check_async(acl::check_client& checker);

protected:
	// ���ظ��� aio_callback �е��麯��

	/**
	 * �ͻ������Ķ��ɹ��ص�����
	 * @param data {char*} ���������ݵ�ַ
	 * @param len {int} ���������ݳ���
	 * @return {bool} ���� true ��ʾ����������ϣ���رո��첽��
	 */
	bool read_callback(char* data, int len);

	/**
	 * �ͻ������ĳ�ʱ�ص�����
	 * @return {bool} ���� true ��ʾ����������ϣ���رո��첽��
	 */
	bool timeout_callback();

	/**
	 * �ͻ������ĳ�ʱ�ص�����
	 */
	void close_callback();

private:
	acl::check_client& checker_;

	// ������������Ϊ˽�з������Ӷ�Ҫ��ö����ڴ���ʱ�ǶѶ���
	~check_async(void);
};
