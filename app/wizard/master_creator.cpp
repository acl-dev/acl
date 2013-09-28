#include "stdafx.h"
#include "file_copy.h"
#include "master_creator.h"

static const char* src_path_ = "tmpl/master";
static char  dst_path_[256];
static char master_name[128];

//////////////////////////////////////////////////////////////////////////

static tpl_t* open_tpl(const char* filename)
{
	tpl_t* tpl = tpl_alloc();
	string filepath;
	filepath.format("%s/%s", src_path_, filename);
	if (tpl_load(tpl, filepath.c_str()) != TPL_OK)
	{
		printf("load file %s error: %s\r\n",
			filepath.c_str(), last_serror());
		tpl_free(tpl);
		return NULL;
	}
	return tpl;
}

//////////////////////////////////////////////////////////////////////////

static bool copy_and_replace(const char* from, const char* to)
{
	tpl_t* tpl = open_tpl(from);
	if (tpl == NULL)
		return false;

	tpl_set_field_fmt_global(tpl, "PROGRAM", "%s", master_name);

	string filepath;
	filepath << dst_path_ << '/' << to;
	if (tpl_save_as(tpl, filepath.c_str()) != TPL_OK)
	{
		printf("save to %s error: %s\r\n", filepath.c_str(),
			last_serror());
		tpl_free(tpl);
		return false;
	}
	printf("create %s ok.\r\n", filepath.c_str());
	tpl_free(tpl);
	return true;
}

static bool create_common()
{
	if (copy_and_replace("Makefile", "Makefile") == false)
		return false;
	string file;

	// for vc2003
	file.format("%s.sln", master_name);
	if (copy_and_replace("master_service.sln", file.c_str()) == false)
		return false;
	file.format("%s.vcproj", master_name);
	if (copy_and_replace("master_service.vcproj", file.c_str()) == false)
		return false;

	// for vc2012
	file.format("%s_vc2012.sln", master_name);
	if (copy_and_replace("master_service_vc2012.sln", file.c_str()) == false)
		return false;
	file.format("%s_vc2012.vcxproj", master_name);
	if (copy_and_replace("master_service_vc2012.vcxproj", file.c_str()) == false)
		return false;
	file.format("%s_vc2012.vcxproj.filters", master_name);
	if (copy_and_replace("master_service_vc2012.vcxproj.filters", file.c_str()) == false)
		return false;

	const char* name = "common_files";
	const FILE_FROM_TO tab[] = {
		{ "stdafx.h", "stdafx.h" },
		{ "stdafx.cpp", "stdafx.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab, src_path_, dst_path_);
}

//////////////////////////////////////////////////////////////////////////

static bool create_master_threads()
{
	create_common();

	string file(master_name);
	file << ".cf";
	if (copy_and_replace("master_threads.cf", file.c_str()) == false)
		return false;

	const char* name = "master_threads";
	const FILE_FROM_TO tab[] = {
		{ "main_threads.cpp", "main.cpp" },
		{ "master_threads.h", "master_service.h" },
		{ "master_threads.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab, src_path_, dst_path_);
}

static bool create_master_proc()
{
	create_common();

	string file(master_name);
	file << ".cf";
	if (copy_and_replace("master_proc.cf", file.c_str()) == false)
		return false;

	const char* name = "master_proc";
	const FILE_FROM_TO tab[] = {
		{ "main_proc.cpp", "main.cpp" },
		{ "master_proc.h", "master_service.h" },
		{ "master_proc.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab, src_path_, dst_path_);
}

static bool create_master_aio()
{
	create_common();

	string file(master_name);
	file << ".cf";
	if (copy_and_replace("master_aio.cf", file.c_str()) == false)
		return false;

	const char* name = "master_aio";
	const FILE_FROM_TO tab[] = {
		{ "main_aio.cpp", "main.cpp" },
		{ "master_aio.h", "master_service.h" },
		{ "master_aio.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab, src_path_, dst_path_);
}

static bool create_master_rpc()
{
	create_common();

	string file(master_name);
	file << ".cf";
	if (copy_and_replace("master_aio.cf", file.c_str()) == false)
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

	return files_copy(name, tab, src_path_, dst_path_);
}

static bool create_master_trigger()
{
	create_common();

	string file(master_name);
	file << ".cf";
	if (copy_and_replace("master_trigger.cf", file.c_str()) == false)
		return false;

	const char* name = "master_trigger";
	const FILE_FROM_TO tab[] = {
		{ "main_trigger.cpp", "main.cpp" },
		{ "master_trigger.h", "master_service.h" },
		{ "master_trigger.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab, src_path_, dst_path_);
}

static bool create_master_udp()
{
	create_common();

	string file(master_name);
	file << ".cf";
	if (copy_and_replace("master_udp.cf", file.c_str()) == false)
		return false;

	const char* name = "master_udp";
	const FILE_FROM_TO tab[] = {
		{ "main_udp.cpp", "main.cpp" },
		{ "master_udp.h", "master_service.h" },
		{ "master_udp.cpp", "master_service.cpp" },
		{ NULL, NULL }
	};

	return files_copy(name, tab, src_path_, dst_path_);
}

void master_creator()
{
	char buf[256];
	int  n;

	while (true)
	{
		printf("please input your program name: "); fflush(stdout);
		n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		if (n == 0)
			snprintf(master_name, sizeof(master_name), "master_service");
		else
			snprintf(master_name, sizeof(master_name), "%s", buf);

		// ´´½¨Ä¿Â¼
		snprintf(dst_path_, sizeof(dst_path_), "%s", master_name);
		acl_make_dirs(dst_path_, 0755);

		printf("choose master_service type:\r\n");
		printf("t: for master_threads; p: for master_proc; "
			"a: for master_aio; g: for master_trigger; "
			"r: for master_rpc; u: for master_udp; "
			"s: skip choose\r\n");
		printf(">"); fflush(stdout);

		n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		else if (strcasecmp(buf, "t") == 0)
		{
			create_master_threads();
			break;
		}
		else if (strcasecmp(buf, "p") == 0)
		{
			create_master_proc();
			break;
		}
		else if (strcasecmp(buf, "a") == 0)
		{
			create_master_aio();
			break;
		}
		else if (strcasecmp(buf, "r") == 0)
		{
			create_master_rpc();
			break;
		}
		else if (strcasecmp(buf, "g") == 0)
		{
			create_master_trigger();
			break;
		}
		else if (strcasecmp(buf, "u") == 0)
		{
			create_master_udp();
			break;
		}
		else if (strcasecmp(buf, "s") == 0)
			break;
		else
			printf("unknown ch: %s\r\n", buf);
	}
	for (int i = 0; i < 78; i++)
		putchar('-');
	printf("\r\n");
}
