/*
 * mwstate.c - Mazewar's shared state object implementation
 */

#include "mwinternal.h"

#define SECS_IN_PHASE_DISCOVERY 5
#define DEFAULT_RAT_NAME "unknown"

typedef struct mw_rat_list_ent {
	struct list_head  mwrle_list;
	mw_rat_t         *mwrle_rat;
	struct timeval    mwrle_timeout;
} mw_rat_list_ent_t;

static void
__mws_init_rat_list_ent_timeout(struct timeval *timeout)
{
	timeout->tv_sec  = 10;
	timeout->tv_usec = 0;
}

static mw_rat_list_ent_t *
__mws_get_rat_list_ent(mw_state_t *s, mw_guid_t id)
{
	/* XXX: Yes, this is bad, and slow, and ugly; but it's simple. */
	mw_rat_list_ent_t *e;
	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		if (mwr_cmp_id(e->mwrle_rat, id) == 0)
			return e;
	}

	return NULL;
}

static mw_rat_t *
__mws_get_rat(mw_state_t *s, mw_guid_t id)
{
	mw_rat_list_ent_t *e;

	e = __mws_get_rat_list_ent(s, id);
	if (e != NULL)
		return e->mwrle_rat;

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

	INIT_LIST_HEAD(&tmp->mws_rat_list);
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

	ASSERT(list_empty(&s->mws_rat_list));

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
	if (s->mws_phase == MWS_PHASE_DISCOVERY)
		return -1;

	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	return mwr_fire_missile(r, s->mws_maze);
}

int
mws_quit(mw_state_t *s)
{
	mw_rat_list_ent_t *e, *n;
	list_for_each_entry_safe(e, n, &s->mws_rat_list, mwrle_list) {
		list_del(&e->mwrle_list);
		mwr_dest(e->mwrle_rat);
		free(e);
	}

	s->mws_local_rat_id = -1;
	return 0;
}

int
mws_add_rat(mw_state_t *s, mw_guid_t *id,
            mw_pos_t x, mw_pos_t y, mw_dir_t dir,
            const char *name)
{
	mw_rat_list_ent_t *e;
	mw_rat_t *r;
	int rc;

	e = (mw_rat_list_ent_t *)malloc(sizeof(mw_rat_list_ent_t));
	if (e == NULL) {
		rc = -ENOMEM;
		goto ent_fail;
	}

	rc = mwr_cons(&r, id, x, y, dir, name);
	if (rc)
		goto err_rat;

	mwr_set_addr(r, s->mws_mcast_addr, s->mws_mcast_socket);

	__mws_init_rat_list_ent_timeout(&e->mwrle_timeout);
	e->mwrle_rat = r;

	list_add_tail(&e->mwrle_list, &s->mws_rat_list);
	return rc;

err_rat:
	free(e);

ent_fail:
	return rc;
}

void
mws_render_wipe(const mw_state_t *s)
{
	mw_rat_list_ent_t *e;
	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		mwr_render_wipe(e->mwrle_rat);
	}
}

void
mws_render_draw(const mw_state_t *s)
{
	mw_rat_list_ent_t *e;
	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		mwr_render_draw(e->mwrle_rat);
	}
}

static void
__mws_update_rats(mw_state_t *s)
{
	mw_rat_list_ent_t *e;

	ASSERT(s->mws_mcast_addr != NULL);

	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		mwr_update(e->mwrle_rat, s->mws_maze);
	}
}

static void
__mws_update_timevals(mw_state_t *s)
{
	struct timeval curtime, diff;
	mw_rat_list_ent_t *e;

	gettimeofday(&curtime, NULL);

	mw_timeval_difference(&diff, &curtime, &s->mws_lasttime);
	mw_timeval_sum(&s->mws_elapsedtime, &s->mws_elapsedtime, &diff);

	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		mw_timeval_difference(&e->mwrle_timeout,
		                      &e->mwrle_timeout, &diff);
	}

	gettimeofday(&s->mws_lasttime, NULL);
}

