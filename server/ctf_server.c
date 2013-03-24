  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <poll.h>
  #include "../lib/types.h"
  #include "../lib/protocol_server.h"
  #include "../lib/protocol_utils.h"
  #include "../lib/maze.h"


  Cell maze[201][201];
  int numfloor;
  int numhome1;
  int numhome2;
  int numwall;
  int numjail1;
  int numjail2;

  FILE *map;

  int
  load()
  {

   printf("Got past variables.\n");
   int c;

   map = fopen ("../server/daGame.map", "r");  

   printf("Opened file.\n");

   int i;
   int j;
   for (i = 0; i < 200; i++)
   {
      for (j = 0; j <= 200; j++)
      {


            c = fgetc(map);

            if (i == 0 || j == 0 || i == 199 || j == 199)
            {
                
                maze[i][j].type = CELL_TYPE_UNBREAKABLE_WALL;
                numwall++;
            }
            else if (c == ' ')
            {
                maze[i][j].type = CELL_TYPE_FLOOR;
                numfloor++;
            }
            else if (c == 'J')
            {
                maze[i][j].type = CELL_TYPE_JAIL1;
                numjail1++;
            }
            else if (c == 'j')
            {
                maze[i][j].type = CELL_TYPE_JAIL2;
                numjail2++;
            }
            else if (c == 'H') 
            {
                maze[i][j].type = CELL_TYPE_HOME1;
                numhome1++;
            }
            else if (c == 'h')
            {
                maze[i][j].type = CELL_TYPE_HOME2;
                numhome2++;
            }
            else if (c == '#')
            {
                maze[i][j].type = CELL_TYPE_WALL;
                numwall++;
            }
      } 
    }
}

