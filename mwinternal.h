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

typedef uint64_t mw_seqno_t;

typedef struct mw_state {
	struct sockaddr   *mws_mcast_addr;
	int                mws_mcast_socket;

	struct list_head   mws_rats;
	mw_guid_t          mws_local_rat_id;

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
int  mwm_set_xpos(mw_missile_t *m, mw_pos_t x);
int  mwm_set_ypos(mw_missile_t *m, mw_pos_t y);
int  mwm_set_dir(mw_missile_t *m, mw_dir_t dir);
void mwm_get_xpos(mw_missile_t *m, mw_pos_t *x);
void mwm_get_ypos(mw_missile_t *m, mw_pos_t *y);
void mwm_get_packed_posdir(mw_missile_t *m, uint32_t *posdir);
int  mwm_is_occupying_cell(mw_missile_t *m, mw_pos_t x, mw_pos_t y);

typedef struct mw_rat {
	struct list_head  mwr_list;
	struct list_head  mwr_tagged_pkt_list;

	mw_guid_t         mwr_id;
	int               mwr_mw_index;
	mw_pos_t          mwr_x_pos;
	mw_pos_t          mwr_y_pos;
	mw_dir_t          mwr_dir;
	char             *mwr_name;
	mw_missile_t     *mwr_missile;
	mw_score_t        mwr_score;

	struct sockaddr  *mwr_mcast_addr;
	int               mwr_mcast_socket;

	mw_seqno_t        mwr_pkt_seqno;

	struct timeval    mwr_state_pkt_timeout;
	struct timeval    mwr_name_pkt_timeout;
	struct timeval    mwr_lasttime;

	mw_pos_t          mwr_x_wipe;
	mw_pos_t          mwr_y_wipe;

	int               mwr_is_local;
} mw_rat_t;

/* Mazewar Rat Constructor
 * Add a missile to the state
 * @id   : Unique ID number of the added rat
 * @x    : Starting x position of rat
 * @y    : Starting y position of rat
 * @dir  : Starting direction of rat
 * @name : Starting name of the rat
 */
int  mwr_cons(mw_rat_t **r, mw_guid_t *id,
              mw_pos_t x, mw_pos_t y, mw_dir_t dir,
              const char *name);
int  mwr_dest(mw_rat_t *r);
void mwr_render_wipe(const mw_rat_t *r);
void mwr_render_draw(const mw_rat_t *r);
int  mwr_cmp_id(mw_rat_t *r, mw_guid_t id);
int  mwr_set_xpos(mw_rat_t *r, mw_pos_t x);
int  mwr_set_ypos(mw_rat_t *r, mw_pos_t y);
int  mwr_set_dir(mw_rat_t *r, mw_dir_t dir);
int  mwr_set_missile_packed_posdir(mw_rat_t *r, uint32_t posdir);
int  mwr_set_score(mw_rat_t *r, mw_score_t score);
int  mwr_increment_score(mw_rat_t *r, int increment);
int  mwr_rm_missile(mw_rat_t *r);
int  mwr_set_is_local_flag(mw_rat_t *r, int is_local);
int  mwr_set_id(mw_rat_t *r, mw_guid_t guid);
int  mwr_set_name(mw_rat_t *r, const char *name);
int  mwr_get_xpos(mw_rat_t *r, mw_pos_t *x);
int  mwr_get_ypos(mw_rat_t *r, mw_pos_t *y);
int  mwr_get_score(mw_rat_t *r, mw_score_t *score);
int  mwr_get_id(mw_rat_t *r, mw_guid_t *id);
int  mwr_is_occupying_cell(mw_rat_t *r, mw_pos_t x, mw_pos_t y);
int  mwr_missile_is_occupying_cell(mw_rat_t *r, mw_pos_t x, mw_pos_t y);
int  mwr_fire_missile(mw_rat_t *r, int **maze);
int  mwr_tagged_by(mw_rat_t *r, mw_guid_t tagger_id);
int  mwr_tagged(mw_rat_t *r, mw_guid_t taggee_id, mw_seqno_t pkt_seqno);
void mwr_update(mw_rat_t *r, int **maze);
void mwr_set_addr(mw_rat_t *r, struct sockaddr *mcast, int socket);
int  mwr_send_state_pkt(mw_rat_t *r);
int  mwr_send_name_pkt(mw_rat_t *r);
int  mwr_send_leaving_pkt(mw_rat_t *r);
int  mwr_send_tagged_pkt(mw_rat_t *r, mw_guid_t shooter_id);
int  mwr_send_ack_pkt(mw_rat_t *r, mw_guid_t ack_id, mw_seqno_t ack_seqno);

#define MW_PKT_HDR_DESCRIPTOR_STATE    0
#define MW_PKT_HDR_DESCRIPTOR_NICKNAME 1
#define MW_PKT_HDR_DESCRIPTOR_TAGGED   2
#define MW_PKT_HDR_DESCRIPTOR_ACK      3
#define MW_PKT_HDR_DESCRIPTOR_LEAVING  4

typedef struct mw_pkt_header {
	uint8_t    mwph_descriptor;
	uint8_t    mwph_mbz[3];
	uint64_t   mwph_guid;
	uint64_t   mwph_seqno;
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
	uint64_t        mwps_crt;

	/* Pad to total size of 64-bytes */
	uint8_t        mwps_mbz[24];
} mw_pkt_state_t;

#define MW_NICKNAME_LEN 32
typedef struct mw_pkt_nickname {
	mw_pkt_header_t mwpn_header;
	uint8_t         mwpn_nickname[MW_NICKNAME_LEN];

	/* Pad to total size of 64-bytes */
	uint8_t mwpn_mbz[12];
} mw_pkt_nickname_t;

typedef struct mw_pkt_tagged {
	mw_pkt_header_t mwpt_header;
	uint64_t        mwpt_shooter_guid;

	/* Pad to total size of 64-bytes */
	uint8_t mwpt_mbz[36];
} mw_pkt_tagged_t;

typedef struct mw_pkt_ack {
	mw_pkt_header_t mwpa_header;
	uint64_t        mwpa_guid;
	uint64_t        mwpa_seqno;

	/* Pad to total size of 64-bytes */
	uint8_t mwpa_mbz[28];
} mw_pkt_ack_t;

typedef struct mw_pkt_leaving {
	mw_pkt_header_t mwpl_header;
	uint64_t        mwpl_leaving_guid;

	/* Pad to total size of 64-bytes */
	uint8_t mwpl_mbz[36];
} mw_pkt_leaving_t;

int      mw_timeval_timeout_triggered(const struct timeval *timeout);
uint64_t mw_rand(void);
void     mw_print_pkt_header(const mw_pkt_header_t *pkt);
void     mw_print_pkt_state(const mw_pkt_state_t *pkt);
void     mw_print_pkt_nickname(const mw_pkt_nickname_t *pkt);
void     mw_print_pkt_leaving(const mw_pkt_leaving_t *pkt);
void     mw_print_pkt_tagged(const mw_pkt_tagged_t *pkt);
void     mw_print_pkt_ack(const mw_pkt_ack_t *pkt);
void     mw_posdir_pack(uint32_t *posdir, mw_pos_t x, mw_pos_t y,
                        mw_dir_t dir);
void     mw_posdir_unpack(uint32_t posdir, mw_pos_t *x, mw_pos_t *y,
                          mw_dir_t *dir);
void     mw_timeval_sum(struct timeval *diff,
                        const struct timeval *x,
                        const struct timeval *y);
void     mw_timeval_difference(struct timeval *diff,
                               const struct timeval *x,
                               const struct timeval *y);

#endif /* _MW_INTERNAL_H */

/* vim: set tabstop=8 shiftwidth=8 noexpandtab: */
