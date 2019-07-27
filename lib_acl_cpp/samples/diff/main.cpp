#include "stdafx.h"
#include <getopt.h>
#include "mail_object.h"

struct DIFF_RES
{
	int equal;
	int added;
	int deleted;
	int updated;
};

static void print_objs(const std::vector<acl::diff_object*>& objs,
	const char* prompt, int max)
{
	int i = 0;
	for (std::vector<acl::diff_object*>::const_iterator cit = objs.begin();
		cit != objs.end() && i++ < max; ++cit)
	{
		printf(">>%s->%d: key: %s, val: %s\r\n",
			prompt, i, (*cit)->get_key(), (*cit)->get_val());
	}
}

static void print_objs(
	const std::vector<std::pair<acl::diff_object*, acl::diff_object*> >& objs,
	const char* prompt, int max)
{
	int i = 0;
	for (std::vector<std::pair<acl::diff_object*, acl::diff_object*> >
		::const_iterator cit = objs.begin();
		cit != objs.end() && i++ < max; ++cit)
	{
		printf(">>%s->%d: key: %s, new val: %s, old val: %s\r\n",
			prompt, i, (*cit).first->get_key(),
			(*cit).first->get_val(),
			(*cit).second->get_val());
	}
}

static int load_objs(acl::diff_manager& manager, const char* filepath,
	std::vector<acl::diff_object*>& new_objs)
{
	acl::ifstream in;

	// 浠ュ彧璇绘柟寮忔墦寮€鏁版嵁鏂囦欢
	if (in.open_read(filepath) == false)
	{
		printf("open %s error %s\r\n", filepath, acl::last_serror());
		return -1;
	}

	// 浠庢瘮杈冨櫒涓幏寰楀唴瀛樻睜瀵硅薄锛屼互渚夸簬涓嬮潰鍒嗛厤鍐呭瓨
	acl::dbuf_guard& dbuf = manager.get_dbuf();
	int linenum = 0;
	acl::string line;

	int nobjs = 0;

	while (!in.eof())
	{
		line.clear();

		// 浠庢枃浠朵腑璇诲彇涓€琛屾暟鎹�
		if (in.gets(line) == false)
			break;

		linenum++;

		// 鍒嗘瀽璇ヨ鏁版嵁锛屾彁鍙� key 鍜� value 鍊�
		char* key = line.c_str();
		char* val = strrchr(key, '|');

		// 妫€鏌ユ暟鎹殑鏈夋晥鎬�
		if (val == NULL	|| val == key || *(val + 1) == 0)
		{
			printf("invalid line: %s in %s, linenum: %d\r\n",
				key, filepath, linenum);
			continue;
		}
		*val++ = 0;

		// 鍒涘缓 diff 瀵硅薄锛屽苟缃叆瀵硅薄闆嗗悎涓�
		mail_object* obj = new (dbuf.dbuf_alloc(sizeof(mail_object)))
				mail_object(manager, key, val);
		new_objs.push_back(obj);
		nobjs++;
	}

	return nobjs;
}

