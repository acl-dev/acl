#include "stdafx.h"
#include "util.h"

//////////////////////////////////////////////////////////////////////////

typedef enum
{
	MSG_STOP,
	MSG_ECHO
} msg_t;

class queue_item : public acl::thread_qitem
{
public:
	queue_item(msg_t type) : type_(type)
	{
	}

	~queue_item()
	{
	}

	msg_t get_type() const
	{
		return type_;
	}

private:
	msg_t type_;
};

class consumer : public acl::thread
{
public:
	consumer(acl::thread_queue& queue) : queue_(queue) {}
	~consumer() {}

protected:
	void* run()
	{
		long long int  n = 0;

		struct timeval begin;
		gettimeofday(&begin, NULL);

		while (true)
		{
			queue_item* item = (queue_item*) queue_.pop();
			if (item == NULL)
				break;

			n++;

			msg_t type = item->get_type();
			delete item;

			if (type == MSG_STOP)
			{
				printf("stop now, max: %lld\r\n", n);
				break;
			}

			switch (type)
			{
			case MSG_ECHO:
				if (n <= 10)
					printf("hello\r\n");
				if (n % 100000 == 0)
				{
					char tmp[256];
					acl::safe_snprintf(tmp, sizeof(tmp),
						"n: %lld", n);
					acl::meter_time(__FILE__, __LINE__, tmp);
				}
				break;
			default:
				printf("unknown msg type\r\n");
				break;
			}
		}

		struct timeval end;
		gettimeofday(&end, NULL);
		double spent = util::stamp_sub(&end, &begin);

		printf("queue item over, max: %lld\r\n", n);
		printf("max: %lld, spent: %.2f, speed: %.2f\r\n",
			n, spent, n * 1000 / (spent > 0 ? spent : 1));

		return NULL;
	}

private:
	acl::thread_queue& queue_;
};

class producer : public acl::thread
{
public:
	producer(acl::thread_queue& queue, int max)
		: queue_(queue), max_(max) {}
	~producer() {}

protected:
	void* run()
	{
		for (int i = 0; i < max_; i++)
		{
			queue_item* item = new queue_item(MSG_ECHO);
			queue_.push(item);
		}
		printf("push msg ok\r\n");

		queue_item* item = new queue_item(MSG_STOP);
		queue_.push(item);
		return NULL;
	}

private:
	acl::thread_queue& queue_;
	int max_;
};

//////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	int max = 10;
	if (argc >= 2)
	{
		max = atoi(argv[1]);
		if (max <= 0)
			max = 10;
	}

	// ³õÊ¼»¯ acl ¿â
	acl::acl_cpp_init();

	acl::thread_queue queue;

	producer producer(queue, max);
	producer.set_detachable(false);
	producer.start();

	consumer consumer(queue);
	consumer.set_detachable(false);
	consumer.start();

	producer.wait();
	consumer.wait();

	printf("enter any key to exit ...\r\n");
	getchar();

	return 0;
}
