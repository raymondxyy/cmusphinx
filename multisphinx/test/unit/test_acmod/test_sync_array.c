#include <stdio.h>
#include <time.h>

#include <sphinxbase/sync_array.h>
#include <sphinxbase/sbthread.h>
#include <sphinxbase/garray.h>
#include <sphinxbase/err.h>
#include <pocketsphinx.h>

#include "test_macros.h"

static int
consumer(sbthread_t *th)
{
	sync_array_t *sa = sbthread_arg(th);
	static int sleep_count = 1;
	int i;

	printf("Thread %p sleeping %d secs\n",
	       th, sleep_count);
	sleep(sleep_count++);
	printf("Thread %p woke up\n", th);

	for (i = 0; i < 20; ++i) {
		if (sync_array_wait(sa, i, -1, -1) == 0) {
			int ent;
			sync_array_get(sa, i, &ent);
			printf("Thread %p got element %d = %d\n",
			       th, i, ent);
			sync_array_release(sa, i, i + 1);
		}
	}
	sync_array_free(sa);
	return 0;
}

int
main(int argc, char *argv[])
{
	sync_array_t *sa;
	sbthread_t *threads[10];
	int i;

	sa = sync_array_init(0, sizeof(int));
	for (i = 0; i < 10; ++i) {
		struct timespec foo;
		threads[i] = sbthread_start(NULL, consumer,
					    sync_array_retain(sa));
		foo.tv_sec = 0;
		foo.tv_nsec = 50000;
		nanosleep(&foo, NULL);
	}

	/* Now just stream a bunch of numbers in there with some
	 * sleeping in between. */
	for (i = 0; i < 20; ++i) {
		printf("Producer appending %d\n", i);
		sync_array_append(sa, &i);
		sleep(1);
	}
	sync_array_finalize(sa);
	for (i = 0; i < 10; ++i) {
		sbthread_wait(threads[i]);
		sbthread_free(threads[i]);
	}
	sync_array_free(sa);
	return 0;
}