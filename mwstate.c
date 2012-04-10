/*
 * mwstate.c - Mazewar's shared state object implementation
 */

#include "mwinternal.h"

int
mws_cons(mw_state_t **s)
{
	mw_state_t *tmp;

	tmp = (mw_state_t *)malloc(sizeof(mw_state_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mws_missiles);
	INIT_LIST_HEAD(&tmp->mws_rats);

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

	ASSERT(list_empty(&s->mws_missiles));
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
__mws_xpos_plus_dir(mw_pos_t *xnew, mw_pos_t xold, mw_dir_t dir)
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
__mws_ypos_plus_dir(mw_pos_t *ynew, mw_pos_t yold, mw_dir_t dir)
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
mws_add_missile(mw_state_t *s, mw_missile_id_t *id,
                mw_pos_t x, mw_pos_t y, mw_dir_t dir)
{
	mw_missile_t *m;
	mw_pos_t xnew, ynew;
	int rc;

	/* The position passed in here is the position of the rat when
	 * the shot was fired. The missile needs to be constructed in
	 * front of the rat, thus we need to calculate it's new position
	 */
	__mws_xpos_plus_dir(&xnew, x, dir);
	__mws_ypos_plus_dir(&ynew, y, dir);

	/* 1 == wall at position, missile shot directly into wall. */
	if (s->mws_maze[xnew][ynew] == 1)
		return 0;

	rc = mwm_cons(&m, id, xnew, ynew, dir);
	if (rc)
		return rc;

	list_add_tail(&m->mwm_list, &s->mws_missiles);
	return 0;
}

int
mws_add_rat(mw_state_t *s, mw_rat_id_t *id,
            mw_pos_t x, mw_pos_t y, mw_dir_t dir,
            const char *name)
{
	mw_rat_t *r;
	int rc;

	rc = mwr_cons(&r, id, x, y, dir, name);

	if (rc)
		return rc;

	list_add_tail(&r->mwr_list, &s->mws_rats);
	return 0;
}

void
mws_render_wipe(const mw_state_t *s)
{
	mw_missile_t *m;
	list_for_each_entry(m, &s->mws_missiles, mwm_list) {
		mwm_render_wipe(m);
	}

	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		mwr_render_wipe(r);
	}
}

void
mws_render_draw(const mw_state_t *s)
{
	mw_missile_t *m;
	list_for_each_entry(m, &s->mws_missiles, mwm_list) {
		mwm_render_draw(m);
	}

	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		mwr_render_draw(r);
	}

}

static void
__mws_update_missiles(mw_state_t *s)
{
	mw_missile_t *pos, *n;

	/* Iteration using 'safe' is needed because the missile _may_ be
	 * removed during iteration. The 'safe' version allows this.
	 */
	list_for_each_entry_safe(pos, n, &s->mws_missiles, mwm_list) {
		mw_pos_t xpos, ypos;

		mwm_update(pos);

		mwm_get_xpos(pos, &xpos);
		mwm_get_ypos(pos, &ypos);

		/* 1 == wall at position x, y */
		if (s->mws_maze[xpos][ypos] == 1) {
			/* Missile hit a wall, time to destroy it */

			/* XXX: This is a bit of a hack, but the
			 *      missile's position needs to be wiped
			 *      first.
			 */
			mwm_render_wipe(pos);

			mwm_dest(pos);
		}
	}
}

void
mws_update(mw_state_t *s)
{
	ASSERT(s->mws_maze != NULL);
	__mws_update_missiles(s);
}

static mw_rat_t *
__mws_get_rat(mw_state_t *s, mw_rat_id_t id)
{
	/* XXX: Yes, this is bad, and slow, and ugly; but it's simple. */
	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		if (mwr_cmp_id(r, id) == 0)
			return r;
	}

	return NULL;
}

int
mws_set_rat_xpos(mw_state_t *s, mw_rat_id_t id, mw_pos_t x)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_set_xpos(r, x);
}

int
mws_set_rat_ypos(mw_state_t *s, mw_rat_id_t id, mw_pos_t y)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_set_ypos(r, y);
}

int
mws_set_rat_dir(mw_state_t *s, mw_rat_id_t id, mw_dir_t dir)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_set_dir(r, dir);
}

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
