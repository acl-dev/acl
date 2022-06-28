#pragma once

class CFiberHttpc : public acl::fiber {
public:
	CFiberHttpc(acl::socket_stream* conn);

protected:
	// @override
	void run(void);

private:
	acl::socket_stream* conn_;

	~CFiberHttpc(void);
};

