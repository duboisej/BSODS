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
  #include <sys/types.h>
  #include <poll.h>
  #include "../lib/types.h"
  #include "../lib/protocol_server.h"
  #include "../lib/protocol_utils.h"


  int player1;
  int player2;
  int turn;

  int board[10];

  int 
  updateEvent(void)
  {
    Proto_Session *s;
    Proto_Msg_Hdr hdr;

    s = proto_server_event_session();
    hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
    proto_session_hdr_marshall(s, &hdr);
    int i;
    for (i = 0; i < 10; i++)
    {
      proto_session_body_marshall_char(s, (char) board[i]);
    }

    for (i = 0; i < 10; i++)
    {
    fprintf(stderr, "%c, ", s->sbuf[i]);
    }


    proto_server_post_event();  
  }

  char MenuString[] =
    "d/D-debug on/off u-update clients q-quit";

  int 
  docmd(char cmd)
  {
    int rc = 1;

    switch (cmd) {
    case 'd':
      proto_debug_on();
      break;
    case 'D':
      proto_debug_off();
      break;
    case 'u':
      rc = updateEvent();
      break;
    case 'q':
      rc=-1;
      break;
    case '\n':
    case ' ':
      rc=1;
      break;
    default:
      printf("Unkown Command\n");
    }
    return rc;
  }

  int
  prompt(int menu) 
  {
    int ret;
    int c=0;

    if (menu) printf("%s:", MenuString);
    fflush(stdout);
    c=getchar();;
    return c;
  }

  void *
  shell(void *arg)
  {
    int c;
    int rc=1;
    int menu=1;

    while (1) {
      if ((c=prompt(menu))!=0) rc=docmd(c);
      if (rc<0) break;
      if (rc==1) menu=1; else menu=0;
    }
    fprintf(stderr, "terminating\n");
    fflush(stdout);
    return NULL;
  }

  int clientHello(Proto_Session *s)
  {
    int rc;
    int player;
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(s));
    if (player1 == 0)
    {
      
      player1 = s->fd;
      // send back Hello reply with player number
      h.type = PROTO_MT_REP_BASE_HELLO;
      proto_session_hdr_marshall(s, &h);
      proto_session_body_marshall_char(s, 'X'); // send Hello reply indicating that client is player 1
      proto_session_dump(s);
      //fprintf(stderr, "Sending reply back on fd %d", s->fd);
      rc = proto_session_send_msg(s, 1);
      fprintf(stderr, "Player number 1 connected and said hi from fd %d\n", s->fd);
      
    }
    else
    {
      player2 = s->fd;
      // send back Hello reply with player number
      h.type = PROTO_MT_REP_BASE_HELLO;
      proto_session_hdr_marshall(s, &h);
      proto_session_body_marshall_char(s, 'O'); // send Hello reply indicating that client is player 2
      rc = proto_session_send_msg(s, 1);
      fprintf(stderr, "Player number 2 connected and said hi from fd %d\n", s->fd);
    }

    proto_server_post_event();

    return rc;
  }

  int printGoodbye(Proto_Session *s)
  {
    int rc = 0;


    return rc;
  }

  int playerMove(Proto_Session *s)
  {
    int rc; 
    int fd;
    int player;
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(s));

    if (s->fd == player1)
    {
      player = 1;
    } 
    else 
    {
      player = 2;
    }

    if (player != turn)
    {
      // set up "Not your turn" reply and send
      h.type = PROTO_MT_REP_BASE_MOVE;
      proto_session_hdr_marshall(s, &h);

      proto_session_body_marshall_int(s, 1); // 1 in body indicates not your turn 
      rc=proto_session_send_msg(s,1);
    }
    else 
    {

      int move = (int) (s->rbuf[0]);
      int index = checkValidMove(move);
      if (index == -1)
      {
        // set up "Not a valid move" reply
        bzero(&h, sizeof(s));
        h.type = PROTO_MT_REP_BASE_MOVE;
        proto_session_hdr_marshall(s, &h);

        proto_session_body_marshall_int(s, 2); // 2 in body indicates invalid move
        rc=proto_session_send_msg(s,1);
      }
      else
      {
        if (player == 1)
          board[index] = 88;
        else
          board[index] = 79;

        int win = checkwin();
        board[9] = win;

        rc = updateEvent();
        printBoard();
      }
    }
    return rc;
  }

  int
  checkwin(void)
  {

    int num = board[0] + board[1] + board[2]; // top horizontal
    if(num == 237 || num == 264) {

      if (num == 237){
        //O’s Won
        return 0;
      } else {
        return 1;
      }
    }

    num = board[3] + board[4] + board[5]; // middle horizontal
    if(num == 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }

    } 

    num = board[6] + board[7] + board[8]; // bottom horizontal
    if(num == 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }
    } 

    num = board[0] + board[4] + board[8]; // left to right diagonal
    if(num == 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }
    } 

    num = board[2]+ board[4] + board[6]; // right to left diagonal
    if(num == 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }
    } 

    num = board[0] + board[3] + board[6]; // left vertical
    if(num == 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }
    } 

    num = board[1] + board[4] + board[7]; // middle vertical
    if(num== 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }
    } 

    num = board[2] + board[5] + board[8]; // right vertical
    if(num == 237 || num == 264) {
      if (num == 237){
        //O’s Won
        return 0;
      } else {
        //X’s Won
        return 1;
      }
    }

    num = 0; 
    int i; 
    for (i = 0; i < 9; i++)
    {
      num += board[i];
    }
    if (num == 756)  // It's a draw - the board is full
    {
      return 2;
    }

    return -1;

  }

  int
  checkValidMove(int cell)
  {
    if (cell < 49 || cell > 57)
    {
      return -1;
    }
    else if (board[cell - 49] == 88 || board[cell - 49] == 79)
    {
      return -1;
    }
    return (cell - 49);
  }

  int 
  printBoard()
  {
    //printf("Fuck you, Niko.\n");
    printf("%c|%c|%c\n", board[0], board[1], board[2]);
    printf("-----\n");
    printf("%c|%c|%c\n", board[3], board[4], board[5]); 
    printf("-----\n");
    printf("%c|%c|%c\n", board[6], board[7], board[8]);
    printf("board[9] = %c\n", board[9]);


  }


  int
  main(int argc, char **argv)
  { 
    if (proto_server_init()<0) {
      fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
      exit(-1);
    }

    fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), 
  	  proto_server_eventport());

    //printf("Finished.");

    // initialize game board
    int i;
    for (i = 0; i < 9; i++)
    {
      board[i] = i+49;
    }

    // board[5] = (int) 'O';
    // board[4] = (int) 'O';
    // board[3] = (int) 'O';

    // board[9] = -1;

    // int won = checkwin();

    // if (won == -1)
    // {
    //   printf("No win.");
    // }
    // else if (won == 0)
    // {
    //   printf("O's win!");
    // }
    // else if (won == 1)
    // {
    //   printf("X's win!");
    // }
    // else 
    // {
    //   printf("There was an error in the function.");
    // }

    if (proto_server_start_rpc_loop()<0) {
      fprintf(stderr, "ERROR: failed to start rpc loop\n");
      exit(-1);
    }

    Proto_MT_Handler h = &clientHello;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_HELLO, h);
    Proto_MT_Handler m = &playerMove;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_MOVE, h);

      
    shell(NULL);

    return 0;
  }
