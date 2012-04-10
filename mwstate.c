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

	*s = tmp;
	return 0;
}

int
mws_dest(mw_state_t *s)
{
	ASSERT(list_empty(&s->mws_missiles));
	ASSERT(list_empty(&s->mws_rats));
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

void mws_render_wipe(const mw_state_t *s)
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

void mws_render_draw(const mw_state_t *s)
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

static mw_rat_t *__mws_get_rat(mw_state_t *s, mw_rat_id_t id)
{
	/* XXX: Yes, this is bad, and slow, and ugly; but it's simple. */
	mw_rat_t *r;
	list_for_each_entry(r, &s->mws_rats, mwr_list) {
		if (mwr_cmp_id(r, id) == 0)
			return r;
	}

	return NULL;
}

int mws_set_rat_xpos(mw_state_t *s, mw_rat_id_t id, mw_pos_t x)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_set_xpos(r, x);
}

int mws_set_rat_ypos(mw_state_t *s, mw_rat_id_t id, mw_pos_t y)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_set_ypos(r, y);
}

int mws_set_rat_dir(mw_state_t *s, mw_rat_id_t id, mw_dir_t dir)
{
	mw_rat_t *r = __mws_get_rat(s, id);

	if (r == NULL)
		return -1;

	return mwr_set_dir(r, dir);
}

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
