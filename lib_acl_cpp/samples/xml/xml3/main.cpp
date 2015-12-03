#include "acl_cpp/lib_acl.hpp"

static const char* data = "{'server': "
  "["
  " {'conns': '10', 'used': '0', 'pid': '47841', 'max_threads': '10',"
  "  'curr_threads': '1', 'busy_threads': '0', 'qlen': '0', 'type': 'default'"
  " },"
  " {'conns': '10', 'used': '0', 'pid': '49628', 'max_threads': '10', "
  "  'curr_threads': '1', 'busy_threads': '0', 'qlen': '0', 'type': 'default'"
  " }"
  "],"
  "'conns': '20', 'used': '0', 'qlen': '0', 'max_threads': '20', "
  "'curr_threads': '2', 'busy_threads': '0', 'addr': '192.168.111.158:1080', "
  "'load': '0.02'}";

int main(void)
{
	acl::json json;
	json.update(data);

	// 将 JSON 数据转为 XML 数据的过程

	acl::xml1 xml;
	acl::xml_node& server_node = xml.create_node("server");
	xml.get_root().add_child(server_node);

	acl::string key;
	acl::json_node& root = json.get_root();
	acl::json_node* child = root.first_child();

	while (child != NULL)
	{
		const char* name = child->tag_name();
		if (name == NULL || *name == 0)
		{
			child = root.next_child();
			continue;
		}

		if (strcasecmp(name, "server") != 0)
		{
			const char* value = child->get_text();
			if (value != NULL)
			{
				if (strcasecmp(name, "addr") == 0)
					key = value;
				server_node.add_attr(name, value);
			}

			child = root.next_child();
			continue;
		}

		acl::json_node* server = child->get_obj();
		if (server == NULL)
		{
			child = root.next_child();
			continue;
		}


		acl::json_node* proc = server->first_child();

		while (proc != NULL)
		{
			// 创建  proc XML 结点
			acl::xml_node& proc_node = xml.create_node("proc");
			server_node.add_child(proc_node);

			acl::json_node* status = proc->first_child();
			while (status != NULL)
			{
				const char* tag = status->tag_name();
				const char* val = status->get_text();
				if (tag && val)
					proc_node.add_child(tag, false, val);
				status = proc->next_child();
			}
			proc = server->next_child();
		}

		child = root.next_child();
	}

	acl::string buf;
	xml.build_xml(buf);
	printf(">>>>>>xml: %s\n", buf.c_str());
	printf("\r\n%s\r\n", data);

	return 0;
}
