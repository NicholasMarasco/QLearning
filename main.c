/* Nicholas Marasco
 * CIS 421 - AI
 * Assignment Q-learning Phase 2
 * Due 12/07/16
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "qlearn.h"

#define MAX_LINE_SIZE 512

typedef enum {NO, NE, EA, SE, SO, SW, WE, NW} action;

// update functions
void epoch(env_t*,FILE*,int);
action decide();
float apply(env_t*,action);
cell_t* getPos(env_t,action);
void updateEnv(env_t*,cell_t*);

// building functions
void resetBoard(cell_t**,finput_t);
cell_t** buildMap(finput_t);
finput_t handleFile(char*);
void placeBurglar(env_t*);

// debug/printing functions
void draw(env_t,FILE*);
void print_finput(finput_t);
int getRand(int);

// destroying functions
void burnMap(env_t);
void free_finput(finput_t);

int main(int ARGC,char *ARGV[]){
  finput_t fileInputs;
  env_t env;
  int epochNum;

  srand(time(NULL));

  if(--ARGC != 2){
    printf("Error: invalid number of arguments: %d\n",ARGC);
    printf("usage: %s <file-name> <number-of-epochs>\n",ARGV[0]);
    return 1;
  }
  else{
    fileInputs = handleFile(ARGV[1]);
    epochNum = atoi(ARGV[2]);
  }
//   print_finput(fileInputs);
  env.mSize = fileInputs.n;

  FILE *fout = fopen("output.txt","a");
  env.map = buildMap(fileInputs);

  // epoch loop
  int i;
  for(i = 0; i < epochNum-1; i++){
    resetBoard(env.map,fileInputs);
    placeBurglar(&env);
    env.pNum = fileInputs.pNum;
    env.pRescue = 0;

    epoch(&env,fout,0);
  }

  resetBoard(env.map,fileInputs);
  placeBurglar(&env);
  draw(env,stdout);
  draw(env,fout);
  env.pNum = fileInputs.pNum;
  env.pRescue = 0;
  epoch(&env,fout,1);

  draw(env,stdout);
  draw(env,fout);
  fprintf(stdout,"Ponies: %d/%d\n\n",env.pRescue,env.pNum);
  fprintf(fout,"Ponies: %d/%d\n\n",env.pRescue,env.pNum);

  fclose(fout);
  free_finput(fileInputs);
  burnMap(env);

  return 0;
}

// do a complete run of an epoch
// parameters:
//  env - environment
//  fout - output file
//  last - are we on last epoch
// returns: nothing
void epoch(env_t *env, FILE* fout, int last){

  float totalR = 0;
  float r = 0;
  int run = 1;

  if(env->doa) run = 0;

  while(run){

//     if(last) printf("We're deciding\n");

    action next = decide(*env, last);

//     if(last) printf("We decided\n");

    r = apply(env,next);
//     printf("r: %f\n",r);
    if(r == 15 || r == -15){
      run = 0;
    }
    totalR += r;
  }

}

// returns a random action
action getRandomAction(){
  action next;
  int r = getRand(8);

  switch(r){
    case 0: return next = NO;
    case 1: return next = NE;
    case 2: return next = EA;
    case 3: return next = SE;
    case 4: return next = SO;
    case 5: return next = SW;
    case 6: return next = WE;
    case 7: return next = NW;
  }

}

// find and store max Q value
// parameters:
//  pos - position to find max of
// returns: nothing
void findMaxQ(cell_t *pos){
  int i;
  pos->maxQ = 0.0;

  for(i = 0; i < 8; i++){
    if(pos->Q[i] > pos->maxQ){
      pos->maxQ = pos->Q[i];
    }
  }
}

// decide random direction to go for now
// parameters:
//  env - environment
//  last - last epoch
// returns: action decision
action decide(env_t env, int last){

  action next;

  if(!last &&
    ((EXPLORE_RATE > getSRand()) || (env.map[env.bX][env.bY].sumQ == 0.0))){
    return getRandomAction(); // Explore
  }
  else{
    findMaxQ(&env.map[env.bX][env.bY]);
    for(next = 0; next < 8; next++){
      if(env.map[env.bX][env.bY].maxQ == env.map[env.bX][env.bY].Q[next]){
        return next;
      }
    }
  }

}

// update sum of Q values at position
// parameters:
//  pos - position to update
// returns: nothing
void updateSum(cell_t *pos){
  int i;
  pos->sumQ = 0.0;

  for(i = 0; i < 8; i++)
    pos->sumQ += pos->Q[i];
}

// update Q values
// parameters:
//  env - pointer to environment
//  newPos - new position of burglar
//  next - action leading to newPos
// returns: reward received
float updateFunction(env_t *env, cell_t *newPos, action next){
  float reward = 0.0;

  if(newPos->x == env->bX && newPos->y == env->bY){
    return reward;
  }

  switch(newPos->val){
    case 'P':
      reward += 10.0;
      env->pRescue++;
      break;
    case 'E':
      reward += 15.0;
      break;
    case 'T':
      reward -= 15.0;
      break;
    default:
      reward += 2.0;
  }

  findMaxQ(newPos);

  int cellCount = env->map[env->bX][env->bY].visit;
  float localAlpha = LEARNING_RATE;
  localAlpha -= cellCount * 0.01;
  if(localAlpha < 0.1){
    localAlpha = 0.1;
  }

  env->map[env->bX][env->bY].Q[next] += localAlpha *
    (reward + (DISCOUNT_RATE * newPos->maxQ) -
     env->map[env->bX][env->bY].Q[next]);

  updateSum(&env->map[env->bX][env->bY]);

  return reward;
}



// apply the given action to the environment
// parameters:
//  env - environment
//  next - action to apply
// returns: reward value
float apply(env_t *env, action next){

  float reward;
  cell_t *newPos = getPos(*env, next);

  reward = updateFunction(env, newPos, next);

  if(reward != 0.0) updateEnv(env, newPos);
  return reward;
}

// get the cell associated with action
// parameters:
//  env - environment
//  next - action to be applied
// returns: cell of next action
cell_t* getPos(env_t env, action next){

//   printf("p: %d %d\na: %d\n",env.bPos.x,env.bPos.y,next);

  cell_t *pos = &env.map[env.bX][env.bY];

  switch(next){
    case NO:
      if(env.bX+1 != env.mSize)
        pos = &env.map[env.bX+1][env.bY];
      break;
    case NE:
      if(env.bX+1 != env.mSize && env.bY-1 >= 0)
        pos = &env.map[env.bX+1][env.bY-1];
      break;
    case EA:
      if(env.bY-1 >= 0)
        pos = &env.map[env.bX][env.bY-1];
      break;
    case SE:
      if(env.bX-1 >= 0 && env.bY-1 >= 0)
        pos = &env.map[env.bX-1][env.bY-1];
      break;
    case SO:
      if(env.bX-1 >= 0)
        pos = &env.map[env.bX-1][env.bY];
      break;
    case SW:
      if(env.bX-1 >= 0 && env.bY+1 != env.mSize)
        pos = &env.map[env.bX-1][env.bY+1];
      break;
    case WE:
      if(env.bY+1 != env.mSize)
        pos = &env.map[env.bX][env.bY+1];
      break;
    case NW:
      if(env.bX+1 != env.mSize && env.bY+1 != env.mSize)
        pos = &env.map[env.bX+1][env.bY+1];
  }

//   printf("p: %d %d\n",pos.x,pos.y);

  if(pos->val == 'X'){
    return &env.map[env.bX][env.bY];
  }
  return pos;
}

// update the map with the next action
// parameters:
//   env - environment
//   new - new burglar position
// returns: nothing
void updateEnv(env_t *env, cell_t *new){
  env->map[env->bX][env->bY].val = 'o';

  env->map[new->x][new->y].val = 'B';
  env->bX = new->x;
  env->bY = new->y;
  env->map[env->bX][env->bY].visit++;
}

// reset board positions
// parameters:
//  map - map
//  fileInputs - file inputs
// returns nothing
void resetBoard(cell_t **map, finput_t fileInputs){
  int i,j;
  for(i = 0; i < fileInputs.n; i++){
    for(j = 0; j < fileInputs.n; j++){
      map[i][j].val = '-';
      map[i][j].visit = 0;
    }
  }

  map[fileInputs.escape.x][fileInputs.escape.y].val = 'E';

  // put in ponies
  cell_t *local_pList = fileInputs.pList;
  for(i = 0; i < fileInputs.pNum; i++)
    map[local_pList[i].x][local_pList[i].y].val = 'P';

  // put in trolls
  cell_t *local_tList = fileInputs.tList;
  for(i = 0; i < fileInputs.tNum; i++)
    map[local_tList[i].x][local_tList[i].y].val = 'T';

  // put in obstructions
  cell_t *local_oList = fileInputs.oList;
  for(i = 0; i < fileInputs.oNum; i++)
    map[local_oList[i].x][local_oList[i].y].val = 'X';

}

// take the file inputs and build the initial map
//   parameters:
//     fileInputs - struct containing the file inputs
//   returns:
//     2D array representation of the map
cell_t** buildMap(finput_t fileInputs){

  cell_t **map = malloc(fileInputs.n * sizeof(cell_t*));

  int i;
  for(i = 0; i < fileInputs.n; i++)
    map[i] = malloc(fileInputs.n * sizeof(cell_t));

  int j;
  for(i = 0; i < fileInputs.n; i++){
    for(j = 0; j < fileInputs.n; j++){
      cell_t cell;
      cell.x = i;
      cell.y = j;
      cell.visit = 0;
      cell.val = '-';
      cell.sumQ = 0.0;
      cell.maxQ = 0.0;
      int k;
      for(k = 0; k < 8; k++)
        cell.Q[k] = 0.0;
      memcpy(&map[i][j],&cell,sizeof(cell_t));
    }
  }

  // put in escape
  map[fileInputs.escape.x][fileInputs.escape.y] = fileInputs.escape;

  // put in ponies
  cell_t *local_pList = fileInputs.pList;
  for(i = 0; i < fileInputs.pNum; i++)
    map[local_pList[i].x][local_pList[i].y] = local_pList[i];

  // put in trolls
  cell_t *local_tList = fileInputs.tList;
  for(i = 0; i < fileInputs.tNum; i++)
    map[local_tList[i].x][local_tList[i].y] = local_tList[i];

  // put in obstructions
  cell_t *local_oList = fileInputs.oList;
  for(i = 0; i < fileInputs.oNum; i++)
    map[local_oList[i].x][local_oList[i].y] = local_oList[i];

  return map;

}

// read in the file and grab the input information
//   parameters:
//     fileName - name of the input file
//   returns:
//     struct finput_t - contains all of the file input data
finput_t handleFile(char *fileName){
  FILE *fp;
  char buf[MAX_LINE_SIZE];

  fp = fopen(fileName,"r");
  if(fp == NULL){
    perror("Error: ");
    exit(-1);
  }

  int n, tNum, pNum;
  int p_X, p_Y;
  cell_t *pList;
  cell_t *oList;
  cell_t *tList;

  int cellSize = sizeof(cell_t);

  // get first line from file
  fgets(buf,MAX_LINE_SIZE,fp);
  sscanf(buf," %d %d %d",&n,&tNum,&pNum);

  // initialize lists
  pList = malloc(pNum*cellSize);
  tList = malloc(tNum*cellSize);

  // oList will include, at worst, every cell in the grid
  cell_t *temp_oList = malloc((n*n)*cellSize);

  // get escape coords
  fgets(buf,MAX_LINE_SIZE,fp);
  sscanf(buf," %d %d",&p_Y,&p_X);

  cell_t escape = {p_X,p_Y,'E'};

  // get pony locations
  char const *place;
  int offset;
  int i = 0;
  fgets(buf,MAX_LINE_SIZE,fp);
  for(place = buf;
      sscanf(place," %d %d%n",&p_Y,&p_X,&offset) == 2; place += offset){

    cell_t temp = {p_X,p_Y,'P'};
    memcpy(&pList[i++],&temp,cellSize);
  }

  // get obstruction locations
  int oNum = 0;
  fgets(buf,MAX_LINE_SIZE,fp);
  for(place = buf;
      sscanf(place," %d %d%n",&p_Y,&p_X,&offset) == 2; place += offset){

    if(p_X == -1 && p_Y == -1){
      oList = malloc(sizeof(cell_t));
      cell_t temp = {-1,-1,'X'};
      memcpy(oList,&temp,cellSize);
    }
    else{
      cell_t temp = {p_X,p_Y,'X'};
      memcpy(&temp_oList[oNum++],&temp,cellSize);
    }
  }
  // get actual oList, if needed
  if(oNum){
    int size = oNum * sizeof(cell_t);
    oList = malloc(size);
    memcpy(oList,temp_oList,size);
  }
  free(temp_oList);

  // get troll list
  i = 0;
  fgets(buf,MAX_LINE_SIZE,fp);
  for(place = buf;
      sscanf(place," %d %d%n",&p_Y,&p_X,&offset) == 2; place += offset){

    cell_t temp = {p_X,p_Y,'T'};
    memcpy(&tList[i++],&temp,cellSize);
  }
  fclose(fp);

  finput_t fileInputs = {n,pNum,oNum,tNum,escape,pList,oList,tList};
  return fileInputs;

}

// put burglar in random location
// parameters:
//  env - environment
// returns: nothing
void placeBurglar(env_t *env){

  int x, y;
  int n = env->mSize;

  do{
    x = getRand(n);
    y = getRand(n);
  } while( env->map[x][y].val == 'X' );

  if(env->map[x][y].val == 'T' || env->map[x][y].val == 'E')
    env->doa = 1;
  else
    env->doa = 0;

  env->map[x][y].val = 'B';
  env->bX = x;
  env->bY = y;

}

// draw the current board
//  parameters:
//    map - 2D representation of map
//    n - size of the map
//  returns: nothing
void draw(env_t env, FILE* out){

  int i, j;
  int n = env.mSize;

  for(i = 0; i < n; i++)
    fprintf(out,"+-");
  fprintf(out,"+\n");

  for(i = n-1; i >= 0; i--){
    fprintf(out,"|");
    for(j = 0; j < n-1; j++)
      fprintf(out,"%c ",env.map[i][j].val);
    fprintf(out,"%c|\n",env.map[i][n-1].val);

    if(i){
      for(j = 0; j < n; j++)
        fprintf(out,"+ ");
      fprintf(out,"+\n");
    }

  }

  for(i = 0; i < n; i++)
    fprintf(out,"+-");
  fprintf(out,"+\n");
}

// print out the file input variables
// parameters:
//   fileInputs - structure containing all of the file input values
void print_finput(finput_t fileInputs){
  int i;

  printf("File Inputs: \n");
  printf("  n: %d\n",fileInputs.n);
  printf("  escape: (%d, %d, %c)\n",fileInputs.escape.x,
                                    fileInputs.escape.y,
                                    fileInputs.escape.val);

  printf("  pNum : %d\n",fileInputs.pNum);
  printf("  pList: ");
  cell_t *pList = fileInputs.pList;
  for(i = 0; i < fileInputs.pNum; i++){
    printf("(%d, %d, %c) ",pList[i].x,pList[i].y,pList[i].val);
  }
  printf("\n");

  printf("  oNum : %d\n",fileInputs.oNum);
  printf("  oList: ");
  cell_t *oList = fileInputs.oList;
  for(i = 0; i < fileInputs.oNum; i++){
    printf("(%d, %d, %c) ",oList[i].x,oList[i].y,oList[i].val);
  }
  printf("\n");

  printf("  tNum : %d\n",fileInputs.tNum);
  printf("  tList: ");
  cell_t *tList = fileInputs.tList;
  for(i = 0; i < fileInputs.tNum; i++){
    printf("(%d, %d, %c) ",tList[i].x,tList[i].y,tList[i].val);
  }
  printf("\n");
}

// get random number in range
// parameters:
//  max - top bound on number
// returns: number in bounds [0,max)
int getRand(int max){
  unsigned int x = (RAND_MAX + 1u) / max;
  unsigned int y = x * max;
  unsigned r;
  do{
    r = rand();
  } while( r >= y );
  return r / x;
}

// destroy the map
// parameters:
//   map - current state of the map
//   n - size of the map
void burnMap(env_t env){
  int i;
  for(i = 0; i < env.mSize; i++)
    free(env.map[i]);
  free(env.map);
}

// free file inputs
// parameters:
//  fin - file inputs
// returns: nothing
void free_finput(finput_t fin){
  free(fin.pList);
  free(fin.oList);
  free(fin.tList);
}
