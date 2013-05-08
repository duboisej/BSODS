#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/maze.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"


#define STRLEN 81
#define BUFLEN 16384
#define XSTR(s) STR(s)
#define STR(s) #s

int playernum;

struct LineBuffer {
  char data[BUFLEN];
  int  len;
  int  newline;
};

struct Globals {
  struct LineBuffer in;
  char host[STRLEN];
  PortType port;
  int connected;
  int playernum;
} globals;


typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

// represent game board..
Cell maze[201][201]; // store cell type and playernum if a player is there

Player players[300]; // Player structs to store necessary info about players.

int numPlayers; // global int to store number of players currently connected. Always updated via update from the server.
int hammers[2][2]; // store hammer locations.
int flags[2][2]; // store flag locations.


int 
getInput()
{
  int len;
  char *ret;

  // to make debugging easier we zero the data of the buffer
  bzero(globals.in.data, sizeof(globals.in.data));
  globals.in.newline = 0;

  ret = fgets(globals.in.data, sizeof(globals.in.data), stdin);
  // remove newline if it exists
  len = (ret != NULL) ? strlen(globals.in.data) : 0;
  if (len && globals.in.data[len-1] == '\n') {
    globals.in.data[len-1]=0;
    globals.in.newline=1;
  } 
  globals.in.len = len;
  return len;
}

void
printMenu(void)
{
  printf("\nYour command options are as follows: \n");
  printf("\n'connect <ip:port>':\n\tconnects you to the server with the corresponding IP address and port number. If you are already connected to a server, the command will do nothing. Example: connect 127.0.0.1:38500\n");
  printf("\n'dump':\n\tprints the map.\n");
  printf("\n'w':\n\tmoves player up.\n");
  printf("\n'a':\n\tmoves player left.\n");
  printf("\n's':\n\tmoves player down.\n");
  printf("\n'd':\n\tmoves player right.\n");
  printf("\n'f':\n\tdrops flag (if carrying).\n");
  printf("\n'disconnect':\n\tdisconnects you from the server you are currently connected to. If not connected to a server, the command will do nothing.\n");
  printf("\n'where':\n\tdisplays the <ip:port> of the server you are currently connected to. If not connected to a server, the command will do nothing.\n");
  printf("\n'quit':\n\tquits the Tic-Tac-Toe client. This assumes a disconnection from the server.\n");
  printf("\n'help':\n\tredisplays this menu.\n");
}

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));

  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  } 
  return 1;
}


static int
update_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


int 
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if (proto_client_connect(C->ph, host, port)!=0) {
      fprintf(stderr, "failed to connect\n");
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
				     h);
    }
#endif
    return 1;
  }
  return 0;
}


int
prompt(int menu) 
{
  static char MenuString[] = "\n?> ";
  int ret;
  int len=0;

  if (menu) printf("%s", MenuString);
  fflush(stdout);
  len = getInput();
  return len;
}

int
doConnect(Client *C) {
  int i, len = strlen(globals.in.data);
  Proto_Session *s;

  if (globals.connected==1) {
    fprintf(stderr, "Already connected to server.\n");
  } else {
    
    for (i=0; i<len; i++) if (globals.in.data[i]==':') globals.in.data[i]=' ';
    sscanf(globals.in.data, "%*s %" XSTR(STRLEN) "s %d", globals.host,
	   &globals.port);
    
    if (strlen(globals.host)==0 || globals.port==0) {
      fprintf(stderr, "Missing server or port.\n");
    } else {
      if (startConnection(C, globals.host, globals.port, update_event_handler)<0) {
    	  fprintf(stderr, "ERROR: startConnection failed\n");
          return -1;
      } else {
		    globals.connected=1;
		    if (proto_client_hello(C->ph) > 0)
        {
            s = proto_client_rpc_session(C->ph);
            //proto_session_dump(s);
            int temp;
            proto_session_body_unmarshall_int(s, 0, &temp);
            globals.playernum = temp;
            printf("Connected to %s:%d: You are player number %d", globals.host, globals.port, globals.playernum);
        } 
        else
        {
          fprintf(stderr, "No reply from server.\n");
          return -1;
        }

      }
    }
  }
  return 1;
}

