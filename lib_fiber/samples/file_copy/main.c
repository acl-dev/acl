#include "stdafx.h"
#include "io.h"

struct IO_CTX {
	int   fd;
	off_t off;
	int   len;
	ACL_FIBER_SEM *sem;
};

struct THREAD {
	pthread_t id;
	int from;
	int to;
	off_t  off;
	size_t len;
	int nfibers;
	char action[128];
	int event_type;
};

struct FIBER {
	struct THREAD *thread;
	off_t  off;
	size_t len;
};

static int __debug = 0;

#define	EQ(x, y)	!strcasecmp((x), (y))

static void  fiber_main(ACL_FIBER *fb acl_unused, void *ctx)
{
	struct FIBER *fiber = (struct FIBER*) ctx;
	ssize_t ret ;

	// For multiple threads with multiple coroutines, pread_pwrite and
	// splice is safety, and sendfile can only be used to copy file
	// in one fiber of one single thread!
	// When copying large file, the splice_copy will be more efficietly
	// than pread_pwrite, because splice_copy won't copy data between
	// kernel and userspace, and pread_pwrite will copy data from kernel
	// to userspace and from userspace to kernel.

	if (EQ(fiber->thread->action, "pread_pwrite")) {
		ret = pread_pwrite(fiber->thread->from, fiber->thread->to,
				fiber->off, fiber->len);
	} else if(EQ(fiber->thread->action, "splice")) {
		int pipefd[2];
		if (pipe(pipefd) == -1) {
			printf("create pipe error %s\r\n", strerror(errno));
			return;
		}
		ret = splice_copy(pipefd, fiber->thread->from,
				fiber->thread->to, fiber->off, fiber->len);
		close(pipefd[0]);
		close(pipefd[1]);
	} else if(EQ(fiber->thread->action, "sendfile")) {
		ret = sendfile_copy(fiber->thread->from, fiber->thread->to,
				fiber->off, fiber->len);
	} else {
		printf("Unkown action=%s\r\n", fiber->thread->action);
		return;
	}

	if (ret != (ssize_t) fiber->len) {
		printf("Copy error %s, ret=%zd, len=%zd\r\n",
			strerror(errno), ret, fiber->len);
		assert(0);
	} else if (__debug) {
		printf("Copy ok ret=%zd, len=%zd, off=%ld\r\n",
			ret, fiber->len, fiber->off);
	}
}

static void *thread_main(void *ctx)
{
	struct THREAD *thr = (struct THREAD*) ctx;
	struct FIBER  *fibers;
	size_t step, mod;
	int i;

	assert(thr->len > 0);
	step = thr->len / thr->nfibers;
	mod  = thr->len % thr->nfibers;

	if (step == 0) {
		thr->nfibers = 1;
		step = mod;
		mod  = 0;
	}

	int n = mod == 0 ? thr->nfibers : (thr->nfibers + 1);
	fibers = (struct FIBER*) calloc(sizeof(struct FIBER), n);

	for (i = 0; i < thr->nfibers; i++) {
		fibers[i].thread = thr;
		fibers[i].off = thr->off + step * i;
		fibers[i].len = step;
		acl_fiber_create(fiber_main, &fibers[i], 320000);
	}

	if (mod > 0) {
		fibers[i].thread = thr;
		fibers[i].off = thr->off + step * i;
		fibers[i].len = mod;
		acl_fiber_create(fiber_main, &fibers[i], 320000);
	}

	acl_fiber_schedule_with(thr->event_type);
	free(fibers);
	return NULL;
}

static size_t get_filesize(const char *filepath)
{
	struct stat statbuf;

	if (stat(filepath, &statbuf) == -1) {
		printf("stat %s error %s\r\n", filepath, strerror(errno));
		return 0;
	}

	return statbuf.st_size;
}

