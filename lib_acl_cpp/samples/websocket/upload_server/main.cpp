#include "stdafx.h"
#include "master_service.h"

int main(int argc, char* argv[])
{
	// ��ʼ�� acl ��
	acl::acl_cpp_init();

	master_service& ms = acl::singleton2<master_service>::get_instance();

	// �������ò�����
	ms.set_cfg_int(var_conf_int_tab);
	ms.set_cfg_int64(var_conf_int64_tab);
	ms.set_cfg_str(var_conf_str_tab);
	ms.set_cfg_bool(var_conf_bool_tab);

	// ��ʼ����

	if (argc >= 2 && strcasecmp(argv[1], "help") == 0)
	{
		printf("usage: %s alone [configure [listen_addr]]\r\n", argv[0]);
		return 0;
	}
	else if (argc >= 2 && strcasecmp(argv[1], "alone") == 0)
	{
		acl::log::stdout_open(true);  // ��־�������׼���

		const char* conf = NULL;
		if (argc >= 3)
			conf = argv[2];

		const char* addr = "0.0.0.0:9004";
		if (argc >= 4)
			addr = argv[3];

		printf("listen on: %s\r\n", addr);
		ms.run_alone(addr, conf, 0, 1000);  // �������з�ʽ

		printf("Enter any key to exit now\r\n");
		getchar();
	}
	else
		ms.run_daemon(argc, argv);  // acl_master ����ģʽ����

	return 0;
}
