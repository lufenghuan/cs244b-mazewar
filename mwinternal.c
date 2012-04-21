/*
 * mwinternal.c - Mazewar's internal helper function
 */

#include "mwinternal.h"

#include <stdio.h>

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

	if (!initialized) {
		srand(time(0));
		initialized = 1;
	}

	for (i = 0; i < 2; i++)
		rc = (rc << (32 * i)) | (uint32_t) rand();

	return rc;
}

#define __PRINT(data, format, descriptor) \
	printf("%15s : " format "\n", descriptor, data);

void
mw_print_pkt_header(const mw_pkt_header_t *pkt)
{
	__PRINT(pkt->mwph_descriptor, "0x%x",  "descriptor");
	__PRINT(pkt->mwph_mbz[0],     "0x%x",  "mbz[0]");
	__PRINT(pkt->mwph_mbz[1],     "0x%x",  "mbz[1]");
	__PRINT(pkt->mwph_mbz[2],     "0x%x",  "mbz[2]");
	__PRINT(pkt->mwph_guid,       "0x%lx", "guid");
	__PRINT(pkt->mwph_seqno,      "0x%lx", "seqno");
}

void
mw_print_pkt_state(const mw_pkt_state_t *pkt)
{
	unsigned int i;
	char buf[16];

	mw_print_pkt_header(&pkt->mwps_header);

	__PRINT(pkt->mwps_rat_posdir,     "0x%x",  "rat_posdir");
	__PRINT(pkt->mwps_missile_posdir, "0x%x",  "missile_posdir");
	__PRINT(pkt->mwps_score,          "0x%x",  "score");
	__PRINT(pkt->mwps_crt,            "0x%lx", "crt");

	for (i = 0; i < sizeof(pkt->mwps_mbz); i++) {
		snprintf(buf, 16, "mbz[%i]", i);
		__PRINT(pkt->mwps_mbz[i], "0x%x", buf);
	}
}

void
mw_print_pkt_nickname(const mw_pkt_nickname_t *pkt)
{
	unsigned int i;
	char buf[16];

	mw_print_pkt_header(&pkt->mwpn_header);

	__PRINT(pkt->mwpn_nickname, "%s", "nickname");

	for (i = 0; i < sizeof(pkt->mwpn_mbz); i++) {
		snprintf(buf, 16, "mbz[%i]", i);
		__PRINT(pkt->mwpn_mbz[i], "0x%x", buf);
	}
}

void
mw_posdir_pack(uint32_t *posdir, mw_pos_t _x, mw_pos_t _y, mw_dir_t _dir)
{
	/* make local copies of position and direction with known sizes.
	 * this avoids confusion as to how many bit's mw_pos_t  and
	 * mw_dir_t structures actually use. this assumes some internal
	 * knowledge of the fact that mw_pos_t and mw_dir_t can be
	 * directly mapped into a uint32_t without any loss of
	 * information.
	 */
	uint32_t x = _x, y = _y, dir = _dir;

	/* according to the mazewar protocol spec, the position and
	 * direction need to be packed into a 32-bit word like so:
	 *
	 *          +------------+------------+-----------+
	 * posdir = | position x | position y | direction |
	 *          +------------+------------+-----------+
	 *          |- 15 bits --|-- 15 bits -|-- 2 bits -|
	 */
	*posdir = ((x   & 0x00007fff) << 17) +
	          ((y   & 0x00007fff) <<  2) +
	          ((dir & 0x00000003) <<  0);
}

void
mw_posdir_unpack(uint32_t posdir, mw_pos_t *x, mw_pos_t *y, mw_dir_t *dir)
{
	uint32_t tmp_dir;
	/* according to the mazewar protocol spec, the position and
	 * direction are packed into a 32-bit word like so:
	 *
	 *          +------------+------------+-----------+
	 * posdir = | position x | position y | direction |
	 *          +------------+------------+-----------+
	 *          |- 15 bits --|-- 15 bits -|-- 2 bits -|
	 */
	*x      = ((posdir >> 17) & 0x00007fff);
	*y      = ((posdir >>  2) & 0x00007fff);
	tmp_dir = ((posdir >>  0) & 0x00000003);

	switch (tmp_dir) {
	case 0:
		*dir = MW_DIR_NORTH;
		break;
	case 1:
		*dir = MW_DIR_SOUTH;
		break;
	case 2:
		*dir = MW_DIR_EAST;
		break;
	case 3:
		*dir = MW_DIR_WEST;
		break;
	default:
		/* XXX: Should never reach here */
		ASSERT(0);
		break;
	}
}

