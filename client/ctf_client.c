#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../lib/maze.h"

#define STRLEN 81
#define BUFLEN 16384
#define XSTR(s) STR(s)
#define STR(s) #s

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
  char playersymbol;
} globals;


typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

//char board[10];

// represent game board..?
char map[201][201];
int xDimension = 200;
int yDimension = 200;

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
  printf("\n'numhome<1 or 2>':\n\tqueries the number of home cells for the specified team, 1 or 2.\n");
  printf("\n'numjail<1 or 2>':\n\tqueries the number of jail cells for the specified team, 1 or 2.\n");
  printf("\n'numwall':\n\tqueries the number of wall cells in the whole map.\n");
  printf("\n'numfloor':\n\tqueries the number of floor cells in the whole map.\n");
  printf("\n'dim':\n\treturns the dimensions of the map.\n");
  printf("\n'cinfo<x,y>':\n\treturns cell information at the given coordinates, if it is a valid one.\n");
  printf("\n'dump':\n\tprints the map.\n");
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
        s = proto_client_rpc_session(C->ph);
		    if (proto_client_hello(C->ph) > 0)
        {
          globals.playersymbol = s->rbuf[0];
          if (globals.playersymbol == 'X' || globals.playersymbol == 'O')
          {
            printf("Connected to %s:%d: You are %c's\n", globals.host, globals.port, globals.playersymbol);
          }
          else if (globals.playersymbol == 'S')
          {
            printf("Connected to %d:%d: You are a spectator\n");
          }
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
doFetchInfo(Client* C, char cell)
{
  
  int rc;
  int *replycode;
  rc = proto_client_move(C->ph, cell);
  // Handle replies
  Proto_Session *s = proto_client_rpc_session(C->ph);
  proto_session_body_unmarshall_int(s, 0, replycode);
  
  if (cell == 'f') {
  	printf("There are %d floor cells.\n", *replycode);
    rc = 1;
  }
  else if (cell == 'H')
  {
    printf("There are %d home cells for team 1.\n", *replycode);
    rc = 1;
  }
  else if (cell == 'h')
  {
    printf("There are %d home cells for team 2.\n", *replycode);
    rc = 1;
  }
  else if (cell == 'J')
  {
    printf("There are %d jail cells for team 1.\n", *replycode);
    rc = 1;
  }
  else if (cell == 'j')
  {
    printf("There are %d jail cells for team 2.\n", *replycode);
    rc = 1;
  }
  else if (cell == 'w')
  {
    printf("There are %d wall cells.\n", *replycode);
    rc = 1;
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
  if (globals.playersymbol != 'S')
  {
	//Send a goodbye message to server. Don't wait for a reply.
	   proto_client_goodbye(C->ph);
  }
	//Close both sockets (fd's)
  printf("Game Over: You Quit\n");
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
doGetDimension (Client *C)
{
  printf("The dimensions of the board are %d x %d.\n", xDimension, yDimension);
  return 1;
}

int 
docmd(Client *C)
{
  int i = 0;
  int rc = 1;
  printf("You entered: %s\n", globals.in.data);
  if (strlen(globals.in.data)==0) return rc; //rc = doReprint();
  else if (strncmp(globals.in.data, "connect", sizeof("connect")-1)==0) rc = doConnect(C);
  else if (strncmp(globals.in.data, "disconnect", sizeof("disconnect")-1)==0) rc = doDisconnect(C);
  else if (strncmp(globals.in.data, "numhome 1", sizeof("numhome 1")-1)==0) rc = doFetchInfo(C, 'H');
  else if (strncmp(globals.in.data, "numhome 2", sizeof("numhome 2")-1)==0) rc = doFetchInfo(C, 'h');
  else if (strncmp(globals.in.data, "numjail 1", sizeof("numjail 1")-1)==0) rc = doFetchInfo(C, 'J');
  else if (strncmp(globals.in.data, "numjail 2", sizeof("numjail 2")-1)==0) rc = doFetchInfo(C, 'j');
  else if (strncmp(globals.in.data, "dim", sizeof("dim")-1)==0) rc = doGetDimension(C);
  else if (strncmp(globals.in.data, "numwall", sizeof("numwall")-1)==0) rc = doFetchInfo(C, 'w');
  else if (strncmp(globals.in.data, "numfloor", sizeof("numfloor")-1)==0) rc = doFetchInfo(C, 'f');
  else if (strncmp(globals.in.data, "dump", sizeof("dump")-1)==0) rc = doDump(C);
  else if (strncmp(globals.in.data, "quit", 
		   sizeof("quit")-1)==0) rc = doQuit();
  else  printf("Unknown Command\n");
  
  return rc;
}

int doQuit(Client *C)
{
  if (globals.playersymbol == 'X' || globals.playersymbol == 'O')
  {
    proto_client_goodbye(C->ph);
  }
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

static int
updateMap(Proto_Session *s)
{
  printf("Got into updateMap.\n");
  proto_session_dump(s);
  int i = 0;
  int j = 0;
  int k;
  for (k = 0; k < s->rhdr.blen; k++)
  {
    char c = s->rbuf[k];
    if (c == 'z')
    {
      break;
    }
    map[i][j] = c;
    printf("%c", c);
    if (j++ == 200)
    {
      j = 0;
      i++;
    }
    
  }
  return 1;
}

int
doDump(Client *C)
{
  int i;
  int j;
  for (i = 0; i < 201; i++)
  {
    for (j = 0; j < 201; j++)
    {
      printf("%c", map[i][j]);
    }
  }

  printf("Cell at 100, 2:");
  doCInfo(C, 100, 2);

  return 1;
}

int
doCInfo(Client *C, int x, int y)
{
  char c = map[x][y];
  int team;

  if (y <= 100) team = 1; 
  else team = 2;

  if (c == ' ')
  {

    printf("Cell at (%d, %d) is an unoccupied floor cell on team %d\n", x, y, team);
  }
  else if (c == 'J' || c == 'j')
  {
      printf("Cell at (%d, %d) is an unoccupied jail cell on team %d\n", x, y, team);
  }
  else if (c == 'H' || c == 'h') 
  {
      printf("Cell at (%d, %d) is an unoccupied home cell on team %d\n", x, y, team);
  }
  else if (c == '#')
  {
      printf("Cell at (%d, %d) is a wall cell on team %d\n", x, y, team);
  }
}
// static int 
// printBoard()
// {
//   printf("%c|%c|%c\n", board[0], board[1], board[2]);
//   printf("-----\n");
//   printf("%c|%c|%c\n", board[3], board[4], board[5]); 
//   printf("-----\n");
//   printf("%c|%c|%c\n", board[6], board[7], board[8]);


// }

// static int
// updateBoard(Proto_Session *s)
// {
//   int rc = 1;
//   int i;
//   int same = 1;

//   // check if board needs to be updated
//   for (i = 0; i < 9; i++)
//   {
//     char old = board[i];
//     char new = s->rbuf[i];
//     if (old != new)
//     {
//       board[i] = new;
//       same = 0;
//     }
//   }
//   // grab the last cell 
//   board[9] = s->rbuf[9];

//   if (same == 0)
//   {
//     fprintf(stderr, "\n");
//     printBoard();
//   }

//   // Game win logic
//   if (board[9] == 1) // X's Won
//   {
//     if (globals.playersymbol == 'X')
//     {
//       printf("Game Over: You win!! :D\n");
//     } 
//     else if (globals.playersymbol == 'O')
//     {
//       printf("Game Over: You lose :'(\n");
//     } 
//     else
//     {
//       printf("Game Over: X's Won.\n");
//     }
//     exit(0);
//   }
//   else if (board[9] == 0) // O's Won
//   {
//     if (globals.playersymbol == 'O')
//     {
//       printf("Game Over: You win!! :D\n");
//     }
//     else if (globals.playersymbol == 'X')
//     {
//       printf("Game Over: You lose :'(\n");
//     }
//     else
//     {
//       printf("Game Over: O's Won.\n");
//     }
//     exit(0);
//   }
//   else if (board[9] == 2)
//   {
//     printf("Game Over: Draw. :/\n");
//     exit(0);
//   }
//   else if (board[9] == 3) // One player quit
//   {
//     if (globals.playersymbol == 'X' || globals.playersymbol == 'O')
//     {
//       printf("Game Over: Other Side Quit\n");

//     }
//     else
//     {
//       printf("A player quit the game.\n");
//     }
//     exit(0);
//   }
//   else
//   {
//     return 1;
//   }
  
// }

int 
main(int argc, char **argv)
{
  Client c;

  initGlobals();

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

  shell(&c);

  return 0;
}