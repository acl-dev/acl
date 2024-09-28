#pragma once

class CFiberConnect
{
public:
	CFiberConnect(const char* ip, int port, int count);
	~CFiberConnect(void);

	bool Start(void);

private:
	std::string ip_;
	int port_;
	int count_;
};