int
doWhere(void) {
    if(globals.connected == 0) {
    	printf("Not connected to a server.\n");
    } else {
    	printf("Connected to server at IP %s on port %d.",
    		globals.host, globals.port);
    }
    return 1;
}


int
doMove(Client* C, char move)
{
  int rc;
  rc = proto_client_move(C->ph, move, globals.playernum);
  // Handle replies
  Proto_Session *s = proto_client_rpc_session(C->ph);
  proto_session_hdr_unmarshall(s, &(s->rhdr));
  char replycode = s->rbuf[0];
  //printf("got back replycode %c\n", replycode);
  if (replycode == 'I')
  {
    printf("Sorry. Invalid move.\n");
  }
  else if (replycode == 'w')
  {
    printf("Team 1 won!!!!\n");
    doDisconnect(C);
  }
  else if (replycode == 'W')
  {
    printf("Team 2 won!!!!\n");
    doDisconnect(C);
  }
  return rc;

}

int
doDisconnect(Client* C)
{
  if (globals.connected == 0)
  {
    printf("You are not connected to a server.\n");
    return 1;
  }

	//Close both sockets (fd's)
  printf("Game Over.\n");
	Proto_Session* srpc = proto_client_rpc_session(C->ph);
	Proto_Session* sevent = proto_client_event_session(C->ph);
	fprintf(stderr, "RPC fd = %d\n", srpc->fd);
	fprintf(stderr, "Event fd = %d\n", sevent->fd);
	close(srpc->fd);
	close(sevent->fd);
	fprintf(stderr, "Disconnecting from server.\n");
	globals.connected = 0;
	printMenu();
	return 1;
}

int 
docmd(Client *C)
{

  // Use kbhit to track incoming keystrokes
  int i = 0;
  int rc = 1;
  printf("You entered: %s\n", globals.in.data);
  if (strlen(globals.in.data)==0) return rc; //rc = doReprint();
  else if (strncmp(globals.in.data, "connect", sizeof("connect")-1)==0) rc = doConnect(C);
  else if (strncmp(globals.in.data, "disconnect", sizeof("disconnect")-1)==0) rc = doDisconnect(C);
  else if (strncmp(globals.in.data, "dump", sizeof("dump")-1)==0) rc = doDump(C);
  else if (strncmp(globals.in.data, "w", sizeof("w")-1)==0) rc = doMove(C, 'w');
  else if (strncmp(globals.in.data, "s", sizeof("s")-1)==0) rc = doMove(C, 's');
  else if (strncmp(globals.in.data, "a", sizeof("a")-1)==0) rc = doMove(C, 'a');
  else if (strncmp(globals.in.data, "d", sizeof("d")-1)==0) rc = doMove(C, 'd');
  else if (strncmp(globals.in.data, "f", sizeof("f")-1)==0) rc = doMove(C, 'f');
  else if (strncmp(globals.in.data, "quit", 
		   sizeof("quit")-1)==0) rc = doQuit();
  else  printf("Unknown Command\n");
  
  return rc;
}

int doQuit(Client *C)
{
  
  printf("You Quit.\n");
  
  
  return -1;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char c;
  int rc;
  int menu=1;

  while (1) {
    if ((prompt(menu))!=0) rc=docmd(C);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void 
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
	   "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
	   "examples:\n" 
           " %s 12345 : starts client connecting to localhost:12345\n"
	  " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
	   pgm, pgm, pgm, pgm);
 
}

void
initGlobals(void)
{
  bzero(&globals, sizeof(globals));
  globals.host[0]=0;
  globals.port=0;
  globals.connected=0;
}


