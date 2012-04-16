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

/* For rendering functions, direction macros, etc. */
#include "mazewar.h"

/* XXX: Can't '#include "display.h"' because of "multiple definition"
 *      errors. Ideally, the header files should only provide 'extern'
 *      declarations and not actual definitions. To get around this,
 *      since I need access to a couple variables from that file, I
 *      removed the static modifiers for those variables, and add extern
 *      declarations here. Sigh..
 */
extern BitCell normalArrows[NDIRECTION];
extern BitCell missile[1];

#define ASSERT(x) assert((x))

typedef enum {
	MWS_PHASE_DISCOVERY,
	MWS_PHASE_ACTIVE
} mws_phase_t;

typedef struct mw_state {
	struct sockaddr   *mws_mcast_addr;
	int                mws_mcast_socket;

	struct list_head   mws_missiles;
	struct list_head   mws_rats;

	mws_phase_t        mws_phase;
	struct timeval     mws_elapsedtime;
	struct timeval     mws_lasttime;

	int              **mws_maze;
	int                mws_xmax;
	int                mws_ymax;
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
int  mwm_cons(mw_missile_t **m, mw_missile_id_t *id,
              mw_pos_t x, mw_pos_t y, mw_dir_t dir);
int  mwm_dest(mw_missile_t *m);
void mwm_render_wipe(const mw_missile_t *m);
void mwm_render_draw(const mw_missile_t *m);
void mwm_update(mw_missile_t *m);
void  mwm_get_xpos(mw_missile_t *m, mw_pos_t *xpos);
void  mwm_get_ypos(mw_missile_t *m, mw_pos_t *ypos);

typedef struct mw_rat {
	struct list_head  mwr_list;
	mw_rat_id_t       mwr_id;
	mw_pos_t          mwr_x_pos;
	mw_pos_t          mwr_y_pos;
	mw_dir_t          mwr_dir;
	char             *mwr_name;

	mw_pos_t         mwr_x_wipe;
	mw_pos_t         mwr_y_wipe;
} mw_rat_t;

/* Mazewar Rat Constructor
 * Add a missile to the state
 * @id   : Unique ID number of the added rat
 * @x    : Starting x position of rat
 * @y    : Starting y position of rat
 * @dir  : Starting direction of rat
 * @name : Starting name of the rat
 */
int  mwr_cons(mw_rat_t **r, mw_rat_id_t *id,
              mw_pos_t x, mw_pos_t y, mw_dir_t dir,
              const char *name);
int  mwr_dest(mw_rat_t *r);
void mwr_render_wipe(const mw_rat_t *r);
void mwr_render_draw(const mw_rat_t *r);
int  mwr_cmp_id(mw_rat_t *r, mw_rat_id_t id);
int  mwr_set_xpos(mw_rat_t *r, mw_pos_t x);
int  mwr_set_ypos(mw_rat_t *r, mw_pos_t y);
int  mwr_set_dir(mw_rat_t *r, mw_dir_t dir);

typedef struct mw_pkt_header {
	uint8_t    mwph_descriptor;
	uint8_t    mwph_mbz[3];
	uint64_t   mwph_guid;
	uint64_t   mwph_seqnum;
} mw_pkt_header_t;

typedef struct mw_pkt_state {
	mw_pkt_header_t mwps_header;

	/* Bits:  0 - 14 are for the rat's x position
	 * Bits: 15 - 29 are for the rat's y position
	 * Bits: 30 & 31 are for the rat's direction
	 */
	uint32_t        mwps_rat_posdir;

	/* Bits:  0 - 14 are for the missile's x position
	 * Bits: 15 - 29 are for the missile's y position
	 * Bits: 30 & 31 are for the missile's direction
	 */
	uint32_t        mwps_missile_posdir;

	uint32_t        mwps_score;
	uint64_t        mwps_timestamp;
	uint64_t        mwps_crt;
} mw_pkt_state_t;

typedef struct mw_pkt_nickname {
	mw_pkt_header_t mwpn_header;
	uint8_t         mwpn_nickname[32];
} mw_pkt_nickname_t;

typedef struct mw_pkt_tagged {
	mw_pkt_header_t mwpt_header;
	uint64_t        mwpt_shooter_guid;
} mw_pkt_tagged_t;

typedef struct mw_pkt_ack {
	mw_pkt_header_t mwpa_header;
	uint64_t        mwpa_guid;
	uint64_t        mwpa_seqnum;
} mw_pkt_ack_t;

typedef struct mw_pkt_leaving {
	mw_pkt_header_t mwpl_header;
	uint64_t        mwpl_leaving_guid;
} mw_pkt_leaving_t;

void mw_timeval_difference(struct timeval *diff, const struct timeval *x,
                           const struct timeval *y);

void mw_timeval_sum(struct timeval *diff, const struct timeval *x,
                    const struct timeval *y);

#endif /* _MW_INTERNAL_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
