#include "octoMap.h"
#include "octoTree.h"
// #include "config_autofly.h"

//rrtConnect config
#define ITER_MAX 2000
#define MAX_ARRAY_SIZE ITER_MAX
#define MAXRAND TREE_CENTER_X * 2
#define MIN_DISTANCE TREE_RESOLUTION/2
// #if TREE_RESOLUTION >= 2
//     #define MIN_DISTANCE TREE_RESOLUTION/2
// #else
//     #define MIN_DISTANCE 1
// #endif
#define STRIDE TREE_RESOLUTION*2

typedef struct{
    coordinate_t loc;
    short index_parent;
}vertex_t;

typedef struct array_t{
    vertex_t arr[MAX_ARRAY_SIZE];
    short len;
}array_t;

void planning(coordinate_t* X_start,coordinate_t* X_end,octoTree_t *octoTree,octoMap_t *octoMap, array_t* result);
void generate_random_node(coordinate_t* X_rand);
short find_nearest_neighbor(coordinate_t* X_rand,array_t *current);
void steer(coordinate_t *X_near, coordinate_t *X_rand,vertex_t* X_new);
bool obstaclefree(octoTree_t *octoTree, octoMap_t *octoMap, coordinate_t start, coordinate_t end);
BOOL addToArray_vertex(array_t* array, vertex_t* element);
BOOL addToArray_coordinate(array_t *array, coordinate_t* element, short index_p);

// void writefile(array_t* array, char* filename,int flag);
