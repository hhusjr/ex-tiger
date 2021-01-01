#ifndef TIGER_LIVENESS
#define TIGER_LIVENESS

#include "graph.h"
#include "temp.h"

typedef struct LV_moveList_ *LV_moveList;
struct LV_moveList_ {
    G_node src, dst;
    LV_moveList tail;
};
LV_moveList LV_MoveList(G_node src, G_node dst, LV_moveList tail);

typedef struct LV_graph_ LV_graph;
struct LV_graph_ {
    G_graph graph;
    LV_moveList moves;
};

Temp_temp LV_gtemp(G_node n);

LV_graph LV_liveness(G_graph flow);

#endif
