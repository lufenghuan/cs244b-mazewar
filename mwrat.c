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
	HackMazeBitmap(Loc(r->mwr_x_wipe), Loc(r->mwr_y_wipe), &empty);
}

void
mwr_render_draw(const mw_rat_t *r)
{
	HackMazeBitmap(Loc(r->mwr_x_pos), Loc(r->mwr_y_pos),
	               &normalArrows[r->mwr_dir]);
}
