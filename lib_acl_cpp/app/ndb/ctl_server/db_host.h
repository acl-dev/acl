#pragma once

class db_host
{
public:
	db_host(unsigned int id, const char* addr,
		long long int count);
	virtual ~db_host(void);

	unsigned int get_id() const
	{
		return id_;
	}

	const char* get_addr() const
	{
		return addr_;
	}

	long long int get_count() const
	{
		return count_;
	}
protected:
	unsigned int id_;
	char* addr_;
	long long int count_;
};

class idx_host : public db_host
{
public:
	idx_host(unsigned int id, const char* addr,
		long long int count);
	~idx_host();
protected:
private:
};

class dat_host : public db_host
{
public:
	dat_host(unsigned int id, const char* addr,
		long long int count, int priority);
	~dat_host();

	int get_priority() const
	{
		return priority_;
	}
protected:
	int priority_;
private:
};
