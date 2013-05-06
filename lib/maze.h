// maze.h
// enums and typedefs for the maze

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <poll.h>
#include "../lib/types.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"

int isJail1Cell(int x, int y);
int isJail2Cell(int x, int y);
int isHome1Cell(int x, int y);
int isHome2Cell(int x, int y);

typedef enum  {

  // Floor
  CELL_TYPE_FLOOR,
  
  // Wall
  CELL_TYPE_WALL,
  CELL_TYPE_UNBREAKABLE_WALL,
  
  // Jail
  CELL_TYPE_JAIL1,
  CELL_TYPE_JAIL2,
  
  // Home 
  CELL_TYPE_HOME1,
  CELL_TYPE_HOME2

} Cell_Types;

typedef struct {
  int hammerID;
  int uses;
} Hammer;

typedef struct
{
  Hammer mjolnir; 
	Cell_Types type; 
  int flag; 
  int playernum; 
} Cell; 

typedef struct 
{
  int x;
  int y;
} Point;

typedef struct {
  Point location;
  Hammer mjolnir;
  int playernum;
  int team;
  int flag;
  int jail;
} Player;



