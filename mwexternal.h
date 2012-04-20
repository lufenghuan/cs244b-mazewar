/*
 * mwexternal.h - Mazewar's external header file
 */

#ifndef _MW_EXTERNAL_H
#define _MW_EXTERNAL_H

#include <stdint.h>

typedef uint8_t  mw_pos_t;
typedef uint32_t mw_missile_id_t;
typedef uint64_t mw_guid_t;
typedef int32_t  mw_score_t;

typedef enum {
	MW_DIR_NORTH,
	MW_DIR_SOUTH,
	MW_DIR_EAST,
	MW_DIR_WEST,
	MW_DIR_NDIR
} mw_dir_t;

typedef struct mw_state mw_state_t;
typedef struct mw_pkt_header mw_pkt_header_t;

/* Mazewar State Constructor */
int mws_cons(mw_state_t **s);

/* Mazewar State Destructor */
int mws_dest(mw_state_t *s);

void mws_set_maze(mw_state_t *s, int **maze, int xmax, int ymax);
void mws_set_addr(mw_state_t *s, struct sockaddr *mcast, int socket);

/* Add a missile to the state
 * @id   : Unique ID number of the added rat
 * @x    : Starting x position of rat
 * @y    : Starting y position of rat
 * @dir  : Starting direction of rat
 * @name : Starting name of the rat
 */
int mws_add_rat(mw_state_t *s, mw_guid_t *id,
                mw_pos_t x, mw_pos_t y, mw_dir_t dir,
                const char *name);

void mws_render_wipe(const mw_state_t *s);
void mws_render_draw(const mw_state_t *s);

void mws_update(mw_state_t *s);

int mws_set_rat_xpos(mw_state_t *s, mw_guid_t id, mw_pos_t x);
int mws_set_rat_ypos(mw_state_t *s, mw_guid_t id, mw_pos_t y);
int mws_set_rat_dir(mw_state_t *s, mw_guid_t id, mw_dir_t dir);
int mws_set_rat_send_pkts_flag(mw_state_t *s, mw_guid_t id, int send_pkts);
int mws_get_rat_score(mw_state_t *s, mw_guid_t id, mw_score_t *score);
int mws_fire_missile(mw_state_t *s, mw_guid_t id);

void mws_receive_pkt(mw_state_t *s, mw_pkt_header_t *pkt);

#endif /* _MW_EXTERNAL_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
