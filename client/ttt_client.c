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

struct LineBuffer {
  char data[BUFLEN];
  int  len;
  int  newline;
};

struct Globals {
  struct LineBuffer in;
  char server[STRLEN];
  PortType port;
  FDType serverFD;
  int connected;
  int verbose;
} globals;

typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

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
  int c=0;

  if (menu) printf("%s", MenuString);
  fflush(stdout);
  c = getchar();
  return c;
}


// FIXME:  this is ugly maybe the speration of the proto_client code and
//         the game code is dumb
int
game_process_reply(Client *C)
{
  Proto_Session *s;

  s = proto_client_rpc_session(C->ph);

  fprintf(stderr, "%s: do something %p\n", __func__, s);

  return 1;
}


int 
doRPCCmd(Client *C, char c) 
{
  int rc=-1;

  switch (c) {
  case 'h':  
    {
      rc = proto_client_hello(C->ph);
      printf("hello: rc=%x\n", rc);
      if (rc > 0) game_process_reply(C);
    }
    break;
  case 'm':
    scanf("%c", &c);
    rc = proto_client_move(C->ph, c);
    break;
  case 'g':
    rc = proto_client_goodbye(C->ph);
    break;
  default:
    printf("%s: unknown command %c\n", __func__, c);
  }
  // NULL MT OVERRIDE ;-)
  printf("%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  return rc;
}

int
doRPC(Client *C)
{
  int rc;
  char c;

  printf("enter (h|m<c>|g): ");
  scanf("%c", &c);
  rc=doRPCCmd(C,c);

  printf("doRPC: rc=0x%x\n", rc);

  return rc;
}


int 
docmd(Client *C, char cmd)
{
  int rc = 1;

  switch (cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'r':
    rc = doRPC(C);
    break;
  case 'q':
    rc=-1;
    break;
  case '\n':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
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
    if ((c=prompt(menu))!=0) rc=docmd(C, c);
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
initGlobals(int argc, char **argv)
{
  bzero(&globals, sizeof(globals));

  if (argc==1) {
    usage(argv[0]);
    exit(-1);
  }

  if (argc==2) {
    strncpy(globals.host, "localhost", STRLEN);
    globals.port = atoi(argv[1]);
  }

  if (argc>=3) {
    strncpy(globals.host, argv[1], STRLEN);
    globals.port = atoi(argv[2]);
  }

}

static int
testEvent(Proto_Session *s)
{
  fprintf(stderr, "Received test event! OMGLOL!!!");
  return 1;
}

static void
printMenu()
{
  printf("Your options are: \nconnect <ip:port> (to connect to a server)\n");
  printf("disconnect (to disconnect from the server)\n");
  printf("<Enter key> (display current game board)\n");
  printf("[0-9] (mark the appropriate cell)\n");
  printf("where (display <ip:port> of the server you are connected to).\n");
  printf("quit (to quit client)");

}

int 
main(int argc, char **argv)
{
  Client c;
  printf("Welcome to Tic-Tac-Toe!\n");
  printMenu();

  shell(&c);

  return 0;
}

// Stuff from Assignment 1

// int 
// doCmd(void)
// {
//   int rc = 1;

//   if (strlen(globals.in.data)==0) return rc;
//   else if (strncmp(globals.in.data, "connect", 
//        sizeof("connect")-1)==0) rc = doConnect();
//   else if (strncmp(globals.in.data, "send", 
//        sizeof("send")-1)==0) rc = doSend();
//   else if (strncmp(globals.in.data, "quit", 
//        sizeof("quit")-1)==0) rc = doQuit();
//   else if (strncmp(globals.in.data, "verbose", 
//        sizeof("verbose")-1)==0) rc = doVerbose();
//   else printf("Unknown Command\n");

//   return rc;
// }

// int
// main(int argc, char **argv)
// {
//   int rc, menu=1;

//   bzero(&globals, sizeof(globals));

//   while (1) {
//     if (prompt(menu)>=0) rc=doCmd(); else rc=-1;
//     if (rc<0) break;
//     //What do you think the next line is for
//     if (rc==1) menu=1; else menu=0;
//   }

//   VPRINTF("Exiting\n");
//   fflush(stdout);

//   return 0;
// }

