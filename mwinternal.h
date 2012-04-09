/*
 * mwinternal.h - Mazewar's internal header file
 */

#ifndef _MW_INTERNAL_H
#define _MW_INTERNAL_H

#include "list.h"
#include "mwexternal.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#define ASSERT(x) assert((x))

typedef struct mw_state {
	struct list_head mws_missiles;
} mw_state_t;

typedef struct mw_missile {
	struct list_head mwm_list;
	mw_missile_id_t  mwm_id;
	mw_pos_t         mwm_x_pos;
	mw_pos_t         mwm_y_pos;
	mw_dir_t         mwm_dir;

	struct timeval   mwm_timeout;
	struct timeval   mwm_lasttime;

	mw_pos_t         mwm_x_wipe;
	mw_pos_t         mwm_y_wipe;
} mw_missile_t;

/* Mazewar Missile Constructor
 * @id  : Unique ID number of the added missile
 * @x   : Starting x position of missile
 * @y   : Starting y position of missile
 * @dir : Starting direction of missile
 */
int mwm_cons(mw_missile_t **m, mw_missile_id_t *id,
             mw_pos_t x, mw_pos_t y, mw_dir_t dir);

/* Mazewar Missile Destructor */
int mwm_dest(mw_missile_t *m);

void mwm_render_wipe(const mw_missile_t *m);
void mwm_render_draw(const mw_missile_t *m);
void mwm_update(mw_missile_t *m);

#endif /* _MW_INTERNAL_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
