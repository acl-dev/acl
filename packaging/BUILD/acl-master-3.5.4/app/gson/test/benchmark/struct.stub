#pragma once

struct user
{
	user(const char* username, const char* domain, int age, bool male)
	: username_(username)
	, domain_(domain)
	, age_(age)
	, male_(male)
	{}

	user() {}
	~user() {}

	acl::string username_;
	acl::string domain_;
	int age_;
	bool male_;
};

struct message
{
	int type_;
	acl::string cmd_;
	std::list<user> data_;
};

struct user1
{
	user1(const char* username, const char* domain, int age, bool male)
	{
		size_t n = strlen(username);
		username_ = new char[n + 1];
		memcpy(username_, username, n);
		username_[n] = 0;

		n = strlen(domain);
		domain_ = new char[n + 1];
		memcpy(domain_, domain, n);
		domain_[n] = 0;
		age_ = age;
		male_ = male;
	}

	user1()
	{
		username_ = NULL;
		domain_ = NULL;
	}

	~user1()
	{
		delete []username_;
		delete []domain_;
	}

	char* username_;
	char* domain_;
	int age_;
	bool male_;
};

struct message1
{
	message1()
	{
	}

	~message1()
	{
		for (auto it = data_.begin(); it != data_.end(); ++it)
			delete *it;
	}

	int type_;
	acl::string cmd_;
	std::list<user1*> data_;
};