// updateMap - receives event update from server, parses game information, updates data structures
// and dumps map to standard output in ASCII form.
static int
updateMap(Proto_Session *s)
{
  int rc;
  //printf("Got into updateMap.\n");
  //proto_session_dump(s);
  int offset = 0;

  int win;
  offset = proto_session_body_unmarshall_int(s, offset, &win);
  if (win == 1)
  {
    printf("Team 1 won.\n");
    exit(0);
  }
  else if (win == 2)
  {
    printf("Team 2 won.\n");
    exit(0);
  }

  // Get number of players (to read in players)

  offset = proto_session_body_unmarshall_int(s, offset, &numPlayers);

  //printf("There are %d players, read from buffer\n", numPlayers);
  // Load players

  Player tempPlayers[numPlayers+1];
  int tempHammers[2][2];
  int tempFlags[2][2];

  int m;
  for (m = 1; m < numPlayers+1; m++)
  {
    if (offset != -1)
    {
      offset = proto_session_body_unmarshall_player(s, offset, &(tempPlayers[m]));
    }
    else
    {
      return -1;
    }
  }

  // Load flag locations
  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempFlags[0][0]));
  }
  else
  {
    return -1;
  }

  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempFlags[0][1]));
  }
  else
  {
    return -1;
  }

  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempFlags[1][0]));
  }
  else
  {
    return -1;
  }

  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempFlags[1][1]));
  }
  else
  {
    return -1;
  }

  // Load hammer locations
   if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempHammers[0][0]));
  }
  else
  {
    return -1;
  }

  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempHammers[0][1]));
  }
  else
  {
    return -1;
  }

  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempHammers[1][0]));
  }
  else
  {
    return -1;
  }

  if (offset != -1)
  {
    offset = proto_session_body_unmarshall_int(s, offset, &(tempHammers[1][1]));
  }
  else
  {
    return -1;
  }

  // Check if flags have moved. If so, update maze.
  int p;
  int q;
  for (p = 0; p < 2; p++)
  {
    for (q = 0; q < 2; q++)
    {
      if (flags[p][q] != tempFlags[p][q])
      {
        maze[(flags[p][0])][(flags[p][1])].flag = 0; // remove flag from old location on map
        flags[p][q] = tempFlags[p][q]; // copy over new location 
      }
    }
  }

  // Check if hammers have moved. If so, update maze. 
  p = 0;
  q = 0;
  for (p = 0; p < 2; p++)
  {
    for (q = 0; q < 2; q++)
    {
      if (hammers[p][q] != tempHammers[p][q])
      {
        maze[(hammers[p][0])][(hammers[p][1])].mjolnir.hammerID = 0; // remove hammer from old location on map
        hammers[p][q] = tempHammers[p][q]; // copy over new location 
      }
    }
  }


  // Dump temp player information - for debugging purposes

    // printf("Printing Temp Player information:\n");
    // printf("There are %d current players.\n", numPlayers);
    // int b;
    // for (b = 1; b < numPlayers+1; b++)
    // {
    //   printf("Player %d:\n", b);
    //   Player *p = &(tempPlayers[b]);
    //   printf("\tPlayer number is %d\n", p->playernum);
    //   printf("\tPlayer is on team number %d\n", p->team);
    //   printf("\tLocation: %d,%d\n", p->location.x, p->location.y);
    //   Hammer *h = &(p->mjolnir);
    //   if (h->hammerID != 0)
    //   {
    //     printf("\tCarrying Hammer %d with %d uses left.\n", h->hammerID, h->uses);
    //   }
    //   if (p->flag != 0)
    //   {
    //     printf("\tCarrying Flag %d\n", p->flag);
    //   }
    //   printf("\n");
      
    // }

  // Update map representation from player info
  int a;
  for (a = 1; a < numPlayers+1; a++)
  {
    Player *player = &(players[a]);
    Player *temp_player = &(tempPlayers[a]);
    Point *location = &(player->location);
    Point *temp_location = &(temp_player->location);
    int oldx = location->x;
    int oldy = location->y;
    int newx = temp_location->x;
    int newy = temp_location->y;
    Cell *new_cell = &(maze[newx][newy]);
    Cell *old_cell = &(maze[oldx][oldy]);

    if (oldx != newx || oldy != newy)
    {
      // Player moved. Update all data structures.
      // Update maze
      new_cell->playernum = a;
      if (old_cell->playernum == a)
      {
        old_cell->playernum = 0;
      }

      player->playernum = a;
      player->team = temp_player->team;
      location->x = newx;
      location->y = newy;
      player->mjolnir.hammerID = temp_player->mjolnir.hammerID;
      player->mjolnir.uses = temp_player->mjolnir.uses;
      player->flag = temp_player->flag;

    }


  }

  // Load map

  int i = 0;
  int j = 0;
  int k;
  for (k = offset; k < s->rhdr.blen; k++)
  {
    char c = s->rbuf[k];
    if (c == 'z')
    {
      break;
    }
    
    // Load appropriate Cell
    Cell *cell = &(maze[i][j]);
    if (c == 'H')
    {
      cell->type = CELL_TYPE_HOME2;
    }
    else if (c == 'h')
    {
      cell->type = CELL_TYPE_HOME1;
    }
    else if (c == 'J')
    {
      cell->type = CELL_TYPE_JAIL2;
    }
    else if (c == 'j')
    {
      cell->type = CELL_TYPE_JAIL1;
    }
    else if (c == '$')
    {
      cell->type = CELL_TYPE_UNBREAKABLE_WALL;
    }
    else if (c == '#')
    {
      cell->type = CELL_TYPE_WALL;
    }
    else if (c == 'M')
    {
      cell->mjolnir.hammerID = 2;
      rc = findMapCell(cell, i, j);
    }
    else if (c == 'm')
    {
      cell->mjolnir.hammerID = 1;
      rc = findMapCell(cell, i, j);
    }
    else if (c == 'F')
    {
      cell->flag = 2;
      rc = findMapCell(cell, i, j);
    }
    else if (c == 'f')
    {
      cell->flag = 1;
      rc = findMapCell(cell, i, j);
    }
    else if (c == ' ')
    {
      cell->type = CELL_TYPE_FLOOR;
    }

    if (j++ == 200)
    {
      j = 0;
      i++;
    }
    
  }
  
  // functions for dumping data structure information below:
  dumpPlayers();
  dumpFlagLocations();
  dumpHammerLocations();
  dumpMap(); // dump map to standard output as ASCII
  return rc;
}

