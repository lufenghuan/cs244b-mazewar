/*
 *   FILE: toplevel.c
 * AUTHOR: name (email)
 *   DATE: March 24 23:59:59 PST 2009
 *  DESCR:
 */

/* #define DEBUG */

#include "main.h"
#include <string>
#include "mazewar.h"

static bool		updateView;	/* true if update needed */
MazewarInstance::Ptr M;

/* Use this socket address to send packets to the multi-cast group. */
static Sockaddr         groupAddr;
#define MAX_OTHER_RATS  (MAX_RATS - 1)


int main(int argc, char *argv[])
{
    Loc x(1);
    Loc y(5);
    Direction dir(0);
    char        *ratName, *mazeName;

	signal(SIGHUP, quit);
	signal(SIGINT, quit);
	signal(SIGTERM, quit);

    getName("Welcome to CS244B MazeWar!\n\nYour Name", &ratName);
	getString("Maze Name", &mazeName);
	M = MazewarInstance::mazewarInstanceNew(string(ratName));
	MazewarInstance* a = M.ptr();
	strncpy(M->myName_, ratName, NAMESIZE);
    free(ratName);

    /* since this sample only deals with a local version, we ignore
       mazeName for now.  We also don't need any calls to getHostName,
       because we're not going out over the net for anything.
     */
        free(mazeName);

	MazeInit(argc, argv);

	mws_add_rat(M->state, &M->local_id, MY_X_LOC, MY_Y_LOC, MY_MW_DIR_T,
	                      M->myName_);
	mws_set_local_rat(M->state, M->local_id);

	/* XXX: This is called in MazeInit, is it needed here too?
	 *      Disabling until further understood why it's here.
     NewPosition(M);
	 */

 	/* So you can see what a Rat is supposed to look like, we create
 	   one rat at the following location: (1,5)
 	   It doesn't move, you can't shoot it, you can just walk around it */
	/* XXX: This appears to be for demonstration purposes only. As such,
	 * it should be removed as the game's implementation progresses */
        SetRatPosition(1, x, y, dir);

	play();

	return (0);
}


/* ----------------------------------------------------------------------- */

void
play(void)
{
	MWEvent		event;
	MW244BPacket	incoming;

	event.eventDetail = &incoming;

	while (TRUE) {
		NextEvent(&event, M->theSocket());
		if (!M->peeking())
			switch(event.eventType) {
			case EVENT_A:
				aboutFace();
				break;

			case EVENT_S:
				leftTurn();
				break;

			case EVENT_D:
				forward();
				break;

			case EVENT_F:
				rightTurn();
				break;

			case EVENT_BAR:
				backward();
				break;

			case EVENT_LEFT_D:
				peekLeft();
				break;

			case EVENT_MIDDLE_D:
				shoot();
				break;

			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;

			case EVENT_INT:
				quit(0);
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;
			}

		ratStates();		/* clean house */

		manageMissiles();

		DoViewUpdate();

		mws_update(M->state);

		/* Any info to send over network? */

	}
}

/* ----------------------------------------------------------------------- */

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