static bool check_diff(const char* new_file, const char* old_file,
	const DIFF_RES* res = NULL)
{
	// 棣栧厛瀹氫箟宸泦姣旇緝鍣�
	acl::diff_manager manager;
	// 瀛樻斁褰撳墠瀵硅薄
	std::vector<acl::diff_object*> cur_objs;
	// 瀛樻斁鏃х殑瀵硅薄
	std::vector<acl::diff_object*> old_objs;

	// 鍏堜粠鏂囦欢涓鍙栧綋鍓嶇殑瀵硅薄鑷虫柊瀵硅薄闆嗗悎涓�
	int n = load_objs(manager, new_file, cur_objs);
	if (n < 0)
	{
		printf("load curr objs error from %s\r\n", new_file);
		return false;
	}
	printf("cur objs: %d\r\n", n);

	// 鍐嶄粠鏂囦欢涓鍙栨棫鐨勫璞¤嚦鏃у璞￠泦鍚堜腑
	n = load_objs(manager, old_file, old_objs);
	if (n < 0)
	{
		printf("load old objs error from %s\r\n", old_file);
		return false;
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	// 寮€濮嬭繘琛屽樊闆嗘瘮杈冿紝骞惰幏寰楁渶缁堟柊澧炲璞′汉涓暟
	manager.diff_changes(cur_objs, old_objs);

	struct timeval end;
	gettimeofday(&end, NULL);

	double spent = acl::stamp_sub(end, begin);
	printf("spent: %.4f ms\r\n", spent);

	printf("-------------------------------------------------------\r\n");

	// 鑾峰緱鏂板瀵硅薄闆嗗悎
	const std::vector<acl::diff_object*>& new_objs = manager.get_new();

	// 鎵撳嵃鏂板瀵硅薄鑷冲睆骞曪紝闄愬畾鏈€澶ц緭鍑� 10 涓�
	print_objs(new_objs, "new objs", 10);
	printf("new nobjs: %d\r\n", (int) new_objs.size());

	printf("-------------------------------------------------------\r\n");

	// 鑾峰緱琚垹闄ょ殑瀵硅薄闆嗗悎
	const std::vector<acl::diff_object*>& del_objs = manager.get_deleted();

	// 鎵撳嵃鍒犻櫎鐨勫璞¤嚦灞忓箷锛岄檺瀹氭渶澶ц緭鍑� 10 涓�
	print_objs(del_objs, "deleted objs", 10);
	printf("deleted objs: %d\r\n", (int) del_objs.size());

	printf("-------------------------------------------------------\r\n");

	// 鑾峰緱鍙樺寲鐨勫璞￠泦鍚�
	const std::vector<std::pair<acl::diff_object*, acl::diff_object*> >&
		upd_objs = manager.get_updated();

	// 鎵撳嵃鍙樺寲鐨勫璞¤嚦灞忓箷锛岄檺瀹氭渶澶ц緭鍑� 10 涓�
	print_objs(upd_objs, "updated objs", 10);
	printf("updated objs: %d\r\n", (int) upd_objs.size());

	printf("-------------------------------------------------------\r\n");

	// 鑾峰緱鐩稿悓鐨勫璞￠泦鍚�
	const std::vector<acl::diff_object*>& equ_objs = manager.get_same();
	print_objs(equ_objs, "equal objs", 10);
	printf("equal objs: %d\r\n", (int) equ_objs.size());

	printf("-------------------------------------------------------\r\n");

	// 鏈嚱鏁拌繑鍥炲垯 manager 瀵硅薄鍙婂唴閮ㄨ嚜寤哄唴瀛樻睜鑷姩閿€姣�

	if (res == NULL)
		return true;

	if (res->equal != (int) equ_objs.size())
	{
		printf("invalid equal: %d, %lu\r\n", res->equal, equ_objs.size());
		return false;
	}

	if (res->updated != (int) upd_objs.size())
	{
		printf("invalid updated: %d, %lu\r\n",
			res->updated, equ_objs.size());
		return false;
	}

	if (res->deleted != (int) del_objs.size())
	{
		printf("invalid deleted: %d, %lu\r\n",
			res->deleted, del_objs.size());
		return false;
	}

	if (res->added != (int) new_objs.size())
	{
		printf("invalid added: %d, %lu\r\n",
			res->added, new_objs.size());
		return false;
	}

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n new_file -o old_file\r\n"
		"	-N new_objs_count\r\n"
		"	-U update_objs_count\r\n"
		"	-D delete_objs_count\r\n"
		"	-E equal_objs_count\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int  ch;
	acl::string new_file, old_file;
	DIFF_RES res;

	res.deleted = -1;
	res.updated = -1;
	res.added = -1;
	res.equal = -1;

	while ((ch = getopt(argc, argv, "hn:o:N:U:D:E:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			new_file = optarg;
			break;
		case 'o':
			old_file = optarg;
			break;
		case 'N':
			res.added = atoi(optarg);
			break;
		case 'U':
			res.updated = atoi(optarg);
			break;
		case 'D':
			res.deleted = atoi(optarg);
			break;
		case 'E':
			res.equal = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (new_file.empty() || old_file.empty())
	{
		usage(argv[0]);
		return -1;
	}

	// 濡傛灉璁剧疆浜嗗畬鏁寸殑鏈熸湜缁撴灉闆嗘暟鍊硷紝鍒欎紶鍏ユ牎楠屽樊闆嗙粨鏋�

	if (check_diff(new_file, old_file,
		res.updated >= 0 && res.deleted >= 0
		&& res.added >= 0 && res.equal >= 0 ? &res : NULL) == true)
	{
		printf("check diff ok, new_file: %s, old_file: %s\r\n",
			new_file.c_str(), old_file.c_str());
	}
	else
	{
		printf("check error, new_file: %s, old_file: %s\r\n",
			new_file.c_str(), old_file.c_str());
		return 1;
	}

	return 0;
}