static void
__mws_evict_dead_peers(mw_state_t *s)
{
	mw_rat_list_ent_t *e, *n;
	list_for_each_entry_safe(e, n, &s->mws_rat_list, mwrle_list) {
		if (mwr_cmp_id(e->mwrle_rat, s->mws_local_rat_id) == 0)
			continue; /* Can't evict local rat */

		if (mw_timeval_timeout_triggered(&e->mwrle_timeout)) {
			list_del(&e->mwrle_list);
			mwr_dest(e->mwrle_rat);
			free(e);
		}
	}
}

static void
__mws_check_for_tagging(mw_state_t *s)
{
	mw_rat_list_ent_t *e;

	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		mw_guid_t         tagger_id;
		mw_rat_list_ent_t *each;

		mwr_get_id(e->mwrle_rat, &tagger_id);

		list_for_each_entry(each, &s->mws_rat_list, mwrle_list) {
			mw_guid_t taggee_id;
			mw_pos_t  x, y;

			if (each == e) /* Can't tag yourself */
				continue;

			mwr_get_id(each->mwrle_rat, &taggee_id);
			mwr_get_xpos(each->mwrle_rat, &x);
			mwr_get_ypos(each->mwrle_rat, &y);

			if (mwr_missile_is_occupying_cell(e->mwrle_rat, x, y))
				mwr_tagged_by(each->mwrle_rat, tagger_id);
		}
	}
}

