#pragma once

class mymonitor : public acl::connect_monitor
{
public:
	mymonitor(acl::connect_manager& manager, const acl::string& proto);
	~mymonitor(void);

protected:
	/**
	 * ���� connect_monitor �麯�������ر�����������һ���жϸ������Ƿ��Ǵ���
	 * @param checker {check_client&} ��������ӵļ����󣬿���ͨ��
	 *  check_client ���еķ������£�
	 *  1) get_conn ��÷��������Ӿ��
	 *  2) get_addr ��÷���˵�ַ
	 *  3) set_alive ���������Ƿ���
	 *  4) close �ر�����
	 */
	void nio_check(acl::check_client& checker,
		acl::aio_socket_stream& conn);

	/**
	 * ͬ�� IO ����麯�����ú������̳߳ص�ĳ�����߳̿ռ������У�����������ر�����
	 * �Լ��ʵ��Ӧ�õ��������Ӵ��״̬�������ڱ������������� IO ����
	 * @param checker {check_client&} ��������ӵļ�����
	 *  check_client ����������õķ������£�
	 *  1) get_addr ��÷���˵�ַ
	 *  2) set_alive ���������Ƿ���
	 *  check_client ���н�ֹ���õķ������£�
	 *  1) get_conn ��÷��������Ӿ��
	 *  2) close �ر�����
	 */
	void sio_check(acl::check_client& checker, acl::socket_stream& conn);

private:
	acl::string proto_;
};
