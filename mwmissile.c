/*
 * mwmissile.c - Mazewar's missile object implementation
 */

#include "mwinternal.h"

/* For rendering functions (i.e. HackMazeBitmap) */
#include "mazewar.h"

/* Global counter used to uniquely assign ID numbers to missiles */
static mw_missile_id_t mw_missile_count = 0;

static void
__mwm_init_timeout(struct timeval *timeout)
{
	timeout->tv_sec  = 1;
	timeout->tv_usec = 0;
}

static void
__mwm_update_position(mw_missile_t *m)
{
	/* "North" is defined to be to the right, positive X direction */

	/* XXX: These need to check for boundary conditions (i.e. when
	 * moving would cause the missile to move through a wall.
	 */
	switch(m->mwm_dir) {
	case MW_DIR_NORTH:
		m->mwm_x_wipe = m->mwm_x_pos++;
		break;
	case MW_DIR_SOUTH:
		m->mwm_x_wipe = m->mwm_x_pos--;
		break;
	case MW_DIR_EAST:
		m->mwm_y_wipe = m->mwm_y_pos++;
		break;
	case MW_DIR_WEST:
		m->mwm_y_wipe = m->mwm_y_pos--;
		break;
	default:
		ASSERT(0);
	}
}

int
mwm_cons(mw_missile_t **m, mw_missile_id_t *id,
         mw_pos_t x, mw_pos_t y, mw_dir_t dir)
{
	mw_missile_t *tmp;

	tmp = (mw_missile_t *)malloc(sizeof(mw_missile_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mwm_list);
	/* XXX: Not thread safe. Accessing Global */
	tmp->mwm_id    = mw_missile_count++;
	tmp->mwm_x_pos = tmp->mwm_x_wipe = x;
	tmp->mwm_y_pos = tmp->mwm_y_wipe = y;
	tmp->mwm_dir   = dir;

	__mwm_init_timeout(&tmp->mwm_timeout);
	gettimeofday(&tmp->mwm_lasttime, NULL);

	if (id != NULL)
		*id = tmp->mwm_id;

	*m = tmp;
	return 0;
}

int
mwm_dest(mw_missile_t *m)
{
	list_del(&m->mwm_list);
	free(m);
	return 0;
}

/* XXX: This should really be in the same file as the other BitCell's,
 *      and be included that way. Since that is not the case, it's
 *      unfortunately hacked in here.
 */
static BitCell empty = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

void
mwm_render_wipe(const mw_missile_t *m)
{
	HackMazeBitmap(Loc(m->mwm_x_wipe), Loc(m->mwm_y_wipe), &empty);
}

void
mwm_render_draw(const mw_missile_t *m)
{
	HackMazeBitmap(Loc(m->mwm_x_pos), Loc(m->mwm_y_pos), &missile[0]);
}

static int
__mwm_timeout_triggered(mw_missile_t *m)
{
	struct timeval curtime, diff;
	int triggered = 0;

	gettimeofday(&curtime, NULL);

	mw_timeval_difference(&diff, &curtime, &m->mwm_lasttime);
	mw_timeval_difference(&m->mwm_timeout, &m->mwm_timeout, &diff);

	if ((m->mwm_timeout.tv_sec < 0) || (m->mwm_timeout.tv_sec == 0 &&
	                                    m->mwm_timeout.tv_usec <= 0)) {
		triggered = 1;
		__mwm_init_timeout(&m->mwm_timeout);
	}

	gettimeofday(&m->mwm_lasttime, NULL);
	return triggered;
}

void
mwm_update(mw_missile_t *m)
{
	if (__mwm_timeout_triggered(m))
		__mwm_update_position(m);
}

void
mwm_get_xpos(mw_missile_t *m, mw_pos_t *xpos)
{
	*xpos = m->mwm_x_pos;
}

void
mwm_get_ypos(mw_missile_t *m, mw_pos_t *ypos)
{
	*ypos = m->mwm_y_pos;
}

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