// resets a map celltype based on location.
int
findMapCell(Cell *cell, int i, int j)
{
  int rc;
  if (isHome1Cell(i, j))
  {
    cell->type = CELL_TYPE_HOME1;
    rc = 1;
  }
  else if (isHome2Cell(i, j))
  {
    cell->type = CELL_TYPE_HOME2;
    rc = 1;
  }
  else if (isJail1Cell(i, j))
  {
    cell->type = CELL_TYPE_JAIL1;
    rc = 1;
  }
  else if (isJail2Cell(i, j))
  {
    cell->type = CELL_TYPE_JAIL2;
    rc = 1;
  }
  else
  {
    cell->type = CELL_TYPE_FLOOR;
    rc = 1;
  }
  return rc;
}

// can be used to manually dump the map to standard out in ASCII format.
int
doDump(Client *C)
{
  if (globals.connected)
  {
    dumpMap();
  }
  else
  {
    printf("You aren't connected to the game server, and can't dump the map.\n");
  }

  return 1;
}


// dumps map to standard out as ASCII
int
dumpMap()
{

    // Dump map 
    printf("Printing Game Map:\n");
    int i;
    int j;
    int celltype;

    for (i = 0; i < 201; i++)
    {
        for (j = 0; j < 201; j++)
        {
            Cell c = maze[i][j];
            Cell_Types celltype = c.type;

            if (celltype == CELL_TYPE_HOME1 || celltype == CELL_TYPE_HOME2 
              || celltype == CELL_TYPE_JAIL1 || celltype == CELL_TYPE_JAIL2 
              || celltype == CELL_TYPE_FLOOR)
            {
              if (c.playernum != 0)
              {
                printf("%d", c.playernum);
              }
              else if (c.mjolnir.hammerID == 2)
              {
                printf("M");
              }
              else if (c.mjolnir.hammerID == 1)
              {
                printf("m");
              }
              else if (c.flag == 2)
              {
                printf("F");
              }
              else if (c.flag == 1)
              {
                printf("f");
              }
              // in the cases below, if a player resides at that cell, print that player's number
              // instead of the cell type.
              else if (celltype == CELL_TYPE_HOME2)
              {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf("H");
                  }
              }
              else if (celltype == CELL_TYPE_HOME1)
              {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf("h");
                  }
              }
              else if (celltype == CELL_TYPE_JAIL2)
              {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf("J");
                  }
              }
              else if (celltype == CELL_TYPE_JAIL1)
              {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf("j");
                  }
              } 
              else if (celltype == CELL_TYPE_FLOOR)
              {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf(" ");
                  }
              }
            }
            else if (celltype == CELL_TYPE_WALL)
            {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf("#");
                  }
            }
            else if (celltype == CELL_TYPE_UNBREAKABLE_WALL)
            {
                  if (c.playernum != 0)
                  {
                    printf("%d", c.playernum);
                  }
                  else
                  {
                    printf("$");
                  }
            }
            

        }
        printf("\n");
    }
}

