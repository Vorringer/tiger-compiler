/*
 * graph.h - Abstract Data Type (ADT) for directed graphs
 */

typedef struct G_graph_ *G_graph;  /* The "graph" type */
typedef struct G_node_ *G_node;    /* The "node" type */
typedef struct G_nodeList_ *G_nodeList;
struct G_graph_ {int nodecount;
		 G_nodeList mynodes, mylast;
	       };

struct G_node_ {
  G_graph mygraph;
  int mykey;
  G_nodeList succs;
  G_nodeList preds;
  void *info;
};
struct G_nodeList_ { G_node head; G_nodeList tail;};

/* Make a new graph */
G_graph G_Graph(void); 
/* Make a new node in graph "g", with associated "info" */
G_node G_Node(G_graph g, void *info);

/* Make a NodeList cell */
G_nodeList G_NodeList(G_node head, G_nodeList tail);

/* Get the list of nodes belonging to "g" */
G_nodeList G_nodes(G_graph g);

/* Tell if "a" is in the list "l" */
bool G_inNodeList(G_node a, G_nodeList l);

/* Make a new edge joining nodes "from" and "to", which must belong
    to the same graph */
void G_addEdge(G_node from, G_node to);

/* Delete the edge joining "from" and "to" */
void G_rmEdge(G_node from, G_node to);

/* Show all the nodes and edges in the graph, using the function "showInfo"
    to print the name of each node */
void G_show(FILE *out, G_nodeList p, void showInfo(void *));

/* Get all the successors of node "n" */
G_nodeList G_succ(G_node n);

/* Get all the predecessors of node "n" */
G_nodeList G_pred(G_node n);

/* Tell if there is an edge from "from" to "to" */
bool G_goesTo(G_node from, G_node n);

/* Tell how many edges lead to or from "n" */
int G_degree(G_node n);

/* Get all the successors and predecessors of "n" */
G_nodeList G_adj(G_node n);

/* Get the "info" associated with node "n" */
void *G_nodeInfo(G_node n);

/* The type of "tables" mapping graph-nodes to information */
typedef struct TAB_table_  *G_table;

/* Make a new table */
G_table G_empty(void);

G_nodeList delete(G_node a, G_nodeList l);
G_nodeList G_union(G_nodeList l1, G_nodeList l2);
G_nodeList G_except(G_nodeList t1, G_nodeList t2);
void G_appendTail(G_node a, G_nodeList * tl);
G_nodeList G_copyList(G_nodeList tl);
G_nodeList reverseList(G_nodeList head);

/* Enter the mapping "node"->"value" to the table "t" */
void G_enter(G_table t, G_node node, void *value);

/* Tell what "node" maps to in table "t" */
void *G_look(G_table t, G_node node);

typedef struct G_edge_ *G_edge;
struct G_edge_{
    G_node src, dst;
};
G_edge G_Edge(G_node, G_node);

typedef struct G_edgeList_ *G_edgeList;
struct G_edgeList_ {
    G_edge head;
    G_edgeList tail;
};
G_edgeList G_EdgeList(G_edge, G_edgeList);

typedef struct counter_ *counter;

struct counter_ {
    int num;
};

counter Counter(int);