int
dump()
{
    int i;
    int j;
    int celltype;

    for (i = 0; i < 200; i++)
    {
        for (j = 0; j < 200; j++)
        {
            celltype = maze[i][j].type;

            if (celltype == CELL_TYPE_HOME1)
            {
                printf("H");
            }
            else if (celltype == CELL_TYPE_HOME2)
            {
                printf("h");
            }
            else if (celltype == CELL_TYPE_JAIL1)
            {
                printf("J");
            }
            else if (celltype == CELL_TYPE_JAIL2)
            {
                printf("j");
            }
            else if (celltype == CELL_TYPE_WALL || celltype == CELL_TYPE_UNBREAKABLE_WALL)
            {
                printf("#");
            }
            else if (celltype == CELL_TYPE_FLOOR)
            {
                printf(" ");
            }

        }
        printf("\n");
    }
}

  int 
  updateEvent(void)
  {
    printf("updateEvent called.\n");
    Proto_Session *s;
    Proto_Msg_Hdr hdr;

    s = proto_server_event_session();
    hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
    proto_session_hdr_marshall(s, &hdr);

    int i;
    int j;
    int celltype;

    for (i = 0; i < 200; i++)
    {
        for (j = 0; j < 200; j++)
        {
            celltype = maze[i][j].type;

            if (celltype == CELL_TYPE_HOME1)
            {
                proto_session_body_marshall_char(s, 'H');
                //printf("H");
            }
            else if (celltype == CELL_TYPE_HOME2)
            {
                proto_session_body_marshall_char(s, 'h');
                //printf("h");
            }
            else if (celltype == CELL_TYPE_JAIL1)
            {
                proto_session_body_marshall_char(s, 'J');
                //printf("J");
            }
            else if (celltype == CELL_TYPE_JAIL2)
            {
                proto_session_body_marshall_char(s, 'j');
                //printf("j");
            }
            else if (celltype == CELL_TYPE_WALL || celltype == CELL_TYPE_UNBREAKABLE_WALL)
            {
                proto_session_body_marshall_char(s, '#');
                //printf("#");
            }
            else if (celltype == CELL_TYPE_FLOOR)
            {
              // if (maze[i][j].contains == OBJECT_TYPE_FLAG)
                // {
                //   map[count] = 'f';
                // }
                // else if (maze[i][j].contains == OBJECT_TYPE_HAMMER)
                // {
                //   map[count] = 'm';
                // }
              proto_session_body_marshall_char(s, ' ');
                //printf(" ");
            }

        }
        proto_session_body_marshall_char(s, '\n');
    }
    proto_session_body_marshall_char(s, 'z');
    proto_session_dump(s);
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

  int playerHello(Proto_Session *s)
  {
    printf("Player connected and said hello\n");
    int rc;
    int player;
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(s));
    h.type = PROTO_MT_REP_BASE_HELLO;
    proto_session_hdr_marshall(s, &h);
    rc = proto_session_send_msg(s, 1);
    updateEvent();
    return rc;
  }

  int playerGoodbye(Proto_Session *s)
  {
    int rc = 0;
    int player = 0;
    
    updateEvent();
    exit(0);

    return rc;
  }

  int playerQuery(Proto_Session *s)
  {
    printf("Got into playerQuery, motherfucker.\n");
    int rc; 
    int fd;
    int player;
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(s));

    char choice = s->rbuf[0]; // get choice from send buffer

        // send good (empty) reply
    bzero(&h, sizeof(s));
    h.type = PROTO_MT_REP_BASE_MOVE;
    proto_session_hdr_marshall(s, &h);

     // logic for which number to send back
    int reply = -1; 
    if (choice == 'f')
    {
      reply = numfloor;
    }
    else if (choice == 'H')
    {
       reply = numhome1;
    }
    else if (choice == 'h')
    {
      reply = numhome2;
    }
    else if (choice == 'w')
    {
      //printf("There are %d wall cells according to the server.\n", numwall);
      reply = numwall;
    }
    else if (choice == 'J')
    {
      reply = numjail1;
    }
    else if (choice == 'j')
    {
      reply = numjail2;
    }

    proto_session_body_marshall_int(s, reply); // 0 in body indicates good move.
    rc=proto_session_send_msg(s,1);

    //updateEvent();
    
    return rc;
  }

  // int
  // checkwin(void)
  // {

  //   int num = board[0] + board[1] + board[2]; // top horizontal
  //   if(num == 237 || num == 264) {

  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       return 1;
  //     }
  //   }

  //   num = board[3] + board[4] + board[5]; // middle horizontal
  //   if(num == 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }

  //   } 

  //   num = board[6] + board[7] + board[8]; // bottom horizontal
  //   if(num == 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }
  //   } 

  //   num = board[0] + board[4] + board[8]; // left to right diagonal
  //   if(num == 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }
  //   } 

  //   num = board[2]+ board[4] + board[6]; // right to left diagonal
  //   if(num == 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }
  //   } 

  //   num = board[0] + board[3] + board[6]; // left vertical
  //   if(num == 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }
  //   } 

  //   num = board[1] + board[4] + board[7]; // middle vertical
  //   if(num== 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }
  //   } 

  //   num = board[2] + board[5] + board[8]; // right vertical
  //   if(num == 237 || num == 264) {
  //     if (num == 237){
  //       //O’s Won
  //       return 0;
  //     } else {
  //       //X’s Won
  //       return 1;
  //     }
  //   }

  //   num = 0; 
  //   int i; 
  //   for (i = 0; i < 9; i++)
  //   {
  //     num += board[i];
  //   }
  //   if (num == 756)  // It's a draw - the board is full
  //   {
  //     return 2;
  //   }

  //   return -1;

  // }

  // int
  // checkValidMove(int cell)
  // {
  //   if (cell < 49 || cell > 57)
  //   {
  //     return -1;
  //   }
  //   else if (board[cell - 49] == 88 || board[cell - 49] == 79)
  //   {
  //     return -1;
  //   }
  //   return (cell - 49);
  // }

  // int 
  // printBoard()
  // {
  //   printf("%c|%c|%c\n", board[0], board[1], board[2]);
  //   printf("-----\n");
  //   printf("%c|%c|%c\n", board[3], board[4], board[5]); 
  //   printf("-----\n");
  //   printf("%c|%c|%c\n", board[6], board[7], board[8]);
  //   printf("board[9] = %d\n", board[9]);


  // }


  int
  main(int argc, char **argv)
  { 
    if (proto_server_init()<0) {
      fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
      exit(-1);
    }

    fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), 
  	  proto_server_eventport());

    // initialize game board
    load();

    if (proto_server_start_rpc_loop()<0) {
      fprintf(stderr, "ERROR: failed to start rpc loop\n");
      exit(-1);
    }


    Proto_MT_Handler h = &playerHello;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_HELLO, h);
    Proto_MT_Handler q = &playerQuery;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_MOVE, q);
    Proto_MT_Handler g = &playerGoodbye;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_GOODBYE, g);

      
    shell(NULL);

    return 0;
  }

