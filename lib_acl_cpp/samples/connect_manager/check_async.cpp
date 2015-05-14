#include "stdafx.h"
#include "check_async.h"

check_async::check_async(acl::check_client& checker)
: checker_(checker)
{
}

check_async::~check_async(void)
{
}

bool check_async::read_callback(char* data, int len)
{
	// ��Ϊ acl ���첽 IO ���������ݿ϶����������������������� \0��
	// ����ֱ�ӵ��ַ����Ƚ��ڴ˴��ǰ�ȫ��

	if (strncasecmp(data, "+OK", 3) == 0)
	{
		// ���� QUIT ����
		checker_.get_conn().format("QUIT\r\n");

		// �������������Ϊ���״̬
		checker_.set_alive(true);

		// �����رոü������
		checker_.close();

		// �˴����� true �� false �����ԣ���Ϊ�����Ѿ�����Ҫ��رռ������
		printf(">>> NIO_CHECK SERVER(%s) OK: %s, len: %d <<<\r\n",
			checker_.get_addr(), data, len);
		return true;
	}

	// ���� QUIT ����
	checker_.get_conn().format("QUIT\r\n");

	// ���������Ϊ������״̬
	checker_.set_alive(false);

	printf(">>> NIO_CHECK SERVER(%s) ERROR: %s, len: %d <<<\r\n",
		checker_.get_addr(), data, len);

	// ���� false ֪ͨ����Զ��رո�����
	return false;
}

bool check_async::timeout_callback()
{
	// ����ʱ������ֱ�ӽ�������Ϊ������
	checker_.set_alive(false);

	// ���� false ͨ������Զ��رոü������
	return false;
}

void check_async::close_callback()
{
	// ��̬����������Ҫ��̬ɾ��
	delete this;
}
