#include "StdAfx.h"
#include "rpc/rpc_manager.h"
#include "ping/ping.h"
#include "dns/nslookup.h"
#include "mail/smtp_client.h"
#include "mail/pop3_client.h"
#include "test_all.h"

//////////////////////////////////////////////////////////////////////////

void ping_result::ping_report(size_t total, size_t curr, size_t nerr)
{
	test_.ping_report(total, curr, nerr);
}

void ping_result::ping_finish(const char* dbpath)
{
	test_.ping_finish(dbpath);
}

//////////////////////////////////////////////////////////////////////////

void nslookup_result::nslookup_report(size_t total, size_t curr)
{
	test_.nslookup_report(total, curr);
}

void nslookup_result::nslookup_finish(const char* dbpath)
{
	test_.nslookup_finish(dbpath);
}

//////////////////////////////////////////////////////////////////////////

void smtp_result::smtp_report(const char* msg, size_t total,
	size_t curr, const SMTP_METER& meter)
{
	test_.smtp_report(msg, total, curr, meter);
}

void smtp_result::smtp_finish(const char* dbpath)
{
	test_.smtp_finish(dbpath);
}	

//////////////////////////////////////////////////////////////////////////

void pop3_result::pop3_report(const char* msg, size_t total,
	size_t curr, const POP3_METER& meter)
{
	test_.pop3_report(msg, total, curr, meter);
}

void pop3_result::pop3_finish(const char* dbpath)
{
	test_.pop3_finish(dbpath);
}	

//////////////////////////////////////////////////////////////////////////

test_all::test_all(test_callback* callback)
: callback_(callback)
, ping_result_(*this)
, ns_result_(*this)
, smtp_result_(*this)
, pop3_result_(*this)
, ping_ok_(false)
, dns_ok_(false)
, smtp_ok_(false)
, pop3_ok_(false)
, pop3_recv_all_(false)
, pop3_recv_limit_(1)
, pop3_save_(false)
{

}

test_all::~test_all()
{

}

void test_all::start()
{
	acl::rpc_request* req;

	// 启动 PING 过程
	req = new ping(ip_file_.c_str(), &ping_result_,
		ping_npkt_, ping_delay_, ping_timeout_, ping_size_);
	rpc_manager::get_instance().fork(req);

	// 启动 DNS 查询过程
	req = new nslookup(domain_file_.c_str(), &ns_result_,
		dns_ip_.c_str(), dns_port_, dns_timeout_);
	rpc_manager::get_instance().fork(req);

	// 启动邮件发送过程
	smtp_client* smtp = new smtp_client();
	(*smtp).set_callback(&smtp_result_)
		.add_file(attach_.c_str())
		.set_smtp(smtp_addr_.c_str(), smtp_port_)
		.set_conn_timeout(conn_timeout_)
		.set_rw_timeout(rw_timeout_)
		.set_account(mail_user_.c_str())
		.set_passwd(mail_pass_.c_str())
		.set_from(mail_user_.c_str())
		.set_subject("邮件发送过程测试!")
		.add_to(recipients_.c_str());
	rpc_manager::get_instance().fork(smtp);

	// 启动邮件接收过程
	pop3_client* pop3 = new pop3_client();
	(*pop3).set_callback(&pop3_result_)
		.set_pop3(pop3_addr_.c_str(), pop3_port_)
		.set_conn_timeout(conn_timeout_)
		.set_rw_timeout(rw_timeout_)
		.set_account(mail_user_.c_str())
		.set_passwd(mail_pass_.c_str())
		.set_recv_count(pop3_recv_all_ ? -1 : (int) pop3_recv_limit_)
		.set_recv_save(pop3_save_);
	rpc_manager::get_instance().fork(pop3);
}

void test_all::check_finish()
{
	if (ping_ok_ && dns_ok_ && smtp_ok_ && pop3_ok_)
	{
		callback_->test_finish();
		delete this;
	}
}

void test_all::ping_report(size_t total, size_t curr, size_t nerr)
{
	unsigned nstep;
	if (total > 0)
		nstep = (int) ((curr * 100) / total);
	else
		nstep = 0;

	acl::string msg;
	msg.format("ping 过程 %d/%d; failed: %d", curr, total, nerr);
	callback_->test_report(msg.c_str(), nstep);
}

void test_all::ping_finish(const char* dbpath)
{
	callback_->test_store(dbpath);
	ping_ok_ = true;
	check_finish();
}

void test_all::nslookup_report(size_t total, size_t curr)
{
	unsigned nstep;
	if (total > 0)
		nstep = (unsigned) ((curr * 100) / total);
	else
		nstep;

	acl::string msg;
	msg.format("共 %d 个域名, 完成 %d 个域名", total, curr);
	callback_->test_report(msg.c_str(), nstep);
}

void test_all::nslookup_finish(const char* dbpath)
{
	callback_->test_store(dbpath);
	dns_ok_ = true;
	check_finish();
}

void test_all::smtp_report(const char* msg, size_t total,
	size_t curr, const SMTP_METER& meter)
{
	unsigned nstep;

	if (total > 0)
		nstep = (int) ((curr * 100) / total);
	else
		nstep = 0;
	callback_->test_report(msg, nstep);
}

void test_all::smtp_finish(const char* dbpath)
{
	callback_->test_store(dbpath);
	smtp_ok_ = true;
	check_finish();
}

void test_all::pop3_report(const char* msg, size_t total,
	size_t curr, const POP3_METER& meter)
{
	unsigned nstep;

	if (total > 0)
		nstep = (int) ((curr * 100) / total);
	else
		nstep = 0;
	callback_->test_report(msg, nstep);
}

void test_all::pop3_finish(const char* dbpath)
{
	callback_->test_store(dbpath);
	pop3_ok_ = true;
	check_finish();
}

//////////////////////////////////////////////////////////////////////////
