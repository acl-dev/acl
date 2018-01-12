#pragma once
class CFiberConnect : public acl::fiber
{
public:
	CFiberConnect(UINT count);

private:
	~CFiberConnect(void);

	// @override
	void run(void);

private:
	UINT m_count;
	socket_t m_sock;
};