void
mws_update(mw_state_t *s)
{
	ASSERT(s->mws_maze != NULL);

	__mws_update_timevals(s);

	if (s->mws_phase == MWS_PHASE_DISCOVERY) {
		if (s->mws_elapsedtime.tv_sec >= SECS_IN_PHASE_DISCOVERY) {
			s->mws_phase = MWS_PHASE_ACTIVE;
			__mws_init_elapsedtime(&s->mws_elapsedtime);
		}
	} else {
		__mws_update_rats(s);
		__mws_check_for_tagging(s);
	}

	__mws_evict_dead_peers(s);
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
__mws_is_cell_occupied(mw_state_t *s, mw_pos_t x, mw_pos_t y)
{
	mw_rat_list_ent_t *e;
	list_for_each_entry(e, &s->mws_rat_list, mwrle_list) {
		/* Skip the local rat when deciding if a cell is occupied. */
		if (mwr_cmp_id(e->mwrle_rat, s->mws_local_rat_id) == 0)
			continue;

		if (mwr_is_occupying_cell(e->mwrle_rat, x, y))
			return 1;
	}

	return 0;
}

int
mws_set_rat_xpos(mw_state_t *s, mw_guid_t id, mw_pos_t x)
{
	mw_pos_t y;

	if (s->mws_phase == MWS_PHASE_DISCOVERY)
		return -1;

	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	mwr_get_ypos(r, &y);
	if (__mws_is_cell_occupied(s, x, y))
		return -1;

	return mwr_set_xpos(r, x);
}

int
mws_set_rat_ypos(mw_state_t *s, mw_guid_t id, mw_pos_t y)
{
	mw_pos_t x;

	if (s->mws_phase == MWS_PHASE_DISCOVERY)
		return -1;

	mw_rat_t *r = __mws_get_rat(s, id);
	if (r == NULL)
		return -1;

	mwr_get_xpos(r, &x);
	if (__mws_is_cell_occupied(s, x, y))
		return -1;

	return mwr_set_ypos(r, y);
}

int
mws_set_rat_dir(mw_state_t *s, mw_guid_t id, mw_dir_t dir)
{
	if (s->mws_phase == MWS_PHASE_DISCOVERY)
		return -1;

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

int
__mws_is_local_occupying_cell(mw_state_t *s, mw_pos_t x, mw_pos_t y)
{
	mw_pos_t local_x, local_y;
	mw_rat_t *r;

	r = __mws_get_rat(s, s->mws_local_rat_id);
	if (r == NULL) /* XXX: No local rat? */
		return 0;

	mwr_get_xpos(r, &local_x);
	mwr_get_ypos(r, &local_y);

	return (x == local_x && y == local_y);
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

	if (__mws_is_local_occupying_cell(s, x, y))
		; /* TODO: Resolve collision based on CRT */

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
	/* XXX: We drop received packets sent by this client, so this
	 * should never trigger unless another client is reporting
	 * this local rat as leaving (which shouldn't happen).
	 */
	ASSERT(s->mws_local_rat_id != pkt->mwpl_leaving_guid);

	mw_rat_list_ent_t *e =
	              __mws_get_rat_list_ent(s, pkt->mwpl_leaving_guid);

	if (e != NULL) { /* XXX: No rat for this guid? */
		list_del(&e->mwrle_list);
		mwr_dest(e->mwrle_rat);
		free(e);
	}
}

void
__mws_process_pkt_tagged(mw_state_t *s, mw_pkt_tagged_t *pkt)
{
	/* My local rat wasn't the shooter in this tagged event, so
	 * ignore this packet.
	 */
	if (s->mws_local_rat_id != pkt->mwpt_shooter_guid)
		return;

	mw_rat_t *r = __mws_get_rat(s, pkt->mwpt_shooter_guid);
	if (r != NULL) /* XXX: No rat for this guid? */
		mwr_tagged(r, pkt->mwpt_header.mwph_guid,
		              pkt->mwpt_header.mwph_seqno);
}

void
__mws_process_pkt_ack(mw_state_t *s, mw_pkt_ack_t *pkt)
{
	mw_rat_t *r = __mws_get_rat(s, pkt->mwpa_guid);
	if (r != NULL) /* XXX: No rat for this guid? */
		mwr_process_ack_pkt(r, pkt->mwpa_seqno);
}

void
mws_receive_pkt(mw_state_t *s, mw_pkt_header_t *pkt)
{
	mw_rat_list_ent_t *e;

	mw_ntoh_pkt_header(pkt);

	if (s->mws_local_rat_id == pkt->mwph_guid)
		return; /* Ignore our own local rat's packets */

	/* To ensure a list entry doesn't timeout, we need to
	 * reinitialize the timeout when a packet from that rat is
	 * received.
	 */
	e = __mws_get_rat_list_ent(s, pkt->mwph_guid);
	if (e != NULL) /* XXX: No ent for this guid? */
		__mws_init_rat_list_ent_timeout(&e->mwrle_timeout);

	switch (pkt->mwph_descriptor) {
	case MW_PKT_HDR_DESCRIPTOR_STATE:
		mw_ntoh_pkt_state((mw_pkt_state_t *)pkt);
		__mws_process_pkt_state(s, (mw_pkt_state_t *)pkt);
		break;
	case MW_PKT_HDR_DESCRIPTOR_NICKNAME:
		mw_ntoh_pkt_nickname((mw_pkt_nickname_t *)pkt);
		__mws_process_pkt_nickname(s, (mw_pkt_nickname_t *)pkt);
		break;
	case MW_PKT_HDR_DESCRIPTOR_LEAVING:
		mw_ntoh_pkt_leaving((mw_pkt_leaving_t *)pkt);
		__mws_process_pkt_leaving(s, (mw_pkt_leaving_t *)pkt);
		break;
	case MW_PKT_HDR_DESCRIPTOR_TAGGED:
		mw_ntoh_pkt_tagged((mw_pkt_tagged_t *)pkt);
		__mws_process_pkt_tagged(s, (mw_pkt_tagged_t *)pkt);
		break;
	case MW_PKT_HDR_DESCRIPTOR_ACK:
		mw_ntoh_pkt_ack((mw_pkt_ack_t *)pkt);
		__mws_process_pkt_ack(s, (mw_pkt_ack_t *)pkt);
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
