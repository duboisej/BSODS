  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <poll.h>
  #include "../lib/types.h"
  #include "../lib/maze.h"
  #include "../lib/protocol_server.h"
  #include "../lib/protocol_utils.h"

  // Players can't spawn on either of the hammer's home locations.
  #define HAMMER1X 10
  #define HAMMER1Y 95
  #define HAMMER2X 190
  #define HAMMER2Y 95

  Cell maze[201][201];

  Player players[500];

  int hammers[2][2];
  int flags[2][2];

  int nextPlayerIndex = 1;
  int nextTeam = 1;

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

// int 
//   updateEvent(void)
//   {
//     printf("updateEvent called.\n");
//     Proto_Session *s;
//     Proto_Msg_Hdr hdr;

//     s = proto_server_event_session();
//     hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
//     proto_session_hdr_marshall(s, &hdr);

//     printf("Marshalling hammer locations\n");
//     // Marshall hammer locations
//     int l;
//     int m;
//     int offset = 0;
//     // for (l = 0; l < 2; l++)
//     // {
//     //   for (m = 0; m < 2; m++)
//     //   {
//     //     proto_session_body_marshall_int(s, hammers[l][m]);
//     //   }
//     // }

//     printf("Marshalling game map\n");
//     // Marshall game map
//     int i;
//     int j;
//     for (i = 0; i < 201; i++)
//     {
//       for (j = 0; j < 201; j++)
//       {
//         printf("marshalling cell (%d,%d)\n", i, j);
//         Cell *c = &(maze[i][j]);
//         proto_session_body_marshall_cell(s, c);
//       }
//     }

//     // printf("Marshalling number of players\n");
//     // // Send over number of players

//     // proto_session_body_marshall_int(s, nextPlayerIndex);

//     // printf("Marshalling players\n");
//     // // Marshall Player info
//     // int k;
//     // for (k = 1; k < nextPlayerIndex; k++)
//     // {
//     //   Player *p = &(players[k]);
//     //   proto_session_body_marshall_player(s, p);
//     // }

//     proto_session_dump(s);
//     proto_server_post_event();

