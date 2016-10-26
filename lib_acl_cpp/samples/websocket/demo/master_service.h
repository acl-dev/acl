#pragma once

////////////////////////////////////////////////////////////////////////////////
// ����������

extern acl::master_str_tbl var_conf_str_tab[];
extern acl::master_bool_tbl var_conf_bool_tab[];
extern acl::master_int_tbl var_conf_int_tab[];
extern acl::master_int64_tbl var_conf_int64_tab[];

////////////////////////////////////////////////////////////////////////////////

//class acl::socket_stream;

class master_service : public acl::master_threads
{
public:
	master_service();
	~master_service();

protected:
	/**
	 * ���麯������ĳ���ͻ������������ݿɶ���رջ����ʱ���ô˺���
	 * @param stream {socket_stream*}
	 * @return {bool} ���� false ���ʾ���������غ���Ҫ�ر����ӣ�
	 *  �����ʾ��Ҫ���ֳ����ӣ��������������Ӧ��Ӧ�÷��� false
	 */
	virtual bool thread_on_read(acl::socket_stream* stream);

	/**
	 * ���̳߳��е�ĳ���̻߳��һ������ʱ�Ļص�������
	 * ���������һЩ��ʼ������
	 * @param stream {socket_stream*}
	 * @return {bool} ������� false ���ʾ����Ҫ��ر����ӣ�����
	 *  �ؽ��������ٴ����� thread_main ����
	 */
	virtual bool thread_on_accept(acl::socket_stream* stream);

	/**
	 * ��ĳ���������ӵ� IO ��д��ʱʱ�Ļص�����������ú������� true ���ʾ�����ȴ���һ��
	 * ��д��������ϣ���رո�����
	 * @param stream {socket_stream*}
	 * @return {bool} ������� false ���ʾ����Ҫ��ر����ӣ�����
	 *  �ؽ��������ٴ����� thread_main ����
	 */
	virtual bool thread_on_timeout(acl::socket_stream* stream);

	/**
	 * ����ĳ���̰߳󶨵����ӹر�ʱ�Ļص�����
	 * @param stream {socket_stream*}
	 */
	virtual void thread_on_close(acl::socket_stream* stream);

	/**
	 * ���̳߳���һ�����̱߳�����ʱ�Ļص�����
	 */
	virtual void thread_on_init();

	/**
	 * ���̳߳���һ���߳��˳�ʱ�Ļص�����
	 */
	virtual void thread_on_exit();

	/**
	 * �������л��û���ݺ���õĻص��������˺���������ʱ������
	 * ��Ȩ��Ϊ��ͨ���޼���
	 */
	virtual void proc_on_init();

	/**
	 * ���ӽ�����Ҫ�˳�ʱ��ܽ��ص��˺�������ܾ����ӽ����Ƿ��˳�ȡ���ڣ�
	 * 1) ����˺������� true ���ӽ��������˳�������
	 * 2) ������ӽ������пͻ������Ӷ��ѹرգ����ӽ��������˳�������
	 * 3) �鿴�����ļ��е�������(ioctl_quick_abort)�������������� 0 ��
	 *    �ӽ��������˳�������
	 * 4) �����пͻ������ӹرպ���˳�
	 * @param ncleints {size_t} ��ǰ���ӵĿͻ��˸���
	 * @param nthreads {size_t} ��ǰ�̳߳��з�æ�Ĺ����̸߳���
	 * @return {bool} ���� false ��ʾ��ǰ�ӽ��̻������˳��������ʾ��ǰ
	 *  �ӽ��̿����˳���
	 */
	virtual bool proc_exit_timer(size_t nclients, size_t nthreads);

	/**
	 * �������˳�ǰ���õĻص�����
	 */
	virtual void proc_on_exit();
};
