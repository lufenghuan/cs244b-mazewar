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

	*s = tmp;
	return 0;
}

int
mws_dest(mw_state_t *s)
{
	ASSERT(list_empty(&s->mws_missiles));
	free(s);
	return 0;
}

int
mws_add_missile(mw_state_t *s, mw_missile_id_t *id,
                mw_pos_t x, mw_pos_t y, mw_dir_t dir)
{
	mw_missile_t *m;
	int rc;

	rc = mwm_cons(&m, id, x, y, dir);
	if (rc)
		return rc;

	list_add_tail(&m->mwm_list, &s->mws_missiles);
	return 0;
}

void mws_render(const mw_state_t *s)
{
	mw_missile_t *m;

	list_for_each_entry(m, &s->mws_missiles, mwm_list) {
		mwm_render(m);
	}
}

static void __mws_update_missiles(mw_state_t *s)
{
	mw_missile_t *m;
	list_for_each_entry(m, &s->mws_missiles, mwm_list) {
		mwm_update(m);
	}
}

void mws_update(mw_state_t *s)
{
	__mws_update_missiles(s);
}


/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
