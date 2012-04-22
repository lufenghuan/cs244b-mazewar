/*
 * mwrat.c - Mazewar's rat object implemenation
 */

#include "mwinternal.h"

/* For rendering functions (i.e. HackMazeBitmap) */
#include "mazewar.h"

#include <string.h>

/* The Mazewar Protocol Spec states that in the event that a rat
 * does not have an outstanding missile, the 32-bit missile posdir
 * word will be 0xffffffff.
 */
#define NULL_MISSILE_POSDIR 0xffffffff

typedef struct mw_index_hole {
	list_head    mwih_list;
	unsigned int mwih_index;
} mw_index_hole_t;

static unsigned int mw_index = 0;

/* As rat's leave the game, there needs to be a mechanism to reuse the
 * mw_index values that these rats were using. Originally, a
 * monotonically increasing index value was used to assign indexes to
 * new rats. This created holes in the currently in use indexes as rats
 * left the game. To fix this, these holes are tracked as rats leave,
 * and can then be reused when a new rat is created (rather than
 * incrementing the mw_index global and using that value).
 */
LIST_HEAD(mw_index_holes);

static void
__mwr_init_state_pkt_timeout(struct timeval *timeout)
{
	timeout->tv_sec  = 0;
	timeout->tv_usec = 500000;
}

static void
__mwr_init_name_pkt_timeout(struct timeval *timeout)
{
	timeout->tv_sec  = 5;
	timeout->tv_usec = 0;
}

