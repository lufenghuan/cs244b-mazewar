/*
 * mwrat.c - Mazewar's rat object implemenation
 */

#include "mwinternal.h"

/* For rendering functions (i.e. HackMazeBitmap) */
#include "mazewar.h"

#include <string.h>

static void
__mwr_init_state_pkt_timeout(struct timeval *timeout)
{
	timeout->tv_sec  = 0;
	timeout->tv_usec = 500000;
}

int
mwr_cons(mw_rat_t **r, mw_guid_t *id,
         mw_pos_t x, mw_pos_t y, mw_dir_t dir,
         const char *name)
{
	mw_rat_t *tmp;

	tmp = (mw_rat_t *)malloc(sizeof(mw_rat_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mwr_list);
	/* XXX: Not thread safe. Accessing Global */
	tmp->mwr_id    = mw_rand();
	tmp->mwr_x_pos = tmp->mwr_x_wipe = x;
	tmp->mwr_y_pos = tmp->mwr_y_wipe = y;
	tmp->mwr_dir   = dir;

	tmp->mwr_name  = strdup(name);
	if (tmp->mwr_name == NULL) {
		mwr_dest(tmp);
		return -ENOMEM;
	}

	tmp->mwr_missile      = NULL;
	tmp->mwr_mcast_addr   = NULL;
	tmp->mwr_mcast_socket = -1;
	tmp->mwr_pkt_seqno    = 0;

	__mwr_init_state_pkt_timeout(&tmp->mwr_state_pkt_timeout);
	gettimeofday(&tmp->mwr_lasttime, NULL);

	if (id != NULL)
		*id = tmp->mwr_id;

	*r = tmp;
	return 0;
}

int
mwr_dest(mw_rat_t *r)
{
	ASSERT(list_empty(&r->mwr_list));

	if (r->mwr_name != NULL)
		free(r->mwr_name);

	free(r);
	return 0;
}

/* XXX: This should really be in the same file as the other BitCell's,
 *      and be included that way. Since that is not the case, it's
 *      unfortunately hacked in here.
 */
static BitCell empty = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

void
mwr_render_wipe(const mw_rat_t *r)
{
	if (r->mwr_missile != NULL)
		mwm_render_wipe(r->mwr_missile);

	HackMazeBitmap(Loc(r->mwr_x_wipe), Loc(r->mwr_y_wipe), &empty);
}

void
mwr_render_draw(const mw_rat_t *r)
{
	if (r->mwr_missile != NULL)
		mwm_render_draw(r->mwr_missile);

	HackMazeBitmap(Loc(r->mwr_x_pos), Loc(r->mwr_y_pos),
	               &normalArrows[r->mwr_dir]);
}

int
mwr_cmp_id(mw_rat_t *r, mw_guid_t id)
{
	if (r->mwr_id > id)
		return 1;
	else if (r->mwr_id < id)
		return -1;
	else
		return 0;
}

int
mwr_set_xpos(mw_rat_t *r, mw_pos_t x)
{
	r->mwr_x_wipe = r->mwr_x_pos;
	r->mwr_x_pos  = x;
	return 0;
}

int
mwr_set_ypos(mw_rat_t *r, mw_pos_t y)
{
	r->mwr_y_wipe = r->mwr_y_pos;
	r->mwr_y_pos  = y;
	return 0;
}

int
mwr_set_dir(mw_rat_t *r, mw_dir_t dir)
{
	r->mwr_dir = dir;
	return 0;
}

void
__mwr_xpos_plus_dir(mw_pos_t *xnew, mw_pos_t xold, mw_dir_t dir)
{
	/* "North" is defined to be to the right, positive X direction */
	switch(dir) {
	case MW_DIR_NORTH:
		*xnew = xold + 1;
		break;
	case MW_DIR_SOUTH:
		*xnew = xold - 1;
		break;
	default:
		*xnew = xold;
		break;
	}
}

void
__mwr_ypos_plus_dir(mw_pos_t *ynew, mw_pos_t yold, mw_dir_t dir)
{
	/* "North" is defined to be to the right, positive X direction */
	switch(dir) {
	case MW_DIR_EAST:
		*ynew = yold + 1;
		break;
	case MW_DIR_WEST:
		*ynew = yold - 1;
		break;
	default:
		*ynew = yold;
		break;
	}
}

int
mwr_fire_missile(mw_rat_t *r, int **maze)
{
	mw_pos_t x, y;
	int rc;

	/* Rat already has an outstanding missile, it isn't allowed to
	 * fire until this missile is destroyed.
	 */
	if (r->mwr_missile != NULL)
		return 1;

	/* The missile needs to be constructed such that its first
	 * position is directly in front of the rat, thus we need to
	 * calculate this new position.
	 */
	__mwr_xpos_plus_dir(&x, r->mwr_x_pos, r->mwr_dir);
	__mwr_ypos_plus_dir(&y, r->mwr_y_pos, r->mwr_dir);

	/* 1 == wall at position, missile shot directly into wall. */
	if (maze[x][y] == 1)
		return 0;

	/* Rat can only have a single missile, no need for ID */
	rc = mwm_cons(&r->mwr_missile, NULL, x, y, r->mwr_dir);
	if (rc)
		return rc;

	return 0;
}

static void
__mwr_update_missile(mw_rat_t *r, int **maze)
{
	mw_missile_t *m = r->mwr_missile;
	mw_pos_t x, y;

	if (m == NULL)
		return;

	mwm_update(m);

	mwm_get_xpos(m, &x);
	mwm_get_ypos(m, &y);

	/* 1 == wall at position x, y */
	if (maze[x][y] == 1) {
		/* Missile hit a wall, time to destroy it */

		/* XXX: This is a bit of a hack, but the
		 *      missile's position needs to be wiped
		 *      first.
		 */
		mwm_render_wipe(m);

		mwm_dest(m);
		r->mwr_missile = m = NULL;
	}
}

static void
__mwr_update_timeouts(mw_rat_t *r)
{
	struct timeval curtime, diff;

	gettimeofday(&curtime, NULL);

	mw_timeval_difference(&diff, &curtime, &r->mwr_lasttime);

	mw_timeval_difference(&r->mwr_state_pkt_timeout,
	                      &r->mwr_state_pkt_timeout, &diff);

	gettimeofday(&r->mwr_lasttime, NULL);
}

static int
__mwr_state_pkt_timeout_triggered(mw_rat_t *r)
{
	if (mw_timeval_timeout_triggered(&r->mwr_state_pkt_timeout)) {
		__mwr_init_state_pkt_timeout(&r->mwr_state_pkt_timeout);
		return 1;
	}

	return 0;
}

void
mwr_update(mw_rat_t *r, int **maze)
{
	__mwr_update_missile(r, maze);
	__mwr_update_timeouts(r);

	if (__mwr_state_pkt_timeout_triggered(r))
		mwr_send_state_pkt(r);
}

int
mwr_send_state_pkt(mw_rat_t *r)
{
	mw_pkt_state pkt;

	ASSERT(r->mwr_mcast_addr != NULL);

	/* TODO: Fill in pkt with actual state information */
	pkt.mwps_header.mwph_descriptor = MW_PKT_HDR_DESCRIPTOR_STATE;
	pkt.mwps_header.mwph_mbz[0]     = 0;
	pkt.mwps_header.mwph_mbz[1]     = 0;
	pkt.mwps_header.mwph_mbz[2]     = 0;
	pkt.mwps_header.mwph_guid       = r->mwr_id;
	pkt.mwps_header.mwph_seqno      = r->mwr_pkt_seqno++;
	pkt.mwps_rat_posdir             = 0xABAD1DEA;
	pkt.mwps_missile_posdir         = 0xABAD1DEA;
	pkt.mwps_score                  = 0xABAD1DEA;
	pkt.mwps_timestamp              = 0xABAD1DEA;
	pkt.mwps_crt                    = mw_rand();

	/* The timeout can be re-initialized because a state packet is
	 * being transmitted. Keeps the caller from having to do this.
	 */
	__mwr_init_state_pkt_timeout(&r->mwr_state_pkt_timeout);

	/* XXX: Must swap pkt before sending it on the wire */
	return sendto(r->mwr_mcast_socket, &pkt, sizeof(mw_pkt_state), 0,
	              r->mwr_mcast_addr, sizeof(struct sockaddr));
}

void
mwr_set_addr(mw_rat_t *r, struct sockaddr *mcast, int socket)
{
	r->mwr_mcast_addr   = mcast;
	r->mwr_mcast_socket = socket;
}
