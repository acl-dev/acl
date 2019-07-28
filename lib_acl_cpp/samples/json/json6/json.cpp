#include "stdafx.h"

static void print(acl::json& json)
{
	acl::json_node* node = json.first_node();
	const char* ptr;

	while (node)
	{
		ptr = node->to_string().c_str();
		printf(">>>node: %s\r\n", ptr);

		node = json.next_node();
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -o [parse once, default: no]\r\n", procname);
}

int main(int argc, char* argv[])
{
	bool  once = false;
	int   ch;

	while((ch = getopt(argc, argv, "ho")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'o':
			once = true;
			break;
		default:
			break;
		}
	}

	const char* datas[] = {
		"{\"data\":[{\"action\":\"set\",\"updatetime\":\"20130928150025\"},{\"action\":\"get\",\"updatetime\":\"20100928150025\"}]}",
		"{\"data\":[{\"action\":\"set\"},{\"action\":\"get\"},{\"action\":\"del\"}],\"value\":\"result\",\"value2\":{\"value21\":\"value211\"}}",
		"{\"DataValue\":{\"RemoteLoginRemind\":[{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\"CloseDoc()\"},[{\"value\":\"Save\",\"onclick\":\"SaveDoc()\"}]]}}",
		"{\"DataValue\":{\"RemoteLoginRemind\":\"true1\",\"ModifyPasswdRemind\":\"true2\",\"MailForwardRemind\":\"true3\",\"SecureLoginVerification\":\"remote\"}}",
		"{\"data\":[{\"action\":\"set\"}],\"value\":\"result\"}",
		"{\"data\":{\"action\":\"set\"},\"cmd\":{\"value\":\"result\"}}",
		"{\"data\":{},\"value\":\"result\"}",
		"{\"item1\":[\"open\",\"onclick\",\"item2\",\"Help\"]}",
		"{\"item1\":[{\"open\":\"onclick\"},{\"item2\":\"Help\"}]}",
		"{\"data\":\"action\",\"value\":\"result\"}",
		"{\"data\":[{\"action\":\"set\"},{\"action\":\"get\"}]}",
		"{\"value\":\"Close\",\"onclick\":\"CloseDoc()\",\"test\":{\"dddd\":{\"value\":\"Help\",\"onclick\":\"Help()\"},\"help\":\"helloworld!\"}}",
		"{\"value\":\"Close\",\"onclick\":\"CloseDoc()\",{\"dddd\":{\"value\":\"Help\",\"onclick\":\"Help()\"},\"help\":\"helloworld!\"}}",
		"{\"menuitem2\":[{\"value1\":\"Open1\",\"onclick\":\"Open1()\"},{\"value2\":\"Open2\",\"onclick\":\"Open2()\"},[{\"value3\":\"Open3\",\"onclick\":\"Open3()\"}],[\"value4\",\"Open4\",\"onclick\",\"Open4()\"],[{\"value5\":\"Open5\"},\"onclick\",\"Open5()\"],{\"value6\":\"Open6\",\"onclick\":\"Open6()\"}]}",
		"{\"help\":\"hello world!\",\"menuitem2\":[{\"value1\":\"Open1\",\"onclick\":\"Open1()\"},[{\"value3\":\"Open3\",\"onclick\":\"Open3()\"}],{\"value6\":\"Open6\"}]}",
		"{\"menu\":{\"item\":{\"value\":\"open\"}},\"help\":\"hello\"}",
		"{\"menu\":{\"item\":{\"value\":\"open\",\"value\":\"close\",\"value\":\"save\"}},\"help\":\"hello\"}",
		"{\"menu\":[{\"value\":\"open\",\"value\":\"close\"},\"open\",\"close\",\"new\",[\"open\",\"close\",\"new\"],[{\"value\":\"open\",\"value\":\"close\"}]]}",
		"{\"menu\":{\"item\":{\"entry\":{\"value\":\"open\",\"value\":\"close\"},\"entry\":{\"value\":\"new\"}}},\"help\":\"hello\"}",
		"{\"menuname\":{\"id:file\":\"file\",\"value{\":\"File\",\"popup{}\":{\"menuitem1}\":[{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\"CloseDoc()\"}],\"menuname[]\":\"hello world\",\"inner\":{\"value\":\"new\",\"value\":\"open\"},\"menuitem2\":[{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\"CloseDoc()\"},{{\"value\":\"Help\",\"onclick\":\"Help()\"}}]}},\"help\":\"helloworld!\",\"menuitem2\":[{\"value1\":\"Open1\",\"onclick\":\"Open1()\"},{\"value2\":\"Open2\",\"onclick\":\"Open2()\"},[{\"value3\":\"Open3\",\"onclick\":\"Open3()\"}],[{\"value4\":\"Open4\"},\"onclick\",\"Open4()\"],[\"value5\",\"Open5\",\"onclick\",\"Open5()\"],{\"value6\":\"Open6\",\"onclick\":\"Open6()\"}]}",
		NULL
	};

	size_t  nok = 0, i;
	for (i = 0; datas[i] != NULL; i++)
	{
#if 0
		if (i != 16)
			continue;
#endif

		acl::json json;

		if (once)
			json.update(datas[i]);
		else
		{
			const char* ptr = datas[i];
			char  buf[2];
			while (*ptr)
			{
				buf[0] = *ptr;
				buf[1] = 0;
				json.update(buf);
				ptr++;
			}
		}

		printf("====================================================================\r\n");

		printf(">>> src string:\r\n%s\r\n", datas[i]);
		printf(">>> to  string:\r\n%s\r\n", json.to_string().c_str());

		if (json.to_string() != datas[i])
		{
			printf("====================================================================\r\n");
			printf("ERROR, not equal, item: %d\r\n", (int) i);
			print(json);
			exit (1);
		}
		else
		{
			nok++;
			printf("OK\r\n");
			print(json);
		}
	}

	printf("====================================================================\r\n");

	if (nok == i)
		printf("All ok\r\n");
	else
		printf("Some Error(%d, %d)\r\n", (int) nok, (int) i);
#ifdef WIN32
	printf("enter any key to exit!\r\n");
	getchar();
#endif
	return 0;
}
