/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "errormsg.h"
#include "table.h"


G_graph G_Graph(void)
{G_graph g = (G_graph) checked_malloc(sizeof *g);
 g->nodecount = 0;
 g->mynodes = NULL;
 g->mylast = NULL;
 return g;
}

G_nodeList G_NodeList(G_node head, G_nodeList tail)
{G_nodeList n = (G_nodeList) checked_malloc(sizeof *n);
 n->head=head;
 n->tail=tail;
 return n;
}

/* generic creation of G_node */
G_node G_Node(G_graph g, void *info)
{G_node n = (G_node)checked_malloc(sizeof *n);
 G_nodeList p = G_NodeList(n, NULL);
 assert(g);
 n->mygraph=g;
 n->mykey=g->nodecount++;

 if (g->mylast==NULL)
   g->mynodes=g->mylast=p;
 else g->mylast = g->mylast->tail = p;

 n->succs=NULL;
 n->preds=NULL;
 n->info=info;
 return n;
}

G_nodeList G_nodes(G_graph g)
{
  assert(g);
  return g->mynodes;
} 

/* return true if a is in l list */
bool G_inNodeList(G_node a, G_nodeList l) {
  G_nodeList p;
  for(p=l; p!=NULL; p=p->tail)
    if (p->head==a) return TRUE;
  return FALSE;
}

void G_addEdge(G_node from, G_node to) {
  assert(from);  assert(to);
  assert(from->mygraph == to->mygraph);
  if (G_goesTo(from, to)) return;
  to->preds=G_NodeList(from, to->preds);
  from->succs=G_NodeList(to, from->succs);
}

G_nodeList delete(G_node a, G_nodeList l) {
  assert(a && l);
  if (a==l->head) return l->tail;
  else return G_NodeList(l->head, delete(a, l->tail));
}

void G_rmEdge(G_node from, G_node to) {
  assert(from && to);
  to->preds=delete(from,to->preds);
  from->succs=delete(to,from->succs);
}

 /**
  * Print a human-readable dump for debugging.
  */
void G_show(FILE *out, G_nodeList p, void showInfo(void *)) {
  for (; p!=NULL; p=p->tail) {
    G_node n = p->head;
    G_nodeList q;
    assert(n);
    if (showInfo) 
      showInfo(n->info);
    fprintf(out, " (%d): ", n->mykey); 
    for(q=G_succ(n); q!=NULL; q=q->tail) 
           fprintf(out, "%d ", q->head->mykey);
    fprintf(out, "\n");
  }
}

G_nodeList G_succ(G_node n) { assert(n); return n->succs; }

G_nodeList G_pred(G_node n) { assert(n); return n->preds; }

bool G_goesTo(G_node from, G_node n) {
  return G_inNodeList(n, G_succ(from));
}

/* return length of predecessor list for node n */
static int inDegree(G_node n)
{ int deg = 0;
  G_nodeList p;
  for(p=G_pred(n); p!=NULL; p=p->tail) deg++;
  return deg;
}

/* return length of successor list for node n */
static int outDegree(G_node n)
{ int deg = 0;
  G_nodeList p; 
  for(p=G_succ(n); p!=NULL; p=p->tail) deg++;
  return deg;
}

int G_degree(G_node n) {return inDegree(n)+outDegree(n);}

/* put list b at the back of list a and return the concatenated list */
static G_nodeList cat(G_nodeList a, G_nodeList b) {
  if (a==NULL) return b;
  else return G_NodeList(a->head, cat(a->tail, b));
}

/* create the adjacency list for node n by combining the successor and 
 * predecessor lists of node n */
G_nodeList G_adj(G_node n) {return cat(G_succ(n), G_pred(n));}

void *G_nodeInfo(G_node n) {return n->info;}

/* G_node table functions */

G_table G_empty(void) {
  return TAB_empty();
}

void G_enter(G_table t, G_node node, void *value)
{
  TAB_enter(t, node, value);
}

void *G_look(G_table t, G_node node)
{
  return TAB_look(t, node);
}

G_edge G_Edge(G_node src, G_node dst) {
  G_edge e = (G_edge)checked_malloc(sizeof(*e));
  e->src = src;
  e->dst = dst;
  return e;
}

G_edgeList G_EdgeList(G_edge e, G_edgeList l) {
  G_edgeList list = (G_edgeList)checked_malloc(sizeof(*list));
  list->head = e;
  list->tail = l;
  return list;
}

G_nodeList G_union(G_nodeList l1, G_nodeList l2)
{
    G_nodeList l=cat(l1,l2);
    G_nodeList ll=l;
    for (; l; l = l->tail) 
    {
        G_nodeList tmpl = l->tail;
        G_node tmp = l->head;
        G_nodeList parent = l;
        for (; tmpl; tmpl = tmpl->tail) 
        {
            if (tmp == tmpl->head) 
            {
              parent->tail = tmpl->tail; 
            } 
            else 
            {
              parent = tmpl;
            }
        }
    }
    return ll;
}

void G_appendTail(G_node a, G_nodeList * tl)
{
  G_nodeList append = G_NodeList(a, NULL);
  append->tail = *tl;
  *tl = append;
}

G_nodeList G_except(G_nodeList t1, G_nodeList t2)
{
  G_nodeList r = NULL, tmpl;

  while (t1){
    tmpl = t2;
    for (; tmpl; tmpl = tmpl->tail) {
      if (tmpl->head == t1->head) {
        goto l;
      }
    }
    G_appendTail(t1->head, &r);
l:      t1 = t1->tail;
  }
  return r;
}

G_nodeList G_copyList(G_nodeList tl)
{
  G_nodeList r = NULL, last;
  for (; tl; tl = tl->tail) {
    G_appendTail(tl->head, &r);
  }
  return r;
}

G_nodeList reverseList(G_nodeList head)
{
        G_nodeList newHead;
    
        if((head == NULL) || (head->tail == NULL))
            return head;
    
        newHead = reverseList(head->tail); /*递归部分*/
        head->tail->tail = head; /*回朔部分*/
        head->tail = NULL;
    
       return newHead;
}


counter Counter(int num) {
    counter c = (counter) checked_malloc(sizeof(*c));
    c->num = num;
    return c;
}
