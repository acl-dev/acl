#include "stdafx.h"
#include "file_tmpl.h"
#include "master_creator.h"

static bool create_master_threads(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_threads.cf", file.c_str()) == false)
		return false;

	const char* name = "master_threads";
	const FILE_FROM_TO tab[] = {
		{ "main_threads.cpp", "main.cpp" },
		{ "master_threads.h", "master_service.h" },
		{ "master_threads.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return tmpl.files_copy(name, tab);
}

static bool create_master_fiber(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_fiber.cf", file.c_str()) == false)
		return false;

	const char* name = "master_fiber";
	const FILE_FROM_TO tab[] = {
		{ "main_fiber.cpp", "main.cpp" },
		{ "master_fiber.h", "master_service.h" },
		{ "master_fiber.cpp", "master_service.cpp" },
		{ "stdafx_fiber.h", "stdafx.h" },
		{ NULL, NULL }
	};

	return tmpl.files_copy(name, tab)
		&& tmpl.copy_and_replace("Makefile_fiber", "Makefile")
		&& tmpl.file_copy("tmpl/Makefile_fiber.in",
			"Makefile.in");
}

static bool create_master_proc(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_proc.cf", file.c_str()) == false)
		return false;

	const char* name = "master_proc";
	const FILE_FROM_TO tab[] = {
		{ "main_proc.cpp", "main.cpp" },
		{ "master_proc.h", "master_service.h" },
		{ "master_proc.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return tmpl.files_copy(name, tab);
}

static bool create_master_aio(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_aio.cf", file.c_str()) == false)
		return false;

	const char* name = "master_aio";
	const FILE_FROM_TO tab[] = {
		{ "main_aio.cpp", "main.cpp" },
		{ "master_aio.h", "master_service.h" },
		{ "master_aio.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return tmpl.files_copy(name, tab);
}

static bool create_master_rpc(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_aio.cf", file.c_str()) == false)
		return false;

	const char* name = "master_rpc";
	const FILE_FROM_TO tab[] = {
		{ "main_aio.cpp", "main.cpp" },
		{ "master_rpc.h", "master_service.h" },
		{ "master_rpc.cpp", "master_service.cpp" },
		{ "rpc_manager.cpp", "rpc_manager.cpp" },
		{ "rpc_manager.h", "rpc_manager.h" },
		{ NULL, NULL }
	};

	return tmpl.files_copy(name, tab);
}

static bool create_master_trigger(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_trigger.cf", file.c_str()) == false)
		return false;

	const char* name = "master_trigger";
	const FILE_FROM_TO tab[] = {
		{ "main_trigger.cpp", "main.cpp" },
		{ "master_trigger.h", "master_service.h" },
		{ "master_trigger.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return tmpl.files_copy(name, tab);
}

static bool create_master_udp(file_tmpl& tmpl)
{
	string file(tmpl.get_project_name());
	file << ".cf";
	if (tmpl.copy_and_replace("master_udp.cf", file.c_str()) == false)
		return false;

	const char* name = "master_udp";
	const FILE_FROM_TO tab[] = {
		{ "main_udp.cpp", "main.cpp" },
		{ "master_udp.h", "master_service.h" },
		{ "master_udp.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return tmpl. files_copy(name, tab);
}

void master_creator()
{
	file_tmpl tmpl;

	// 设置源程序所在目录
	tmpl.set_path_from("tmpl/master");

	while (true)
	{
		char buf[256];
		int  n;

		printf("please input your program name: ");
		fflush(stdout);

		n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		if (n == 0)
			acl::safe_snprintf(buf, sizeof(buf), "master_service");

		tmpl.set_project_name(buf);
		// 创建目录
		tmpl.create_dirs();

		printf("choose master_service type:\r\n");
		printf("	t: for master_threads\r\n"
			"	p: for master_proc\r\n"
			"	a: for master_aio\t\n"
			"	g: for master_trigger\r\n"
			"	r: for master_rpc\r\n"
			"	u: for master_udp\r\n"
			"	f: for master_fiber\r\n"
			"	s: skip choose, try again\r\n");
		printf(">");
		fflush(stdout);

		n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		else if (strcasecmp(buf, "t") == 0)
		{
			tmpl.create_common();
			create_master_threads(tmpl);
			break;
		}
		else if (strcasecmp(buf, "p") == 0)
		{
			tmpl.create_common();
			create_master_proc(tmpl);
			break;
		}
		else if (strcasecmp(buf, "a") == 0)
		{
			tmpl.create_common();
			create_master_aio(tmpl);
			break;
		}
		else if (strcasecmp(buf, "r") == 0)
		{
			tmpl.create_common();
			create_master_rpc(tmpl);
			break;
		}
		else if (strcasecmp(buf, "g") == 0)
		{
			tmpl.create_common();
			create_master_trigger(tmpl);
			break;
		}
		else if (strcasecmp(buf, "u") == 0)
		{
			tmpl.create_common();
			create_master_udp(tmpl);
			break;
		}
		else if (strcasecmp(buf, "f") == 0)
		{
			tmpl.create_common();
			create_master_fiber(tmpl);
			break;
		}
		else if (strcasecmp(buf, "s") == 0)
			goto END;
		else
			printf("unknown ch: %s\r\n", buf);
	}

END:
	for (int i = 0; i < 78; i++)
		putchar('-');
	printf("\r\n");
}
