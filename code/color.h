/*
 * color.h - Data structures and function prototypes for coloring algorithm
 *             to determine register allocation.
 */

#ifndef COLOR_H
#define COLOR_H

typedef struct String_list_ *String_list;
struct String_list_ {string str; String_list tail;};

String_list String_List(string str, String_list tail);



String_list delete_string(string str,String_list sl);
void initTempNode(G_graph cg);
void initAdjSet(int n);
void initAdjList(int n);
void initDegree(int n);
void init(int n,Temp_map initial,Temp_tempList regs);
G_nodeList reverseList(G_nodeList nlist);
void Build(G_graph fg, G_graph cg);
void AddEdge(G_node src, G_node dst);
void MakeWorklist(G_graph graph);
G_nodeList Adjacent(G_node n);
AS_instrList NodeMoves(G_node n);
bool MoveRelated(G_node n);
void Simplify();
void DecrementDegree(G_node m);
void EnableMoves(G_nodeList nlist);
void Coalesce();
void AddWorkList(G_node u);
bool OK(G_node t, G_node r);
bool Conservative(G_nodeList nlist);
G_node GetAlias(G_node n);
void Combine(G_node u, G_node v);
void Freeze();
void FreezeMoves(G_node u);
void SelectSpill();
String_list getAllColors();
void AssignColors();

typedef struct COL_result *COL_resultt;
struct COL_result {Temp_map coloring; Temp_tempList spills;};
struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs);

#endif




