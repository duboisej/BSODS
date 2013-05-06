// maze.c 
// Capture the Flag

  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <poll.h>
  #include "../lib/types.h"
  #include "../lib/protocol_server.h"
  #include "../lib/protocol_utils.h"

// Functions here
int
isJail1Cell(int x, int y)
{
  return ((x > 89 && x < 109) && (y > 89 && y < 98));
}

int
isJail2Cell(int x, int y)
{
  return ((x > 89 && x < 109) && (y > 101 && y < 110));
}

int 
isHome1Cell(int x, int y)
{
  return ((x > 89 && x < 109) && (y > 1 && y < 12));
}

int
isHome2Cell(int x, int y)
{
  return ((x > 89 && x < 109) && (y > 187 && y < 198));
}