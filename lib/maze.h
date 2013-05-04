// maze.h
// enums and typedefs for the maze



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



