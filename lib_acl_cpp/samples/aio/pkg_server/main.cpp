#include <iostream>
#include <assert.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static int   __timeout = 0;

typedef enum
{
	STATUS_T_HDR,
	STATUS_T_DAT,
} status_t;

// ����ͷ
struct DAT_HDR
{
	int  len;		// �����峤��
	char cmd[64];		// ������
};

/**
 * �첽�ͻ������Ļص��������
 */
class io_callback : public acl::aio_callback
{
public:
	io_callback(acl::aio_socket_stream* client)
		: status_(STATUS_T_HDR)
		, client_(client)
		, i_(0)
	{
	}

	~io_callback()
	{
		std::cout << "delete io_callback now ..." << std::endl;
	}

	/**
	 * ʵ�ָ����е��麯�����ͻ������Ķ��ɹ��ص�����
	 * @param data {char*} ���������ݵ�ַ
	 * @param len {int} ���������ݳ���
	 * @return {bool} ���� true ��ʾ����������ϣ���رո��첽��
	 */
	bool read_callback(char* data, int len)
	{
		// ��ǰ״̬�Ǵ�������ͷʱ
		if (status_ == STATUS_T_HDR)
		{
			// ����ͷ�������Ƿ����Ҫ��
			if (len != sizeof(DAT_HDR))
			{
				printf("invalid len(%d) != DAT_HDR(%d)\r\n",
					len, (int) sizeof(DAT_HDR));
				return false;
			}

			// ȡ�������峤�ȣ�����ָ�����ȵ�������

			DAT_HDR* req_hdr = (DAT_HDR*) data;

			// �������ֽ���תΪ�����ֽ���
			req_hdr->len = ntohl(req_hdr->len);
			if (req_hdr->len <= 0)
			{
				printf("invalid len: %d\r\n", req_hdr->len);
				return false;
			}

			// �޸�״̬λ��������һ����Ҫ��ȡ������
			status_ = STATUS_T_DAT;

			// �첽��ָ�����ȵ�����
			client_->read(req_hdr->len, __timeout);
			return true;
		}

		if (status_ != STATUS_T_DAT)
		{
			printf("invalid status: %d\r\n", (int) status_);
			return false;
		}

		if (i_++ < 10)
			printf("req len: %d, dat: %s\r\n", len, data);

		// ��Զ�̿ͻ��˻�д�յ�������

#define	OK	"+OK"
		size_t dat_len = sizeof(OK) - 1;

		DAT_HDR res_hdr;

		// �������ֽ���תΪ�����ֽ���
		res_hdr.len = htonl(dat_len);
		ACL_SAFE_STRNCPY(res_hdr.cmd, "ok", sizeof(res_hdr.cmd));

		// �첽д��Ӧ���ݰ�: ����ͷ��������

		client_->write(&res_hdr, sizeof(res_hdr));
		client_->write(OK, dat_len);

		// ����״̬Ϊ��ȡ��һ�����ݰ�
		status_ = STATUS_T_HDR;

		// ���첽�������ݰ�ͷ
		client_->read(sizeof(DAT_HDR), __timeout);

		return true;
	}

	/**
	 * ʵ�ָ����е��麯�����ͻ�������д�ɹ��ص�����
	 * @return {bool} ���� true ��ʾ����������ϣ���رո��첽��
	 */
	bool write_callback()
	{
		return true;
	}

	/**
	 * ʵ�ָ����е��麯�����ͻ������ĳ�ʱ�ص�����
	 */
	void close_callback()
	{
		// �����ڴ˴�ɾ���ö�̬����Ļص�������Է�ֹ�ڴ�й¶
		delete this;
	}

	/**
	 * ʵ�ָ����е��麯�����ͻ������ĳ�ʱ�ص�����
	 * @return {bool} ���� true ��ʾ����������ϣ���رո��첽��
	 */
	bool timeout_callback()
	{
		std::cout << "Timeout, delete it ..." << std::endl;
		return false;
	}

private:
	status_t status_;
	acl::aio_socket_stream* client_;
	int   i_;
};

/**
 * �첽�������Ļص��������
 */
class io_accept_callback : public acl::aio_accept_callback
{
public:
	io_accept_callback() {}
	~io_accept_callback()
	{
		printf(">>io_accept_callback over!\n");
	}

	/**
	 * �����麯�������������ӵ������ô˻ص�����
	 * @param client {aio_socket_stream*} �첽�ͻ�����
	 * @return {bool} ���� true ��֪ͨ��������������
	 */
	bool accept_callback(acl::aio_socket_stream* client)
	{
		// �����첽�ͻ������Ļص���������첽�����а�
		io_callback* callback = new io_callback(client);

		// ע���첽���Ķ��ص�����
		client->add_read_callback(callback);

		// ע���첽����д�ص�����
		client->add_write_callback(callback);

		// ע���첽���Ĺرջص�����
		client->add_close_callback(callback);

		// ע���첽���ĳ�ʱ�ص�����
		client->add_timeout_callback(callback);

		// ���첽�������ݰ�ͷ
		client->read(sizeof(DAT_HDR), __timeout);
		return (true);
	}
};

static void usage(const char* procname)
{
	printf("usage: %s -h[help]\r\n"
		"	-l ip:port[:1900]\r\n"
		"	-t timeout\r\n"
		"	-k[use kernel event: epoll/iocp/kqueue/devpool]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	bool use_kernel = false;
	int  ch;
	acl::string addr(":1900");

	while ((ch = getopt(argc, argv, "l:hkt:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return (0);
		case 'l':
			addr = optarg;
			break;
		case 'k':
			use_kernel = true;
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	// �����첽���������
	acl::aio_handle handle(use_kernel ? acl::ENGINE_KERNEL : acl::ENGINE_SELECT);

	// ���������첽��
	acl::aio_listen_stream* sstream = new acl::aio_listen_stream(&handle);

	// ��ʼ��ACL��(��������WIN32��һ��Ҫ���ô˺�������UNIXƽ̨�¿ɲ�����)
	acl::acl_cpp_init();

	// ����ָ���ĵ�ַ
	if (sstream->open(addr.c_str()) == false)
	{
		std::cout << "open " << addr.c_str() << " error!" << std::endl;
		sstream->close();
		// XXX: Ϊ�˱�֤�ܹرռ�������Ӧ�ڴ˴��� check һ��
		handle.check();

		getchar();
		return (1);
	}

	// �����ص�����󣬵��������ӵ���ʱ�Զ����ô������Ļص�����
	io_accept_callback callback;
	sstream->add_accept_callback(&callback);
	std::cout << "Listen: " << addr.c_str() << " ok!" << std::endl;

	while (true)
	{
		// ������� false ���ʾ���ټ�������Ҫ�˳�
		if (handle.check() == false)
		{
			std::cout << "pkg_server stop now ..." << std::endl;
			break;
		}
	}

	// �رռ��������ͷ�������
	sstream->close();

	// XXX: Ϊ�˱�֤�ܹرռ�������Ӧ�ڴ˴��� check һ��
	handle.check();

	return (0);
}