static int start(const char *frompath, const char *topath,
	int nthreads, int nfibers, const char *action, int event_type)
{
	struct THREAD *threads;
	int fromfd, tofd, i;
	size_t size, step, mod;

	size = get_filesize(frompath);
	if (size == 0) {
		printf("Invalid filesize=%zd\r\n", size);
		return -1;
	}

	step = size / nthreads;
	mod  = size % nthreads;
	if (step == 0) {
		nthreads = 1;
		step = mod;
		mod  = 0;
	}

	fromfd = open(frompath, O_RDONLY, 0600);
	if (fromfd == -1) {
		printf("open %s error %s\r\n", frompath, strerror(errno));
		return -1;
	}

	//tofd = open(topath, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0600);
	tofd = open(topath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (tofd == -1) {
		printf("open %s error %s\r\n", topath, strerror(errno));
		close(fromfd);
		return -1;
	}

	int n = mod == 0 ? nthreads : (nthreads + 1);
	threads = (struct THREAD*) calloc(sizeof(struct THREAD), n);

	for (i = 0; i < nthreads; i++) {
		threads[i].from = fromfd;
		threads[i].to   = tofd;
		threads[i].off  = step * i;
		threads[i].len  = step;
		threads[i].nfibers = nfibers;
		snprintf(threads[i].action, sizeof(threads[i].action),
			"%s", action);
		threads[i].event_type = event_type;

		int ret = pthread_create(&threads[i].id, NULL,
				thread_main, &threads[i]);
		if (ret != 0) {
			printf("Create thread error=%s\r\n", strerror(ret));
			exit(1);
		}
	}

	if (mod > 0) {
		threads[i].from = fromfd;
		threads[i].to   = tofd;
		threads[i].off  = step * i;
		threads[i].len  = mod;
		threads[i].nfibers = nfibers;
		snprintf(threads[i].action, sizeof(threads[i].action),
			"%s", action);
		threads[i].event_type = event_type;

		int ret = pthread_create(&threads[i].id, NULL,
				thread_main, &threads[i]);
		if (ret != 0) {
			printf("Create thread error=%s\r\n", strerror(ret));
			exit(1);
		}
	}

	for (i = 0; i < n; i++) {
		pthread_join(threads[i].id, NULL);
	}

	free(threads);

	close(fromfd);
	close(tofd);
	return 0;
}

static void usage(const char *proc)
{
	printf("usage: %s -h [help] -d [if show debug info]\r\n"
		"  -e event_type[kernel|select|poll|io_uring]\r\n"
		"  -s from_filepath\r\n"
		"  -d to_filepath\r\n"
		"  -a action[pread_pwrite|sendfile|splice]\r\n"
		"  -t nthreads\r\n"
		"  -c nfibers\r\n"
		, proc);
}

int main(int argc, char *argv[])
{
	int ch, nthreads = 1, nfibers = 1, event_type = FIBER_EVENT_KERNEL;
	char from[256], to[256], action[128];

	snprintf(from, sizeof(from), "from.txt");
	snprintf(to, sizeof(to), "to.txt");
	snprintf(action, sizeof(action), "pread_pwrite");

	while ((ch = getopt(argc, argv, "hDe:s:d:a:t:c:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'D':
			__debug = 1;
			break;
		case 'e':
			if (EQ(optarg, "io_uring")) {
				event_type = FIBER_EVENT_IO_URING;
			} else if (EQ(optarg, "poll")) {
				event_type = FIBER_EVENT_POLL;
			} else if (EQ(optarg, "select")) {
				event_type = FIBER_EVENT_SELECT;
			}
			break;
		case 's':
			snprintf(from, sizeof(from), "%s", optarg);
			break;
		case 'd':
			snprintf(to, sizeof(to), "%s", optarg);
			break;
		case 'a':
			snprintf(action, sizeof(action), "%s", optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (__debug) {
		acl_fiber_msg_stdout_enable(1);
		acl_msg_stdout_enable(1);
	}

	start(from, to, nthreads, nfibers, action, event_type);
	return 0;
}

