#pragma once
class CFiberSleep : public acl::fiber
{
public:
	CFiberSleep(void);
	~CFiberSleep(void);

private:
	void run(void);
};

