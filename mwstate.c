/*
 * mwstate.c - Mazewar's shared state object implementation
 */

#include "mwinternal.h"

#define SECS_IN_PHASE_DISCOVERY 5
#define DEFAULT_RAT_NAME "unknown"

static mw_rat_t *
__mws_get_rat(mw_state_t *s, mw_guid_t id)
{
	/* XXX: Yes, this is bad, and slow, and ugly; but it's simple. */
	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		if (mwr_cmp_id(r, id) == 0)
			return r;
	}

	return NULL;
}

static void
__mws_init_elapsedtime(struct timeval *elapsedtime)
{
	elapsedtime->tv_sec  = 0;
	elapsedtime->tv_usec = 0;
}

int
mws_cons(mw_state_t **s)
{
	mw_state_t *tmp;

	tmp = (mw_state_t *)malloc(sizeof(mw_state_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mws_rats);
	tmp->mws_local_rat_id = -1;

	tmp->mws_phase = MWS_PHASE_DISCOVERY;
	__mws_init_elapsedtime(&tmp->mws_elapsedtime);

	gettimeofday(&tmp->mws_lasttime, NULL);

	tmp->mws_mcast_addr   = NULL;
	tmp->mws_mcast_socket = -1;

	tmp->mws_maze = NULL;
	tmp->mws_xmax = -1;
	tmp->mws_ymax = -1;

	*s = tmp;
	return 0;
}

int
mws_dest(mw_state_t *s)
{
	int i;

	ASSERT(list_empty(&s->mws_rats));

	if (s->mws_maze != NULL) {
		for (i = 0; i < s->mws_xmax; i++)
			free(s->mws_maze[i]);
		free(s->mws_maze);
	}

	free(s);
	return 0;
}

void
mws_set_maze(mw_state_t *s, int **maze, int xmax, int ymax)
{
	s->mws_maze = maze;
	s->mws_xmax = xmax;
	s->mws_ymax = ymax;
}

void
mws_set_addr(mw_state_t *s, struct sockaddr *mcast, int socket)
{
	s->mws_mcast_addr   = mcast;
	s->mws_mcast_socket = socket;
}

int
mws_fire_missile(mw_state_t *s, mw_guid_t id)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_fire_missile(r, s->mws_maze);
}

int
mws_quit(mw_state_t *s)
{
	mw_rat_t *r, *n;
	list_for_each_entry_safe(r, n, &s->mws_rats, mwr_list) {
		mwr_dest(r);
	}

	s->mws_local_rat_id = -1;
	return 0;
}

int
mws_add_rat(mw_state_t *s, mw_guid_t *id,
            mw_pos_t x, mw_pos_t y, mw_dir_t dir,
            const char *name)
{
	mw_rat_t *r;
	int rc;

	rc = mwr_cons(&r, id, x, y, dir, name);
	mwr_set_addr(r, s->mws_mcast_addr, s->mws_mcast_socket);

	if (rc)
		return rc;

	list_add_tail(&r->mwr_list, &s->mws_rats);
	return 0;
}

void
mws_render_wipe(const mw_state_t *s)
{
	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		mwr_render_wipe(r);
	}
}

void
mws_render_draw(const mw_state_t *s)
{
	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		mwr_render_draw(r);
	}
}

static void
__mws_update_rats(mw_state_t *s)
{
	ASSERT(s->mws_mcast_addr != NULL);

	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		mwr_update(r, s->mws_maze);
	}
}

static void
__mws_update_elapsedtime(mw_state_t *s)
{
	struct timeval curtime, diff;

	gettimeofday(&curtime, NULL);

	mw_timeval_difference(&diff, &curtime, &s->mws_lasttime);
	mw_timeval_sum(&s->mws_elapsedtime, &s->mws_elapsedtime, &diff);

	gettimeofday(&s->mws_lasttime, NULL);
}

void
mws_update(mw_state_t *s)
{
	ASSERT(s->mws_maze != NULL);

	__mws_update_elapsedtime(s);

	if (s->mws_phase == MWS_PHASE_DISCOVERY) {
		if (s->mws_elapsedtime.tv_sec >= SECS_IN_PHASE_DISCOVERY) {
			s->mws_phase = MWS_PHASE_ACTIVE;
			__mws_init_elapsedtime(&s->mws_elapsedtime);
		}
	} else {
		__mws_update_rats(s);
	}
}

