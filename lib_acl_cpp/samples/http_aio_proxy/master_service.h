#pragma once

////////////////////////////////////////////////////////////////////////////////
// ����������

extern acl::master_str_tbl var_conf_str_tab[];

extern acl::master_bool_tbl var_conf_bool_tab[];

extern acl::master_int_tbl var_conf_int_tab[];

////////////////////////////////////////////////////////////////////////////////

//class acl::aio_socket_stream;

class master_service : public acl::master_aio
{
public:
	master_service();
	~master_service();

protected:
	/**
	 * ���麯���������յ�һ���ͻ�������ʱ���ô˺���
	 * @param stream {aio_socket_stream*} �½��յ��Ŀͻ����첽������
	 * @return {bool} �ú���������� false ��֪ͨ��������ܲ��ٽ���
	 *  Զ�̿ͻ������ӣ�����������տͻ�������
	 */
	bool on_accept(acl::aio_socket_stream* stream);

	/**
	 * �������л��û���ݺ���õĻص��������˺���������ʱ������
	 * ��Ȩ��Ϊ��ͨ���޼���
	 */
	virtual void proc_on_init();

	/**
	 * �������˳�ǰ���õĻص�����
	 */
	virtual void proc_on_exit();
};
