#pragma once

class CFiberServer : public acl::fiber
{
public:
	CFiberServer(bool autoDestroy = true);
	~CFiberServer(void);

	bool BindAndListen(int port, const std::string& addr);

protected:
	// @override
	void run(void);

private:
	SOCKET m_sock;
	bool   m_autoDestroy;
};

