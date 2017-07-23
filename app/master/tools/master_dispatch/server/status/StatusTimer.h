#pragma once

/**                                                                            
 * 后端服务器连接状态定时器，该定时器会定期汇总后端服务的连接等状态，同时将    
 * 这些状态统一发给状态汇总服务器                                              
 */
class StatusTimer : public acl::event_timer
{
public:
	StatusTimer();

	/**
	 * 销毁动态对象
	 */
	virtual void destroy();

protected:
	// 基类虚函数
	virtual void timer_callback(unsigned int id);

private:
	~StatusTimer();
};
