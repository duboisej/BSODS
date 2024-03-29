  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <poll.h>
  #include "../lib/types.h"
  #include "../lib/maze.h"
  #include "../lib/protocol_server.h"
  #include "../lib/protocol_utils.h"

  // Players can't spawn on either of the hammer's home locations.
  #define HAMMER1X 95
  #define HAMMER1Y 10
  #define HAMMER2X 95
  #define HAMMER2Y 190

  Cell maze[201][201];

  Player players[300];

  int hammers[2][2];
  int flags[2][2];

  int nextPlayerIndex = 0;
  int numPlayers;
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

            if (i == 0 || j == 0 || i == 199 || j == 199 || c=='$')
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
                maze[i][j].type = CELL_TYPE_JAIL2;
                numjail1++;
            }
            else if (c == 'j')
            {
                maze[i][j].type = CELL_TYPE_JAIL1;
                numjail2++;
            }
            else if (c == 'H') 
            {
                maze[i][j].type = CELL_TYPE_HOME2;
                numhome1++;
            }
            else if (c == 'h')
            {
                maze[i][j].type = CELL_TYPE_HOME1;
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
    printf("There are %d current players.\n"), numPlayers;
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
                printf("f");
              }
              else if (c.flag == 1)
              {
                printf("F");
              }
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
                printf("#");
            }
            else if (celltype == CELL_TYPE_UNBREAKABLE_WALL)
            {
                printf("$");
            }
            

        }
        printf("\n");
    }
    return 1;
}

  int 
  updateEvent(void)
  {
    printf("updateEvent called.\n");
    Proto_Session *s;
    Proto_Msg_Hdr hdr;

    s = proto_server_event_session();

    printf("Printing connected players for update\n");

    hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
    proto_session_hdr_marshall(s, &hdr);

    // Marshall player information

    printf("Marshalling number of players\n");
    printf("Number of players is currently %d\n", numPlayers);
    // Send over number of players

    proto_session_body_marshall_int(s, numPlayers);

    printf("Marshalling players\n");
    // Marshall Player info
    int k;
    for (k = 1; k < numPlayers+1; k++)
    {
      Player *p = &(players[k]);
      proto_session_body_marshall_player(s, p);
    }

    // Marshall Flag info
    printf("Marshalling flag locations\n");
    proto_session_body_marshall_int(s, flags[0][0]);
    proto_session_body_marshall_int(s, flags[0][1]);
    proto_session_body_marshall_int(s, flags[1][0]);
    proto_session_body_marshall_int(s, flags[1][1]);

    // Marshall Hammers
    printf("Marshalling hammer locations\n");
    proto_session_body_marshall_int(s, hammers[0][0]);
    proto_session_body_marshall_int(s, hammers[0][1]);
    proto_session_body_marshall_int(s, hammers[1][0]);
    proto_session_body_marshall_int(s, hammers[1][1]);

    // Marshall maze (ASCII)

    int i;
    int j;
    int celltype;

    for (i = 0; i < 200; i++)
    {
        for (j = 0; j < 200; j++)
        {
            Cell c = maze[i][j];
            Cell_Types celltype = c.type;

            // if (i == 100 && j == 17)
            // {
            //   printf("Cell type at location (%d, %d) is: %d\n", i, j, celltype);
            // }

            if (c.mjolnir.hammerID == 1)
            {
              proto_session_body_marshall_char(s, 'm');
            }
            else if (c.mjolnir.hammerID == 2)
            {
              proto_session_body_marshall_char(s, 'M');
            }
            else if (c.flag == 1)
            {
              printf("found flag 1 in updateEvent\n");
              proto_session_body_marshall_char(s, 'f');
            }
            else if (c.flag == 2)
            {
              printf("found flag 2 in updateEvent\n");
              proto_session_body_marshall_char(s, 'F');
            }
            else if (celltype == CELL_TYPE_HOME2)
            {
                proto_session_body_marshall_char(s, 'H');
                //printf("H");
            }
            else if (celltype == CELL_TYPE_HOME1)
            {
                proto_session_body_marshall_char(s, 'h');
                //printf("h");
            }
            else if (celltype == CELL_TYPE_JAIL2)
            {
                proto_session_body_marshall_char(s, 'J');
                //printf("J");
            }
            else if (celltype == CELL_TYPE_JAIL1)
            {
                proto_session_body_marshall_char(s, 'j');
                //printf("j");
            } 
            else if (celltype == CELL_TYPE_FLOOR)
            {
              // if (i == 100 && j == 17)
              //   {
              //     printf("Found floor at 100, 17\n");
              //   }
                proto_session_body_marshall_char(s, ' ');
            }
            else if (celltype == CELL_TYPE_WALL)
            {
                // if (i == 100 && j == 17)
                // {
                //   printf("Found wall at 100, 17\n");
                // }
                proto_session_body_marshall_char(s, '#');
                //printf("#");
            }
            else if (celltype == CELL_TYPE_UNBREAKABLE_WALL)
            {
              //if (i == 0) printf("Got left side unbreakable wall\n");
                //printf("marshalled unbreakable cell\n");
                proto_session_body_marshall_char(s, '$');
            }
            

        }
        proto_session_body_marshall_char(s, '\n');
    }
    proto_session_body_marshall_char(s, 'z'); // marks end of maze

    proto_session_dump(s);
    proto_server_post_event(); 
    //dumpMap(); 
    dumpPlayers();
  }

  // int 
  // moveEvent(int x, int y, int playernum)
  // {
  //   printf("moveEvent called.\n");
  //   Proto_Session *s;
  //   Proto_Msg_Hdr hdr;

  //   s = proto_server_event_session();
  //   hdr.type = PROTO_MT_EVENT_BASE_MOVE;
  //   proto_session_hdr_marshall(s, &hdr);
  //   proto_session_body_marshall_int(s, x);
  //   proto_session_body_marshall_int(s, y);
  //   proto_session_body_marshall_int(s, playernum);
  //   proto_session_dump(s);
  //   proto_server_post_event();  
  // }

  // int 
  // breakWallEvent(int x, int y)
  // {
  //   printf("breakWallEvent called.\n");
  //   Proto_Session *s;
  //   Proto_Msg_Hdr hdr;

  //   s = proto_server_event_session();
  //   hdr.type = PROTO_MT_EVENT_BASE_MOVE;
  //   proto_session_hdr_marshall(s, &hdr);
  //   proto_session_body_marshall_int(s, x);
  //   proto_session_body_marshall_int(s, y);
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
    case 'm':
      rc = dumpMap();
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


  /* CheckMove - returns -1 if invalid move (wall/unbreakable wall cell. If there's 
    another player at the new location, returns that player's number. If a valid move,
    to an empty floor/home/jail cell, returns 0 */
  int
  checkMove(int playernum, Cell *new_location)
  {
    Cell_Types newType = new_location->type;
    // Check cell type
    if (newType == CELL_TYPE_UNBREAKABLE_WALL)
    {
      return -1;
    }
    else if (newType == CELL_TYPE_WALL)
    {
      // player is trying to move into wall
      int hammer = players[playernum].mjolnir.hammerID;
      int uses = players[playernum].mjolnir.uses;
      if (hammer == 0)
      {
        return -1;
      }
      else if (hammer == 1 && uses > 0)
      {
        // player has hammer 1
        return 301; //? 

      }
      else if (hammer == 2 && uses > 0)
      {
        // player has hammer 1
        return 302;
      }
      else
      {
        return -1;
      }
    }
    else if (newType == CELL_TYPE_FLOOR && players[playernum].jail != 0)
    {
      // player is in jail and is trying to move out of jail
      return -1;
    }
    else
    {
      // Check if there's another player there
      int otherPlayer = new_location->playernum;
      if (otherPlayer != 0)
      {
        // There's another player there
        printf("Player %d is there!\n", otherPlayer);
        if (players[otherPlayer].team == players[playernum].team) {
          // player bumps into teammate
          printf("Player %d tried to tag teammate #%d!\n", playernum, otherPlayer);
          return -1;
        } else {
          // player tags player from other team
          printf("Player %d is trying to tag player #%d!\n", playernum, otherPlayer);
          return otherPlayer;
        } 
      }
      else
      {
        // All good.
        return 0;
      }
    }
    
  }

  int 
  getLocationInRange(int min, int max)
  {
    //srand(time(NULL));
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
      c = &(maze[x][y]);
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

    // Note that x and y are flipped since the maze indexing accesses row first and then column
    Point p;
    p.x = y;
    p.y = x;
    return p;
  }

  Point jailSpawn(int playernum)
  {
    int team = (playernum+1) % 2; // Opposite team's jail
    int x;
    int y;
    Cell *c;
    do
    {
      c = &(maze[x][y]);
      if (team == 1)
      {
        x = getLocationInRange(90, 97);
      }
      else 
      {
        x = getLocationInRange(102, 109);
      }
      y = getLocationInRange(90, 108);

    }
    while(c->playernum != 0 && y != 95 && x != 190 && x != 10);

    // Note that x and y are flipped since the maze indexing accesses row first and then column
    Point p;
    p.x = y;
    p.y = x;
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
      y = getLocationInRange(xmin, xmax);
      x = getLocationInRange(ymin, ymax);
    } 
    while (maze[x][y].type != CELL_TYPE_FLOOR && maze[x][y].playernum == 0);

    // Note that x and y are flipped since the maze indexing accesses row first and then column
    p.x = x;
    p.y = y;
    return p;   

  }


  // int
  // tagPlayer(int playernum)
  // {
  //   //printf("issues1");
  //   Player *p = &(players[playernum]);
  //   //printf("issues2");
  //   int playerTeam = p->team;
  //   //printf("issues3");
  //   Point *curr_location = &(p->location);
  //   int x = curr_location->x;
  //   int y = curr_location->y;
  //   Cell *old_location = &(maze[x][y]);
  //   Point jailLocation = jailSpawn(playernum);
  //   Cell *new_location = &(maze[jailLocation.x][jailLocation.y]);

  //   // Update player location
  //   curr_location->x = jailLocation.x;
  //   curr_location->y = jailLocation.y;

  //   // Remove player from old location in maze
  //   old_location->playernum = 0;

  //   // Add player to new location in maze
  //   new_location->playernum = playernum;
  //   return 1;
  // }

  int
  freePlayers(int jail)
  {
    int i;
    for (i = 1; i < numPlayers+1; i++)
    {
      Player *p = &(players[i]);
      if (p->jail == jail)
      {
        //printf("Freeing player %d\n", i);
        p->jail = 0;
        printf("Set jail to 0\n");
        Point old_location = p->location;
        printf("Grabbed old(current) location\n");
        maze[old_location.x][old_location.y].playernum = 0; // remove player from jail cell
        printf("Removed player from jail cell\n");
        Point location = getSpawnLocation(p->team);
        printf("Got new spawn location");
        p->location = location;  
        printf("Set new location\n");

        // Add player number to new cell
        int x = location.x;
        printf("Grabbed x value from new location\n");
        int y = location.y;
        //printf("Transporting player %d back to his home at location (%d, %d)\n", i, x, y);
        Cell *c = &(maze[x][y]);
        c->playernum = i;
      }
    }
  }

  int playerMove(Proto_Session *s)
  {
    int rc; 
    int fd;
    int player;
    Proto_Msg_Hdr h;
    char move = s->rbuf[0];
    int playernum;
    proto_session_body_unmarshall_int(s, 1, &playernum);
    printf("Player %d is moving\n", playernum);
    Player *p = &(players[playernum]);
    int playerTeam = p->team;
    printf("Player %d is on team %d\n", playernum, playerTeam);
    Point *curr_location = &(p->location);
    int x = curr_location->x;
    int y = curr_location->y;
    int newx;
    int newy;
    Cell *old_location = &(maze[x][y]);
    Cell *new_location;
    if (move == 'w')
    {
      newx = x - 1;
      newy = y;
    }
    else if (move == 'a')
    {
      newx = x;
      newy = y-1;
    }
    else if (move == 's')
    {
      newx = x + 1;
      newy = y;
    }
    else if (move == 'd')
    {
      newx = x;
      newy = y+1;
    }
    else
    {
      return -1;
    }
    new_location = &(maze[newx][newy]);
    printf("checking move to location (%d, %d)...\n", newx, newy);
    int valid = checkMove(playernum, new_location); 

    if (valid == 301 || valid == 302)
    {
      // Player is breaking a wall with Hammer 1
      printf("player %d is breaking a wall\n", playernum);
      valid = 0;
      printf("Setting cell at location (%d, %d) to be type floor\n", newx, newy);
      maze[newx][newy].type = CELL_TYPE_FLOOR;
      players[playernum].mjolnir.uses--;
    }

    //printf("Valid = %d!\n", valid);
    int side = 0;
    if (newy > 99) side = 2;
    else side = 1;
    //printf("Valid = %d!\n", valid);

    if (side != playerTeam && valid > 0) {
      valid = playernum;
    }
    
    // || playerTeam != valid%2
    if (valid != -1)
    {
      printf("Valid move!\n");

      // If player is there, tag them first
      if (valid > 0)
      {
        printf("made it into if\n");
        printf("Valid is %d\n", valid);
        tagPlayer(valid);
      }
      else
      {
        curr_location->x = newx;
        curr_location->y = newy;

        // Remove player from old location in maze
        old_location->playernum = 0;

        // Add player to new location in maze
        new_location->playernum = playernum;
      }

      // Check if other team's jail. If so, free players in jail.
      printf("playerTeam is now %d\n", playerTeam);
      if (playerTeam == 1)
      {
        printf("Player is from team 1 (jail check)\n");
        if (new_location->type == CELL_TYPE_JAIL2)
        {
          printf("Player from team 1 is trying to free his teammates.\n");
          freePlayers(2);
        }
      }
      else
      {
        printf("Player is from team 2 (jail check)\n");
        if (new_location->type == CELL_TYPE_JAIL1)
        {
          printf("Player from team 2 is trying to free his teammates.\n");
          freePlayers(1);
        }
      }

      // Get information about hammer/flag in new cell
      int hammerID = new_location->mjolnir.hammerID;
      int flag = new_location->flag;
      int flagIndex = flag - 1;

      // If there's a hammer at the new location, and the player 
      // currently isn't carrying a hammer
      if (hammerID != 0 && p->mjolnir.hammerID == 0)
      {
        // Pick up hammer
        printf("Player %d is picking up hammer %d\n", playernum, hammerID);
        p->mjolnir.hammerID = hammerID;
        p->mjolnir.uses = new_location->mjolnir.uses;

        // Remove hammer from cell
        maze[newx][newy].mjolnir.hammerID = 0;
        maze[newx][newy].mjolnir.uses = 0;

      }
      else if (p->mjolnir.hammerID != 0) // player is currently carrying a hammer
      {
        // move hammer with player (update location in hammers array)
        int p_hammerID = p->mjolnir.hammerID;
        if (p_hammerID == 1)
        {
          // update hammer 1's location
          hammers[0][0] = newx;
          hammers[0][1] = newy;
        }
        else
        {
          hammers[1][0] = newx;
          hammers[1][1] = newy;
        } 
      }

      // If there's a flag at the new location and the player
      // currently isn't carrying a flag
      if (flag != 0 && p->flag == 0)
      {
        // Pick up flag
        p->flag = flag;

        // Remove flag from cell
        maze[newx][newy].flag = 0;
      }
      else if (p->flag != 0) // player is carrying a flag
      {
        // update flag location in flags array
        if (p->flag == 1)
        {
          flags[0][0] = newx;
          flags[0][1] = newy;
        }
        else
        {
          flags[1][0] = newx;
          flags[1][1] = newy;
        }

      }


      // send good reply
      bzero(&h, sizeof(s));
      h.type = PROTO_MT_REP_BASE_MOVE;
      proto_session_hdr_marshall(s, &h);
      proto_session_body_marshall_char(s, 'V'); // 'V' indicates valid move
      rc=proto_session_send_msg(s, 1);
    }
    else
    {
      // send reply with invalid
      bzero(&h, sizeof(s));
      h.type = PROTO_MT_REP_BASE_MOVE;
      proto_session_hdr_marshall(s, &h);
      proto_session_body_marshall_char(s, 'I'); // 'I' in body indicates invalid move
      rc=proto_session_send_msg(s,1);

    }
    
    updateEvent();
    
    return rc;
  }

  
  int
  tagPlayer(int num)
  {
    printf("Tagging player %d\n", num);
    printf("Got into tagplayer\n");
    Player *p = &(players[num]);
    int otherteam;
    if (num % 2 == 1)
    {
      otherteam = 2;
    }
    else
    {
      otherteam = 1;
    }

    // Drop hammer and/or flag if they're carrying them
    if (p->mjolnir.hammerID != 0)
    {
      // Add hammer to current player location (before tag)
      maze[p->location.x][p->location.y].mjolnir.hammerID = p->mjolnir.hammerID;
      maze[p->location.x][p->location.y].mjolnir.uses = p->mjolnir.uses;

      // Make player drop hammer
      p->mjolnir.hammerID = 0;
      p->mjolnir.uses = 0;
    }

    if (p->flag != 0)
    {
      // Add flag to current player location (before tag)
      maze[p->location.x][p->location.y].flag = p->flag;;

      // Make player drop flag
      p->flag = 0;
    }

    p->jail = otherteam; // Other team's jail
    int playerTeam = p->team;
    Point *curr_location = &(p->location);
    int x = curr_location->x;
    int y = curr_location->y;
    Cell *old_location = &(maze[x][y]);
    Point jailLocation = jailSpawn(num);
    int jailx = jailLocation.x;
    int jaily = jailLocation.y;
    Cell *new_location = &(maze[jailx][jaily]);
    // Update player location
    curr_location->x = jailx;
    curr_location->y = jaily;

    // // Remove player from old location in maze
    old_location->playernum = 0;

    // // Add player to new location in maze
    new_location->playernum = num;
    return 1;
  }

  int playerHello(Proto_Session *s)
  {
    printf("Player %d connected and said hello\n", nextPlayerIndex+1);
    int playernum = nextPlayerIndex + 1;
    printf("Assigned playernum\n");

    // Add player and associated info to player array
    players[playernum].playernum = playernum;
    players[playernum].team = nextTeam;
    Point location;
    if (playernum == 1) // test tagging - spawn player 1 on wrong side of the board 
    {
      location.x = 99;
      location.y = 16;
    }
    else if (playernum == 2) // test tagging - spawn player 1 on wrong side of the board 
    {
      location.x = 99;
      location.y = 17;
    }
    else if (playernum == 4)
    {
      location.x = 99;
      location.y = 89;
    }
    else if (playernum == 6)
    {
      location.x = 99;
      location.y = 18;
    }
    else if (playernum == 10)
    {
      location.x = 99;
      location.y = 19;
    }
    else if (playernum == 12)
    {
      location.x = 99;
      location.y = 20;
    }
    else
    {
      location = getSpawnLocation(nextTeam);
    }
    // location = getSpawnLocation(nextTeam);
    players[playernum].location = location;  
    players[playernum].flag = 0;
    players[playernum].mjolnir.hammerID = 0;
    players[playernum].mjolnir.uses = 0; 

    // Add player number to associated cell
    int x = location.x;
    int y = location.y;
    Cell *c = &(maze[x][y]);
    c->playernum = playernum;
    printf("Set player %d at location %d,%d\n", playernum, x, y);

    // Send reply back to player with their playernum
    int rc;
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(s));
    h.type = PROTO_MT_REP_BASE_HELLO;
    proto_session_hdr_marshall(s, &h);
    proto_session_body_marshall_int(s, playernum);
    proto_session_dump(s);
    rc = proto_session_send_msg(s, 1);
    nextPlayerIndex++;
    numPlayers = playernum;
    if (nextTeam == 1)
    {
      nextTeam = 2;
    }
    else
    {
      nextTeam = 1;
    }
    dumpPlayers();
    updateEvent();
    return rc;
  }

  int
  main(int argc, char **argv)
  { 
    // initialize game board
    load();
    // Initialize flag locations
    Point p1 = spawnFlag(1); // get location for flag 1
    printf("Spawned first flag at location (%d,%d)\n", p1.x, p1.y);
    maze[p1.x][p1.y].flag = 1;
    //srand(time(NULL));
    Point p2 = spawnFlag(2); // get location for flag 2
    printf("Spawned second flag at location (%d,%d)\n", p2.x, p2.y);
    maze[p2.x][p2.y].flag = 2;
    flags[0][0] = p1.x;
    flags[0][1] = p1.y;
    flags[1][0] = p2.x;
    flags[1][1] = p2.y;

    // flags[0][0] = 60;
    // flags[0][1] = 60;
    // flags[1][0] = 105;
    // flags[1][1] = 70;

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

        // // encode initial flag information
        // if (i == flags[0][0] && j == flags[0][1]) // flag 1 start location
        // {
        //   printf("found flag 1 location on server\n");
        //   c->flag = 1;
        // }
        // else if (i == flags[1][0] && j == flags[1][1]) // flag 2 start location
        // {
        //   printf("found flag 2 location on server\n");
        //   c->flag = 2;
        // }
        // else
        // {
        //   c->flag = 0;
        // }

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

