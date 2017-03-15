#ifndef LIVENESS_H
#define LIVENESS_H

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);

typedef struct Live_graph *Live_graphh;
struct Live_graph {
	G_graph cg;
        G_graph fg;
	Live_moveList moves;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps);

Temp_tempList lookupLiveMap(G_table t, G_node flowNode);

void calculateLiveMap(G_table t, G_nodeList nodes);

void buildConflictGraph(G_graph cg, G_graph flow, G_table liveMap);

int findGnode(G_nodeList nodes, G_node node);

G_node getGNode(G_graph g, Temp_temp info);

void showTemp(void* info);

Live_moveList delete_move(G_node src,G_node dst, Live_moveList mlist);

Live_moveList union_move(Live_moveList l1, Live_moveList l2);

Live_moveList intersect_move(Live_moveList l1, Live_moveList l2);

bool inMoveList(G_node src, G_node dst, Live_moveList l);

#endif