int
mwr_cons(mw_rat_t **r, mw_guid_t *id,
         mw_pos_t x, mw_pos_t y, mw_dir_t dir,
         const char *name)
{
	mw_rat_t *tmp;
	mw_index_hole_t *h;

	tmp = (mw_rat_t *)malloc(sizeof(mw_rat_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mwr_list);
	/* XXX: Not thread safe. Accessing Global */
	tmp->mwr_id       = mw_rand();
	tmp->mwr_x_pos    = tmp->mwr_x_wipe = x;
	tmp->mwr_y_pos    = tmp->mwr_y_wipe = y;
	tmp->mwr_dir      = dir;

	/* XXX: Not thread safe. Accessing global. */
	if (list_empty(&mw_index_holes)) {
		tmp->mwr_mw_index = mw_index++;
	} else {
		/* Grab the first index in the list of holes and use
		 * that as this rat's mw_index value.
		 */
		h = list_entry(mw_index_holes.next, mw_index_hole_t,
		               mwih_list);

		tmp->mwr_mw_index = h->mwih_index;

		/* Delete this hole as it has been recycled */
		list_del(&h->mwih_list);
		free(h);
	}

	tmp->mwr_name  = strdup(name);
	if (tmp->mwr_name == NULL) {
		mwr_dest(tmp);
		return -ENOMEM;
	}

	tmp->mwr_missile      = NULL;
	tmp->mwr_score        = 0;
	tmp->mwr_mcast_addr   = NULL;
	tmp->mwr_mcast_socket = -1;
	tmp->mwr_pkt_seqno    = 0;

	__mwr_init_state_pkt_timeout(&tmp->mwr_state_pkt_timeout);
	__mwr_init_name_pkt_timeout(&tmp->mwr_name_pkt_timeout);
	gettimeofday(&tmp->mwr_lasttime, NULL);

	tmp->mwr_is_local = 0;

	if (id != NULL)
		*id = tmp->mwr_id;

	*r = tmp;
	return 0;
}

int
mwr_dest(mw_rat_t *r)
{
	int rc = 0;

	/* If it is on a list, remove it from the list */
	if (!list_empty(&r->mwr_list))
		list_del(&r->mwr_list);

	if (r->mwr_name != NULL)
		free(r->mwr_name);

	ClearRatPosition(r->mwr_mw_index);
	mwr_send_leaving_pkt(r);

	mw_index_hole_t *h = (mw_index_hole_t *)malloc(sizeof(mw_index_hole_t));
	if (h == NULL) {
		rc = -ENOMEM;
	} else {
		/* XXX: Not thread safe. Accessing global */
		list_add_tail(&h->mwih_list, &mw_index_holes);
		h->mwih_index = r->mwr_mw_index;
	}

	free(r);
	return rc;
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

	/* Draw rat's arrow in top down view */
	HackMazeBitmap(Loc(r->mwr_x_pos), Loc(r->mwr_y_pos),
	               &normalArrows[r->mwr_dir]);

	/* Draw rat's score and name on the scorecard */
	UpdateScoreCardWithName(r->mwr_mw_index, r->mwr_name);
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
	mwr_send_state_pkt(r);

	SetRatPosition(r->mwr_mw_index, Loc(r->mwr_x_pos),
	               Loc(r->mwr_y_pos), Direction(r->mwr_dir));

	return 0;
}

int
mwr_set_ypos(mw_rat_t *r, mw_pos_t y)
{
	r->mwr_y_wipe = r->mwr_y_pos;
	r->mwr_y_pos  = y;
	mwr_send_state_pkt(r);
	return 0;
}

int
mwr_set_dir(mw_rat_t *r, mw_dir_t dir)
{
	r->mwr_dir = dir;
	mwr_send_state_pkt(r);
	return 0;
}

int
mwr_set_missile_packed_posdir(mw_rat_t *r, uint32_t posdir)
{
	mw_pos_t x, y;
	mw_dir_t dir;
	int rc;

	if (posdir == NULL_MISSILE_POSDIR) {
		mwr_rm_missile(r);
		return 0;
	}

	mw_posdir_unpack(posdir, &x, &y, &dir);
	if (r->mwr_missile == NULL) {
		rc = mwm_cons(&r->mwr_missile, NULL, x, y, dir);
		if (rc)
			return rc;
	} else {
		mwm_set_xpos(r->mwr_missile, x);
		mwm_set_ypos(r->mwr_missile, y);
		mwm_set_dir(r->mwr_missile, dir);
	}

	return 0;
}

int
mwr_set_score(mw_rat_t *r, mw_score_t score)
{
	r->mwr_score = score;
	mwr_send_state_pkt(r);
	return 0;
}

int
mwr_rm_missile(mw_rat_t *r)
{
	if (r->mwr_missile == NULL)
		return 1;

	/* XXX: This is a bit of a hack, but the
	 *      missile's position needs to be wiped
	 *      first.
	 */
	mwm_render_wipe(r->mwr_missile);

	mwm_dest(r->mwr_missile);
	r->mwr_missile = NULL;
	mwr_send_state_pkt(r);
	return 0;
}

int
mwr_set_is_local_flag(mw_rat_t *r, int is_local)
{
	r->mwr_is_local = is_local;
	return 0;
}

int
mwr_set_id(mw_rat_t *r, mw_guid_t id)
{
	r->mwr_id = id;
	return 0;
}

int
mwr_set_name(mw_rat_t *r, const char *name)
{
	if (r->mwr_name != NULL)
		free(r->mwr_name);

	r->mwr_name  = strdup(name);
	if (r->mwr_name == NULL)
		return -1;

	return 0;
}

int
mwr_get_xpos(mw_rat_t *r, mw_pos_t *x)
{
	*x = r->mwr_x_pos;
	return 0;
}

int
mwr_get_ypos(mw_rat_t *r, mw_pos_t *y)
{
	*y = r->mwr_y_pos;
	return 0;
}

int
mwr_get_score(mw_rat_t *r, mw_score_t *score)
{
	*score = r->mwr_score;
	return 0;
}

int
mwr_get_id(mw_rat_t *r, mw_guid_t *id)
{
	*id = r->mwr_id;
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
mwr_is_occupying_cell(mw_rat_t *r, mw_pos_t x, mw_pos_t y)
{
	if (r->mwr_x_pos == x && r->mwr_y_pos == y)
		return 1;

	return 0;
}

int
mwr_missile_is_occupying_cell(mw_rat_t *r, mw_pos_t x, mw_pos_t y)
{
	if (r->mwr_missile == NULL)
		return 0;

	return mwm_is_occupying_cell(r->mwr_missile, x, y);
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

	/* Each missile fired costs 1 point. */
	r->mwr_score--;

	/* Need to let peers know about this new missile. */
	mwr_send_state_pkt(r);
	return 0;
}

int
mwr_tagged_by(mw_rat_t *r, mw_guid_t tagger_id)
{
	/* TODO: Add implementation */
	return 0;
}

int
mwr_tagged(mw_rat_t *r, mw_guid_t taggee_id)
{
	/* TODO: Add implementation */
	return 0;
}

static void
__mwr_update_missile(mw_rat_t *r, int **maze)
{
	mw_missile_t *m = r->mwr_missile;
	mw_pos_t xbefore, ybefore;
	mw_pos_t xafter,  yafter;

	if (m == NULL)
		return;

	mwm_get_xpos(m, &xbefore);
	mwm_get_ypos(m, &ybefore);

	mwm_update(m);

	mwm_get_xpos(m, &xafter);
	mwm_get_ypos(m, &yafter);

	/* 1 == wall at position x, y */
	if (maze[xafter][yafter] == 1) {
		/* Missile hit a wall, time to destroy it */
		mwr_rm_missile(r);
	}

	/* A state packet must be sent on every state change, including
	 * when a rat's missile changes position.
	 */
	if ((xbefore != xafter) || (ybefore != yafter))
		mwr_send_state_pkt(r);

}

static void
__mwr_update_timeouts(mw_rat_t *r)
{
	struct timeval curtime, diff;

	gettimeofday(&curtime, NULL);

	mw_timeval_difference(&diff, &curtime, &r->mwr_lasttime);

	mw_timeval_difference(&r->mwr_state_pkt_timeout,
	                      &r->mwr_state_pkt_timeout, &diff);

	mw_timeval_difference(&r->mwr_name_pkt_timeout,
	                      &r->mwr_name_pkt_timeout, &diff);

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

static int
__mwr_name_pkt_timeout_triggered(mw_rat_t *r)
{
	if (mw_timeval_timeout_triggered(&r->mwr_name_pkt_timeout)) {
		__mwr_init_name_pkt_timeout(&r->mwr_name_pkt_timeout);
		return 1;
	}

	return 0;
}

void
mwr_update(mw_rat_t *r, int **maze)
{
	if (!r->mwr_is_local)
		return;

	__mwr_update_missile(r, maze);
	__mwr_update_timeouts(r);

	if (__mwr_state_pkt_timeout_triggered(r))
		mwr_send_state_pkt(r);

	if (__mwr_name_pkt_timeout_triggered(r))
		mwr_send_name_pkt(r);
}

void
mwr_set_addr(mw_rat_t *r, struct sockaddr *mcast, int socket)
{
	r->mwr_mcast_addr   = mcast;
	r->mwr_mcast_socket = socket;
}

int
mwr_send_state_pkt(mw_rat_t *r)
{
	mw_pkt_state_t pkt;

	if (!r->mwr_is_local)
		return 0;

	ASSERT(r->mwr_mcast_addr != NULL);

	/* TODO: Fill in pkt with actual state information */
	pkt.mwps_header.mwph_descriptor = MW_PKT_HDR_DESCRIPTOR_STATE;
	pkt.mwps_header.mwph_mbz[0]     = 0;
	pkt.mwps_header.mwph_mbz[1]     = 0;
	pkt.mwps_header.mwph_mbz[2]     = 0;
	pkt.mwps_header.mwph_guid       = r->mwr_id;
	pkt.mwps_header.mwph_seqno      = r->mwr_pkt_seqno++;
	pkt.mwps_score                  = r->mwr_score;
	pkt.mwps_crt                    = mw_rand();

	mw_posdir_pack(&pkt.mwps_rat_posdir, r->mwr_x_pos,
	                                     r->mwr_y_pos, r->mwr_dir);

	if (r->mwr_missile == NULL)
		pkt.mwps_missile_posdir = NULL_MISSILE_POSDIR;
	else
		mwm_get_packed_posdir(r->mwr_missile,
		                      &pkt.mwps_missile_posdir);

	memset(pkt.mwps_mbz, 0, sizeof(pkt.mwps_mbz));

	/* The timeout can be re-initialized because a state packet is
	 * being transmitted. Keeps the caller from having to do this.
	 */
	__mwr_init_state_pkt_timeout(&r->mwr_state_pkt_timeout);

	/* TODO: Must swab pkt before sending it on the wire */
	return sendto(r->mwr_mcast_socket, &pkt, sizeof(mw_pkt_state_t), 0,
	              r->mwr_mcast_addr, sizeof(struct sockaddr));
}

int
mwr_send_name_pkt(mw_rat_t *r)
{
	mw_pkt_nickname_t pkt;

	if (!r->mwr_is_local)
		return 0;

	ASSERT(r->mwr_mcast_addr != NULL);

	pkt.mwpn_header.mwph_descriptor = MW_PKT_HDR_DESCRIPTOR_NICKNAME;
	pkt.mwpn_header.mwph_mbz[0]     = 0;
	pkt.mwpn_header.mwph_mbz[1]     = 0;
	pkt.mwpn_header.mwph_mbz[2]     = 0;
	pkt.mwpn_header.mwph_guid       = r->mwr_id;
	pkt.mwpn_header.mwph_seqno      = r->mwr_pkt_seqno++;

	strncpy((char *)&pkt.mwpn_nickname, r->mwr_name, MW_NICKNAME_LEN);
	pkt.mwpn_nickname[MW_NICKNAME_LEN-1] = '\0';

	memset(pkt.mwpn_mbz, 0, sizeof(pkt.mwpn_mbz));

	/* TODO: Must swab pkt before sending it on the wire */
	return sendto(r->mwr_mcast_socket, &pkt, sizeof(mw_pkt_nickname_t), 0,
	              r->mwr_mcast_addr, sizeof(struct sockaddr));
}

int
mwr_send_leaving_pkt(mw_rat_t *r)
{
	mw_pkt_leaving_t pkt;

	if (!r->mwr_is_local)
		return 0;

	ASSERT(r->mwr_mcast_addr != NULL);

	pkt.mwpl_header.mwph_descriptor = MW_PKT_HDR_DESCRIPTOR_LEAVING;
	pkt.mwpl_header.mwph_mbz[0]     = 0;
	pkt.mwpl_header.mwph_mbz[1]     = 0;
	pkt.mwpl_header.mwph_mbz[2]     = 0;
	pkt.mwpl_header.mwph_guid       = r->mwr_id;
	pkt.mwpl_header.mwph_seqno      = r->mwr_pkt_seqno++;

	pkt.mwpl_leaving_guid           = r->mwr_id;

	memset(pkt.mwpl_mbz, 0, sizeof(pkt.mwpl_mbz));

	/* TODO: Must swab pkt before sending it on the wire */
	return sendto(r->mwr_mcast_socket, &pkt, sizeof(mw_pkt_leaving_t), 0,
	              r->mwr_mcast_addr, sizeof(struct sockaddr));
}