//   }

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
            Cell c = maze[i][j];
            Cell_Types celltype = c.type;

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
            else if (celltype == CELL_TYPE_WALL)
            {
                proto_session_body_marshall_char(s, '#');
                //printf("#");
            }
            else if (celltype == CELL_TYPE_UNBREAKABLE_WALL)
            {
                proto_session_body_marshall_char(s, '$');
            }
            else if (celltype == CELL_TYPE_FLOOR)
            {
              if (c.mjolnir.hammerID == 1)
              {
                proto_session_body_marshall_char(s, 'M');
              }
              else if (c.mjolnir.hammerID == 2)
              {
                proto_session_body_marshall_char(s, 'm');
              }
              else if (c.flag == 1)
              {
                proto_session_body_marshall_char(s, 'F');
              }
              else if (c.flag == 2)
              {
                proto_session_body_marshall_char(s, 'f');
              }
              else
              {
                proto_session_body_marshall_char(s, ' ');
                //printf(" ");
              }
            }

        }
        proto_session_body_marshall_char(s, '\n');
    }
    proto_session_body_marshall_char(s, 'z');
    proto_session_dump(s);
    proto_server_post_event();  
  }

  int 
  moveEvent(int x, int y, int playernum)
  {
    printf("moveEvent called.\n");
    Proto_Session *s;
    Proto_Msg_Hdr hdr;

    s = proto_server_event_session();
    hdr.type = PROTO_MT_EVENT_BASE_MOVE;
    proto_session_hdr_marshall(s, &hdr);
    proto_session_body_marshall_int(s, x);
    proto_session_body_marshall_int(s, y);
    proto_session_body_marshall_int(s, playernum);
    proto_session_dump(s);
    proto_server_post_event();  
  }

  int 
  breakWallEvent(int x, int y)
  {
    printf("breakWallEvent called.\n");
    Proto_Session *s;
    Proto_Msg_Hdr hdr;

    s = proto_server_event_session();
    hdr.type = PROTO_MT_EVENT_BASE_MOVE;
    proto_session_hdr_marshall(s, &hdr);
    proto_session_body_marshall_int(s, x);
    proto_session_body_marshall_int(s, y);
    proto_session_dump(s);
    proto_server_post_event();  
  }

  // int 
  // objectEvent(int playernum, Object_Types object, char action)
  // {
  //   printf("objectEvent called\n");
  //   Proto_Session *s;
  //   Proto_Msg_Hdr hdr;

  //   s = proto_server_event_session();
  //   if (object == OBJECT_TYPE_HAMMER)
  //   {
  //     if (action == 'p')
  //     {
  //       hdr.type = PROTO_MT_EVENT_BASE_PICKUPHAMMER;
  //     }
  //     else 
  //     {
  //       hdr.type = PROTO_MT_EVENT_BASE_DROPHAMMER;
  //     }
  //   }
  //   else
  //   {
  //     if(action == 'p')
  //     {
  //       hdr.type = PROTO_MT_EVENT_BASE_PICKUPFLAG;
  //     }
  //     else
  //     {
  //       hdr.type = PROTO_MT_EVENT_BASE_DROPFLAG;
  //     }
  //   }
  //   hdr.type = PROTO_MT_EVENT_BASE_PICKUPFLAG;
  //   proto_session_hdr_marshall(s, &hdr);
  //   proto_session_body_marshall_int(s, playernum);
  //   proto_session_dump(s);
  //   proto_server_post_event();  
  // }

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

  int playerGoodbye(Proto_Session *s)
  {
    int rc = 0;
    int player = 0;
    
    updateEvent();
    exit(0);

    return rc;
  }

  // int 
  // checkMove(char move, int playernum)
  // {
  //   Player p = player[playernum];
  //   Player o = map[playery][playerx]->player;

  //   if(o->team != 0)
  //   {
  //     movingPlayerTeam = p->team;;
  //     otherPlayerTeam = o->team;

  //     if (movingPlayerTeam != otherPlayerTeam)
  //     {
  //       // Tag player

  //     }
  //   }

  // }

  int playerMove(Proto_Session *s)
  {
    int rc; 
    int fd;
    int player;
    Proto_Msg_Hdr h;
    char move = s->rbuf[0];
    int playernum;
    proto_session_body_unmarshall_int(s, sizeof(Proto_Msg_Hdr) + 1, &playernum);
    // Game logic - check for valid move and if valid, update map

    int valid = 0; 

    if(move = 'w')
    {
      //valid = checkMove(move, playernum);

    }

    if (valid)
    {
      // send good (empty) reply
      bzero(&h, sizeof(s));
      h.type = PROTO_MT_REP_BASE_MOVE;
      proto_session_hdr_marshall(s, &h);
      rc=proto_session_send_msg(s, 1);
    }
    else
    {
      // send reply with invalid
      h.type = PROTO_MT_REP_BASE_MOVE;
      proto_session_hdr_marshall(s, &h);
      proto_session_body_marshall_char(s, 'I'); // 'I' in body indicates invalid move
      rc=proto_session_send_msg(s,1);

    }
    // more logic here?
    
    return rc;
  }

  int 
  getLocationInRange(int min, int max)
  {
    srand(time(NULL));
    int x = min + (rand() % (max - min));
    return x;
  }

  Point getSpawnLocation(int team)
  {
    int x;
    int y;
    Cell *c;
    do
    {
      c = &(maze[y][x]);
      if (team == 1)
      {
        x = getLocationInRange(2, 11);
      }
      else 
      {
        x = getLocationInRange(188, 197);
      }
      y = getLocationInRange(90, 108);

    }
    while(c->playernum != 0 && y != 95 && x != 190 && x != 10);

    Point p;
    p.x = x;
    p.y = y;
    return p;
  }

  Point 
  spawnFlag(int team)
  {
    Point p;
    int x;
    int y;
    int xmin;
    int xmax;
    int ymin = 1;
    int ymax = 199;
    if (team == 1)
    {
      xmin = 1; 
      xmax = 100;
    }
    else
    {
      xmin = 101;
      xmax = 199;
    }

    do
    {
      x = getLocationInRange(xmin, xmax);
      y = getLocationInRange(ymin, ymax);
    } 
    while (maze[y][x].type != CELL_TYPE_WALL && maze[y][x].type != CELL_TYPE_JAIL2 
      && maze[y][x].type != CELL_TYPE_JAIL1 && maze[y][x].type != CELL_TYPE_UNBREAKABLE_WALL 
      && maze[y][x].type != CELL_TYPE_HOME2 && maze[y][x].type != CELL_TYPE_HOME1);

    p.x = x;
    p.y = y;
    return p;   

  }

  int playerHello(Proto_Session *s)
  {
    printf("Player %d connected and said hello\n", nextPlayerIndex);
    int playernum = nextPlayerIndex;
    printf("Assigned playernum\n");
    players[playernum].playernum = playernum;
    players[playernum].team = nextTeam;
    Point location = getSpawnLocation(nextTeam);
    players[playernum].location = location;    
    int x = location.x;
    int y = location.y;
    Cell *c = &(maze[y][x]);
    c->playernum = playernum;
    printf("Set player %d at location %d,%d\n", playernum, x, y);

    int rc;
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(s));
    h.type = PROTO_MT_REP_BASE_HELLO;
    proto_session_hdr_marshall(s, &h);
    proto_session_body_marshall_int(s, playernum);
    proto_session_dump(s);
    rc = proto_session_send_msg(s, 1);
    updateEvent();
    nextPlayerIndex++;
    if (nextTeam == 1)
    {
      nextTeam = 2;
    }
    else
    {
      nextTeam = 1;
    }
    return rc;
  }

  int
  main(int argc, char **argv)
  { 

    // Initialize flag locations
    Point p1 = spawnFlag(1); // get location for flag 1
    Point p2 = spawnFlag(2); // get location for flag 2
    flags[0][0] = p1.x;
    flags[0][1] = p1.y;
    flags[1][0] = p2.x;
    flags[1][1] = p2.y;

    // Initialize hammer locations
    hammers[0][0] = HAMMER1X;
    hammers[0][1] = HAMMER1Y;
    hammers[1][0] = HAMMER2X;
    hammers[1][1] = HAMMER2Y;

    //Initialize maze
    int i;
    int j;
    for (i = 0; i < 201; i++)
    {
      for (j = 0; j < 201; j++)
      {

        Cell *c = &(maze[i][j]);
        // encode initial hammer information
        if (i == HAMMER1X && j == HAMMER1Y) // hammer 1 start location
        {
          c->mjolnir.hammerID = 1;
          c->mjolnir.uses = 10;
        }
        else if (i == HAMMER2X && j == HAMMER2Y) // hammer 2 start location
        {
          c->mjolnir.hammerID = 2;
          c->mjolnir.uses = 10;
        }
        else
        {
          c->mjolnir.hammerID = 0;
          c->mjolnir.uses = 0;
        }

        // encode initial flag information
        if (i == flags[0][0] && j == flags[0][1]) // flag 1 start location
        {
          c->flag = 1;
        }
        else if (i == flags[0][1] && j == flags[1][1]) // flag 2 start location
        {
          c->flag = 2;
        }
        else
        {
          c->flag = 0;
        }

        // set initial player number for each cell to 0        
        c->playernum = 0;
        
      }
    }


    printf("Hammer1X at %x\n", hammers[0][0]);
    printf("Hammer1Y at %x\n", hammers[0][1]);

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
    Proto_MT_Handler q = &playerMove;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_MOVE, q);
    Proto_MT_Handler g = &playerGoodbye;
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_GOODBYE, g);

      
    shell(NULL);

    return 0;
  }

