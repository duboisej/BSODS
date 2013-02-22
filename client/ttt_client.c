/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"

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

char board[10];

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
  printf("Your command options are as follows: \n");
  printf("\n'connect <ip:port>':\n\tconnects you to the server with the corresponding IP address and port number. If you are already connected to a server, the command will do nothing. Example: connect 127.0.0.1:38500\n");
  printf("\n'disconnect':\n\tdisconnects you from the server you are currently connected to. If not connected to a server, the command will do nothing.\n");
  printf("\nPressing the Enter key:\n\tIf pressed without any other typing, the current state of the board will be redisplayed.\n");
  printf("\nPressing keys [0-9]:\n\tmarks the corresponding cell on the board. If not your turn, nothing will be done.\n");
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
  //VPRINTF("BEGIN: %s\n", globals.in.data);

  if (globals.connected==1) {
    fprintf(stderr, "Already connected to server.\n");
  } else {
    
    for (i=0; i<len; i++) if (globals.in.data[i]==':') globals.in.data[i]=' ';
    sscanf(globals.in.data, "%*s %" XSTR(STRLEN) "s %d", globals.host,
	   &globals.port);
    
    if (strlen(globals.host)==0 || globals.port==0) {
      fprintf(stderr, "Missing server or port.\n");
    } else {
      //printf("connecting...");
      if (startConnection(C, globals.host, globals.port, update_event_handler)<0) {
    	  fprintf(stderr, "ERROR: startConnection failed\n");
          return -1;
      } else {
		    globals.connected=1;
		    //printf("connected to server!\n");
        s = proto_client_rpc_session(C->ph);
		    if (proto_client_hello(C->ph) > 0)
        {
          globals.playersymbol = s->rbuf[0];
          printf("Connected to %s:%d: You are %c's\n", globals.host, globals.port, globals.playersymbol);
        } 
        else
        {
          fprintf(stderr, "No reply from server.\n");
          return -1;
        }

      }
    }
  }

  //VPRINTF("END: %s %d %d\n", globals.server, globals.port, globals.serverFD);
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
doMark(Client* C, char cell)
{
  int rc;
  rc = proto_client_move(C->ph, cell);
  // Handle replies
  Proto_Session *s = proto_client_rpc_session(C->ph);
  proto_session_hdr_unmarshall(s, &(s->rhdr));
  int replycode = ntohl(s->rbuf[0]);
  fprintf(stderr, "reply code was %d\n", replycode);

  if (replycode == 1)
  {
    printf("Not your turn yet!");
  }
  else if (replycode == 2)
  {
    printf("Not a valid move!");
  }
  
  return rc;

}

int 
docmd(Client *C)
{
  int rc = 1;
  
  if (strlen(globals.in.data)==0) return rc; //rc = doReprint();
  else if (strncmp(globals.in.data, "connect", 
		   sizeof("connect")-1)==0) rc = doConnect(C);
  // else if (strncmp(globals.in.data, "disconnect",
  // 		   sizeof("disconnect")-1==0) rc = doDisconnect();
  else if (strncmp(globals.in.data, "1", 
		   sizeof("1")-1)==0) rc = doMark(C, '1');
  else if (strncmp(globals.in.data, "2", 
		   sizeof("2")-1)==0) rc = doMark(C, '2');
  else if (strncmp(globals.in.data, "3", 
		   sizeof("3")-1)==0) rc = doMark(C, '3');
  else if (strncmp(globals.in.data, "4", 
		   sizeof("4")-1)==0) rc = doMark(C, '4');
  else if (strncmp(globals.in.data, "5", 
		   sizeof("5")-1)==0) rc = doMark(C, '5');
  else if (strncmp(globals.in.data, "6", 
		   sizeof("6")-1)==0) rc = doMark(C, '6');
  else if (strncmp(globals.in.data, "7", 
		   sizeof("7")-1)==0) rc = doMark(C, '7');
  else if (strncmp(globals.in.data, "8", 
		   sizeof("8")-1)==0) rc = doMark(C, '8');
  else if (strncmp(globals.in.data, "9", 
		   sizeof("9")-1)==0) rc = doMark(C, '9');
  else if (strncmp(globals.in.data, "where", 
		   sizeof("where")-1)==0) rc = doWhere();
  else if (strncmp(globals.in.data, "quit", 
		   sizeof("quit")-1)==0) rc = -1;
  else if (strncmp(globals.in.data, "help", 
       sizeof("help")-1)==0) printMenu();
  else  printf("Unknown Command\n");
  
  return rc;
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
printBoard()
{
  //printf("Fuck you, Niko.\n");
  printf("%c|%c|%c\n", board[0], board[1], board[2]);
  printf("-----\n");
  printf("%c|%c|%c\n", board[3], board[4], board[5]); 
  printf("-----\n");
  printf("%c|%c|%c\n", board[6], board[7], board[8]);


}

static int
updateBoard(Proto_Session *s)
{
  //fprintf(stderr, "Called updateBoard, bitch.\n");
  int rc = 1;
  int i;
  fprintf(stderr, "\n");
  for (i = 0; i < 10; i++)
  {
    board[i] = s->rbuf[i];
    //fprintf(stderr, "Unmarshalled char number %c : %c", i, *unmarshalled);
  }
  printBoard();
  if (board[9] == 1) // X's Won
  {
    if (globals.playersymbol == 'X')
    {
      printf("Game Over: You win!! :D\n");
      exit(0);
    } 
    else 
    {
      printf("Game Over: You lose :'(\n");
      exit(0);
    }
  }
  else if (board[9] == 0) // O's Won
  {
    if (globals.playersymbol == 'O')
    {
      printf("Game Over: You win!! :D\n");
      exit(0);
    }
    else 
    {
      printf("Game Over: You lose :'(\n");
      exit(0);
    }
  }
  else if (board[9] == 2)
  {
    printf("Game Over: Draw. :/\n");
    exit(0);
  }
  else
  {
    return 1;
  }
  
}

int 
main(int argc, char **argv)
{
  Client c;

  initGlobals();

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }
  
  printf("Welcome to Tic Tac Toe!\n");
  printMenu();

  Proto_MT_Handler h;
  h = &updateBoard;
  proto_client_set_event_handler(c.ph, PROTO_MT_EVENT_BASE_UPDATE, h);

  shell(&c);

  return 0;
}