int 
dumpFlagLocations()
{
  printf("Printing Flag Locations:\n");
  printf("Flag 1 is at location (%d,%d)\n", flags[0][0], flags[0][1]);
  printf("Flag 2 is at location (%d,%d)\n", flags[1][0], flags[1][1]);
}

int
dumpHammerLocations()
{
  printf("Printing Hammer Locations:\n");
  printf("Hammer 1 is at location (%d,%d)\n", hammers[0][0], hammers[0][1]);
  printf("Hammer 2 is at location (%d,%d)\n", hammers[1][0], hammers[1][1]);
}

int
dumpPlayers()

{
    // Dump Player information
    printf("Printing Player information:\n");
    int a;
    for (a = 1; a < numPlayers+1; a++)
    {
      printf("Player %d:\n", a);
      Player *p = &(players[a]);
      printf("\tPlayer number is %d\n", p->playernum);
      printf("\tPlayer is on team number %d\n", p->team);
      printf("\tLocation: %d,%d\n", p->location.x, p->location.y);
      Hammer *h = &(p->mjolnir);
      if (h->hammerID != 0)
      {
        printf("\tCarrying Hammer %d with %d uses left.\n", h->hammerID, h->uses);
      }
      if (p->flag != 0)
      {
        printf("\tCarrying Flag %d\n", p->flag);
      }
      printf("\n");
      
    }
}


int 
main(int argc, char **argv)
{

  Client c;

  initGlobals();

  if (argc > 1)
  {
    char *input;
    argv++;
    input = *(argv++);
    globals.in.len = strlen(input);
    strcpy(globals.in.data, input);
  }
  

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }
  
  printf("\nWelcome to Capture The Flag Beta!\n");
  printMenu();

  // Add the update event handler
  Proto_MT_Handler h;
  h = &updateMap;
  proto_client_set_event_handler(c.ph, PROTO_MT_EVENT_BASE_UPDATE, h);

  if (globals.in.len > 0)
  {
    docmd(&c);
  }
  shell(&c);

  return 0;
}

