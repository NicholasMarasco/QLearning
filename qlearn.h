#include <stdlib.h>

#define EXPLORE_RATE 0.3
#define LEARNING_RATE 0.5
#define DISCOUNT_RATE 0.5

#define getSRand() ((double)rand() / (double)RAND_MAX)

// structure to contain cell data
typedef struct __cell_t
{
  int x, y, visit;
  char val;
  double Q[8];
  double sumQ;
  double maxQ;
} cell_t;

// structure to contain environment stuff
typedef struct __env_t
{
  int mSize,bX,bY;
  int doa : 1;
  int pRescue, pNum;
  cell_t **map;
} env_t;

// structure to hold input file data
typedef struct __finput_t
{
  int n, pNum, oNum, tNum;
  cell_t escape;
  cell_t *pList;
  cell_t *oList;
  cell_t *tList;
} finput_t;
