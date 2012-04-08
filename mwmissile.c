/*
 * mwmissile.c - Mazewar's missile object implementation
 */

#include "mwinternal.h"

/* Global counter used to uniquely assign ID numbers to missiles */
static mw_missile_id_t mw_missile_count = 0;

int
mwm_cons(mw_missile_t **m, mw_missile_id_t *id,
         mw_pos_t x, mw_pos_t y, mw_dir_t dir)
{
	mw_missile_t *tmp;

	ASSERT(m != NULL);

	tmp = (mw_missile_t *)malloc(sizeof(mw_missile_t));
	if (tmp == NULL)
		return -ENOMEM;

	INIT_LIST_HEAD(&tmp->mwm_list);
	/* XXX: Not thread safe. Accessing Global */
	tmp->mwm_id    = mw_missile_count++;
	tmp->mwm_x_pos = x;
	tmp->mwm_y_pos = y;
	tmp->mwm_dir   = dir;

	if (id != NULL)
		*id = tmp->mwm_id;

	*m = tmp;
	return 0;
}

int
mwm_dest(mw_missile_t *m)
{
	ASSERT(list_empty(&m->mwm_list));

	free(m);
	return 0;
}

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
