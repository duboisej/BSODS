#ifndef __MAZE_H__
#define __MAZE_H__


typedef enum  {

  // Floor
  CELL_TYPE_FLOOR,
  
  // Wall
  CELL_TYPE_WALL,
  CELL_TYPE_UNBREAKABLE_WALL,
  
  // Jail
  CELL_TYPE_JAIL,
  
  // Home 
  CELL_TYPE_HOME

} Cell_Types;

typedef enum  {

  OBJECT_TYPE_FLAG,
  OBJECT_TYPE_HAMMER

} Object_Types;

typedef struct Cell
{
	Cell_Types type;
	Object_Types contains;
};

typedef struct Maze
{
	Cell maze[200][200];
	
};

typedef struct Player
{

};