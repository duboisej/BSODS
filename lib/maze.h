


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

typedef enum  {

  OBJECT_TYPE_FLAG,
  OBJECT_TYPE_HAMMER

} Object_Types;

typedef struct
{
	Cell_Types type;
	Object_Types contains;
} Cell; 