int
mws_set_local_rat(mw_state_t *s, mw_guid_t id)
{
	s->mws_local_rat_id = id;

	mw_rat_t *r = __mws_get_rat(s, s->mws_local_rat_id);
	if (r == NULL)
		return -1;

	return mwr_set_is_local_flag(r, 1);
}

int
__mwr_is_cell_occupied(mw_state_t *s, mw_pos_t x, mw_pos_t y)
{
	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		/* Skip the local rat when deciding if a cell is occupied. */
		if (mwr_cmp_id(r, s->mws_local_rat_id) == 0)
			continue;

		if (mwr_is_occupying_cell(r, x, y))
			return 1;
	}

	return 0;
}

int
mws_set_rat_xpos(mw_state_t *s, mw_guid_t id, mw_pos_t x)
{
	mw_pos_t y;

	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	mwr_get_ypos(r, &y);
	if (__mwr_is_cell_occupied(s, x, y))
		return -1;

	return mwr_set_xpos(r, x);
}

int
mws_set_rat_ypos(mw_state_t *s, mw_guid_t id, mw_pos_t y)
{
	mw_pos_t x;

	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	mwr_get_xpos(r, &x);
	if (__mwr_is_cell_occupied(s, x, y))
		return -1;

	return mwr_set_ypos(r, y);
}

int
mws_set_rat_dir(mw_state_t *s, mw_guid_t id, mw_dir_t dir)
{
	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	return mwr_set_dir(r, dir);
}

int
mws_get_rat_score(mw_state_t *s, mw_guid_t id, mw_score_t *score)
{
	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	return mwr_get_score(r, score);
}

int
__mws_set_rat_id(mw_state_t *s, mw_guid_t old, mw_guid_t _new)
{
	mw_rat_t *r = __mws_get_rat(s, old);
	if (r == NULL)
		return -1;

	return mwr_set_id(r, _new);
}

void
__mws_process_pkt_state(mw_state_t *s, mw_pkt_state_t *pkt)
{
	mw_pos_t x, y;
	mw_dir_t dir;
	mw_guid_t guid;
	mw_rat_t *r;

	mw_posdir_unpack(pkt->mwps_rat_posdir, &x, &y, &dir);
	guid = pkt->mwps_header.mwph_guid;

	r = __mws_get_rat(s, guid);
	if (r == NULL) {
		mw_guid_t tmp;
		mws_add_rat(s, &tmp, x, y, dir, DEFAULT_RAT_NAME);
		__mws_set_rat_id(s, tmp, guid);
		return;
	}

	mwr_set_xpos(r, x);
	mwr_set_ypos(r, y);
	mwr_set_dir(r, dir);
	mwr_set_score(r, pkt->mwps_score);
	mwr_set_missile_packed_posdir(r, pkt->mwps_missile_posdir);

	/* TODO: Still need to account for CRT */
}

void
__mws_process_pkt_nickname(mw_state_t *s, mw_pkt_nickname_t *pkt)
{
	mw_rat_t *r = __mws_get_rat(s, pkt->mwpn_header.mwph_guid);

	if (r != NULL) /* XXX: No rat for this guid? */
		mwr_set_name(r, (char *)pkt->mwpn_nickname);
}

void
__mws_process_pkt_leaving(mw_state_t *s, mw_pkt_leaving_t *pkt)
{
	/* TODO: Add implementation */
}

void
mws_receive_pkt(mw_state_t *s, mw_pkt_header_t *pkt)
{
	/* TODO: Must swab pkt before processing it */

	if (s->mws_local_rat_id == pkt->mwph_guid)
		return; /* Ignore our own local rat's packets */

	switch (pkt->mwph_descriptor) {
	case MW_PKT_HDR_DESCRIPTOR_STATE:
		__mws_process_pkt_state(s, (mw_pkt_state_t *)pkt);
		break;
	case MW_PKT_HDR_DESCRIPTOR_NICKNAME:
		__mws_process_pkt_nickname(s, (mw_pkt_nickname_t *)pkt);
		break;
	case MW_PKT_HDR_DESCRIPTOR_LEAVING:
		__mws_process_pkt_leaving(s, (mw_pkt_leaving_t *)pkt);
		break;
	default:
		/* A packet was received with an unknown descriptor
		 * type, this _should_ never happen. In a development
		 * build, ASSERT, otherwise this is a no-op and the
		 * packet is dropped.
		 */
		ASSERT(0);
		break;
	}
}

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
