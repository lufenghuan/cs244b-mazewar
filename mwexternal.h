/*
 * mwexternal.h - Mazewar's external header file
 */

#ifndef _MW_EXTERNAL_H
#define _MW_EXTERNAL_H

#include <stdint.h>

typedef uint8_t  mw_pos_t;
typedef uint32_t mw_missile_id_t;
typedef uint32_t mw_rat_id_t;

typedef enum {
	MW_DIR_NORTH,
	MW_DIR_SOUTH,
	MW_DIR_EAST,
	MW_DIR_WEST,
	MW_DIR_NDIR
} mw_dir_t;

typedef struct mw_state mw_state_t;

/* Mazewar State Constructor */
int mws_cons(mw_state_t **s);

/* Mazeware State Destructor */
int mws_dest(mw_state_t *s);

/* Add a missile to the state
 * @id  : Unique ID number of the added missile
 * @x   : Starting x position of missile
 * @y   : Starting y position of missile
 * @dir : Starting direction of missile
 */
int mws_add_missile(mw_state_t *s, mw_missile_id_t *id,
                    mw_pos_t x, mw_pos_t y, mw_dir_t dir);

/* Add a missile to the state
 * @id   : Unique ID number of the added rat
 * @x    : Starting x position of rat
 * @y    : Starting y position of rat
 * @dir  : Starting direction of rat
 * @name : Starting name of the rat
 */
int mws_add_rat(mw_state_t *s, mw_rat_id_t *id,
                mw_pos_t x, mw_pos_t y, mw_dir_t dir,
                const char *name);

void mws_render_wipe(const mw_state_t *s);
void mws_render_draw(const mw_state_t *s);

void mws_update(mw_state_t *s);

#endif /* _MW_EXTERNAL_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
