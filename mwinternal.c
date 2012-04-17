/*
 * mwinternal.c - Mazewar's internal helper function
 */

#include "mwinternal.h"

/* Source: www.gnu.org/softwar/libc/manual/html_node/Elapsed-Time.html */
void
mw_timeval_difference(struct timeval *diff,
                      const struct timeval *__x,
                      const struct timeval *__y)
{
	struct timeval x;
	struct timeval y;

	memcpy(&x, __x, sizeof(struct timeval));
	memcpy(&y, __y, sizeof(struct timeval));

	/* Perform the carry for the later subtraction by updating y. */
	if (x.tv_usec < y.tv_usec) {
		int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
		y.tv_usec -= 1000000 * nsec;
		y.tv_sec += nsec;
	}

	if (x.tv_usec - y.tv_usec > 1000000) {
		int nsec = (x.tv_usec - y.tv_usec) / 1000000;
		y.tv_usec += 1000000 * nsec;
		y.tv_sec -= nsec;
	}

	/* Compute the difference. tv_usec is certainly positive. */
	diff->tv_sec  = x.tv_sec  - y.tv_sec;
	diff->tv_usec = x.tv_usec - y.tv_usec;
}

void
mw_timeval_sum(struct timeval *sum,
               const struct timeval *__x,
               const struct timeval *__y)
{
	struct timeval x;
	struct timeval y;

	memcpy(&x, __x, sizeof(struct timeval));
	memcpy(&y, __y, sizeof(struct timeval));

	sum->tv_sec  = x.tv_sec  + y.tv_sec;
	sum->tv_usec = x.tv_usec + y.tv_usec;

	if (sum->tv_usec >= 1000000) {
		sum->tv_sec++;
		sum->tv_usec -= 1000000;
	}
}

int
mw_timeval_timeout_triggered(const struct timeval *timeout)
{
	return ((timeout->tv_sec < 0) || (timeout->tv_sec  == 0 &&
	                                  timeout->tv_usec <= 0));
}

uint64_t
mw_rand(void)
{
	static int initialized = 0;
	uint64_t rc = 0;
	int i;

	if (!initialized)
		srand(time(0));

	for (i = 0; i < 2; i++)
		rc = (rc << (32 * i)) | (uint32_t) rand();

	return rc;
}
