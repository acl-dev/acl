#pragma once

class CFiberHttpd : public acl::fiber
{
public:
	CFiberHttpd(const char* addr);

protected:
	// @override
	void run(void);

private:
	acl::string addr_;

	~CFiberHttpd(void);
};

