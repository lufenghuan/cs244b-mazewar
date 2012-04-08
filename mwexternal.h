/*
 * mwexternal.h - Mazewar's external header file
 */

#ifndef _MW_EXTERNAL_H
#define _MW_EXTERNAL_H

#include <stdint.h>

typedef uint8_t  mw_pos_t;
typedef uint32_t mw_missile_id_t;

/* XXX: These were copied from mazewar.h
 *      I'd rather include that file, than this ugly hack, but
 *      mazewar.h's dependencies are fucked and I want to avoid
 *      including all of it's dependencies for these simple values.
 */
#define	NDIRECTION	4
#define	NORTH		0
#define	SOUTH		1
#define	EAST		2
#define	WEST		3

typedef enum {
	MW_DIR_NORTH = NORTH,
	MW_DIR_SOUTH = SOUTH,
	MW_DIR_EAST  = EAST,
	MW_DIR_WEST  = WEST,
	MW_DIR_NDIR  = NDIRECTION,
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

#endif /* _MW_EXTERNAL_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