void
aboutFace(void)
{
	M->dirIs(_aboutFace[MY_DIR]);
	mws_set_rat_dir(M->state, M->local_id, MY_MW_DIR_T);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
leftTurn(void)
{
	M->dirIs(_leftTurn[MY_DIR]);
	mws_set_rat_dir(M->state, M->local_id, MY_MW_DIR_T);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
rightTurn(void)
{
	M->dirIs(_rightTurn[MY_DIR]);
	mws_set_rat_dir(M->state, M->local_id, MY_MW_DIR_T);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

/* remember ... "North" is to the right ... positive X motion */

void
forward(void)
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case SOUTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case EAST:	if (!M->maze_[tx][ty+1])	ty++; break;
	case WEST:	if (!M->maze_[tx][ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}

	mws_set_rat_xpos(M->state, M->local_id, MY_X_LOC);
	mws_set_rat_ypos(M->state, M->local_id, MY_Y_LOC);
}

/* ----------------------------------------------------------------------- */

void backward()
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case SOUTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case EAST:	if (!M->maze_[tx][ty-1])	ty--; break;
	case WEST:	if (!M->maze_[tx][ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}

	mws_set_rat_xpos(M->state, M->local_id, MY_X_LOC);
	mws_set_rat_ypos(M->state, M->local_id, MY_Y_LOC);
}

/* ----------------------------------------------------------------------- */

void peekLeft()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(WEST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(EAST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(NORTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekRight()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(EAST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(WEST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(NORTH);
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekStop()
{
	M->peekingIs(FALSE);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void shoot()
{
	mw_score_t score;
	mws_fire_missile(M->state, M->local_id);
	/* XXX: Should check return code in case of error. */
	mws_get_rat_score(M->state, M->local_id, &score);

	M->scoreIs(score);
	UpdateScoreCard(M->myRatId().value());
}

/* ----------------------------------------------------------------------- */

/*
 * Exit from game, clean up window
 */

void quit(int sig)
{

	StopWindow();
	exit(0);
}


/* ----------------------------------------------------------------------- */

void NewPosition(MazewarInstance::Ptr m)
//void NewPosition(MazewarInstance *m)
{
	Loc newX(0);
	Loc newY(0);
	Direction dir(0); /* start on occupied square */

	while (M->maze_[newX.value()][newY.value()]) {
	  /* MAZE[XY]MAX is a power of 2 */
	  newX = Loc(random() & (MAZEXMAX - 1));
	  newY = Loc(random() & (MAZEYMAX - 1));

	  /* In real game, also check that square is
	     unoccupied by another rat */
	}

	/* prevent a blank wall at first glimpse */

	if (!m->maze_[(newX.value())+1][(newY.value())]) dir = Direction(NORTH);
	if (!m->maze_[(newX.value())-1][(newY.value())]) dir = Direction(SOUTH);
	if (!m->maze_[(newX.value())][(newY.value())+1]) dir = Direction(EAST);
	if (!m->maze_[(newX.value())][(newY.value())-1]) dir = Direction(WEST);

	m->xlocIs(newX);
	m->ylocIs(newY);
	m->dirIs(dir);
}

/* ----------------------------------------------------------------------- */

void MWError(char *s)

{
	StopWindow();
	fprintf(stderr, "CS244BMazeWar: %s\n", s);
	perror("CS244BMazeWar");
	exit(-1);
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
Score GetRatScore(RatIndexType ratId)
{
  if (ratId.value() == 	M->myRatId().value())
    { return(M->score()); }
  else { return (0); }
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
char *GetRatName(RatIndexType ratId)
{
  if (ratId.value() ==	M->myRatId().value())
    { return(M->myName_); }
  else { return ("Dummy"); }
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ConvertIncoming(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ConvertOutgoing(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ratStates()
{
  /* In our sample version, we don't know about the state of any rats over
     the net, so this is a no-op */
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void manageMissiles()
{
  /* Leave this up to you. */
}

/* ----------------------------------------------------------------------- */

void DoViewUpdate()
{
	mws_render_wipe(M->state);
	mws_render_draw(M->state);

	/* XXX: Hack. I don't want to deal with setting updateView in
	 *      the mws_* methods, thus just always update view, even
	 *      if it is unnecessary.
	 */
	updateView = TRUE;

	if (updateView) {	/* paint the screen */


		/* XXX: I don't think this is necessary anymore as the
		 *      mws_render_* functions take care of it.
		ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
		 */
		if (M->peeking())
			ShowView(M->xPeek(), M->yPeek(), M->dirPeek());
		else
			ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR);
		updateView = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Sample code to send a packet to a specific destination
 */

/*
 * Notice the call to ConvertOutgoing.  You must call ConvertOutgoing before
 * any call to sendto.
 */

void sendPacketToPlayer(RatId ratId)
{
/*
	MW244BPacket		pack;
	DataStructureX		*packX;

	pack.type = PACKET_TYPE_X;
	packX = (DataStructureX *) &pack.body;
	packX->foo = d1;
	packX->bar = d2;

        ....

	ConvertOutgoing(&pack);

	if (sendto((int)M->theSocket(), &pack, sizeof(pack), 0,
		   (struct sockaddr*) &groupAddr, sizeof(Sockaddr)) < 0)
	  { MWError("Sample error"); }
*/
}

/* ----------------------------------------------------------------------- */

/* Sample of processPacket.  Remember, ConvertIncoming() has already
 * been called on the packet.
 */

void processPacket (MWEvent *eventPacket)
{
	mws_receive_pkt(M->state,
	                (mw_pkt_header_t *)eventPacket->eventDetail);
}

/* ----------------------------------------------------------------------- */

/* This will presumably be modified by you.
   It is here to provide an example of how to open a UDP port.
   You might choose to use a different strategy
 */
void
netInit()
{
	bool		joinGame();
	Sockaddr	nullAddr;
	Sockaddr	*thisHost;
	char		buf[128];
	int		reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	/* MAZEPORT will be assigned by the TA to each team */
	M->mazePortIs(htons(MAZEPORT));

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	M->theSocketIs(socket(AF_INET, SOCK_DGRAM, 0));
	if (M->theSocket() < 0)
	  MWError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(M->theSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		MWError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = M->mazePort();
	if (bind(M->theSocket(), (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
	  MWError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 32;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* uncomment the following if you do not want to receive messages that
	   you sent out - of course, you cannot have multiple players on the
	   same machine if you do this */
#if 0
	{
	  u_char loop = 0;
	  if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
	             sizeof(loop)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_LOOP)");
	  }
	}
#endif

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(MAZEGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		MWError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */

	/* I haven't implemented joinGame or startGame in this sample */
	/*
	if (!joinGame())
		startGame();
        */
	printf("\n");

	/* set up some stuff strictly for this local sample */
	M->myRatIdIs(0);
	M->scoreIs(0);
	SetMyRatIndexType(0);

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(MAZEGROUP);

	mws_set_addr(M->state, (struct sockaddr *)&groupAddr, M->theSocket());
}


/* ----------------------------------------------------------------------- */
