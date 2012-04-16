/*
 * mwrat.c - Mazewar's rat object implemenation
 */

#include "mwinternal.h"

/* For rendering functions (i.e. HackMazeBitmap) */
#include "mazewar.h"

#include <string.h>

/* Global counter used to uniquely assign ID numbers to rats */
static mw_rat_id_t mw_rat_count = 0;

int
mwr_cons(mw_rat_t **r, mw_rat_id_t *id,
         mw_pos_t x, mw_pos_t y, mw_dir_t dir,
         const char *name)
{
	mw_rat_t *tmp;

	tmp = (mw_rat_t *)malloc(sizeof(mw_rat_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mwr_list);
	/* XXX: Not thread safe. Accessing Global */
	tmp->mwr_id    = mw_rat_count++;
	tmp->mwr_x_pos = tmp->mwr_x_wipe = x;
	tmp->mwr_y_pos = tmp->mwr_y_wipe = y;
	tmp->mwr_dir   = dir;

	tmp->mwr_name  = strdup(name);
	if (tmp->mwr_name == NULL) {
		mwr_dest(tmp);
		return -ENOMEM;
	}

	tmp->mwr_missile = NULL;

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
mwr_cmp_id(mw_rat_t *r, mw_rat_id_t id)
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

void
mwr_update(mw_rat_t *r, int **maze)
{
	__mwr_update_missile(r, maze);
}
