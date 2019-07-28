#pragma once
class CFiberSleep : public acl::fiber
{
public:
	CFiberSleep(void);

private:
	// @override
	void run(void);
	~CFiberSleep(void);
};

