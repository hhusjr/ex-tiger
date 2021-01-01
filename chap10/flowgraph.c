#include "flowgraph.h"
#include "assem.h"
#include "table.h"

G_graph FG_AssemFlowGraph(AS_instrList il) {
    TAB_table label_map = TAB_empty(); // Map Temp_label to AS_instr
    TAB_table node_map = TAB_empty(); // Map AS_instr to G_Node
    AS_instrList il_nolab = NULL, il_nolab_tail = NULL;
    G_graph g = G_Graph();

    // First pass, memorize each label's position, and build another AS_instrList without label instr
    for (; il; il = il->tail) {
        if (il->head->kind == I_LABEL) {
            // Find the "real" position of the label
            AS_instrList p = il;
            for (; p && p->head->kind == I_LABEL; p = p->tail); // skip labels
            TAB_enter(label_map, il->head->u.LABEL.label, p ? p->head : NULL);
            continue;
        }
        if (!il_nolab) {
            il_nolab = il_nolab_tail = AS_InstrList(il->head, NULL);
        } else {
            il_nolab_tail->tail = AS_InstrList(il->head, NULL);
            il_nolab_tail = il_nolab_tail->tail;
        }
    }

    // Second pass, generate graph
    il = il_nolab;
    for (; il; il = il->tail) {
        // Calculate targets
        AS_instrList targets = NULL;
        if (il->head->kind == I_MOVE
        ||  (il->head->kind == I_OPER && il->head->u.OPER.jumps == NULL)) {
            targets = il->tail ? AS_InstrList(il->tail->head, NULL) : NULL;
        } else if (il->head->kind == I_OPER && il->head->u.OPER.jumps != NULL) {
            for (Temp_labelList jumps = il->head->u.OPER.jumps->labels; jumps; jumps = jumps->tail) {
                AS_instr ins = TAB_look(label_map, jumps->head);
                if (ins) {
                    targets = AS_InstrList(ins, targets);
                }
            }
        } else {
            assert(0);
        }

        // Create edge between il to each node of targets
        G_node src = TAB_look(node_map, il->head);
        if (!src) {
            src = G_Node(g, il->head);
            TAB_enter(node_map, il->head, src);
        }
        for (; targets; targets = targets->tail) {
            G_node dst = TAB_look(node_map, targets->head);
            if (!dst) {
                dst = G_Node(g, targets->head);
                TAB_enter(node_map, targets->head, dst);
            }

            G_addEdge(src, dst);
        }
    }

    return g;
}

Temp_tempList FG_def(G_node n) {
    AS_instr ins = G_nodeInfo(n);
    if (ins->kind == I_MOVE) {
        return ins->u.MOVE.dst;
    } else if (ins->kind == I_OPER) {
        return ins->u.OPER.dst;
    }
    assert(0);
}

Temp_tempList FG_use(G_node n) {
    AS_instr ins = G_nodeInfo(n);
    if (ins->kind == I_MOVE) {
        return ins->u.MOVE.src;
    } else if (ins->kind == I_OPER) {
        return ins->u.OPER.src;
    }
    assert(0);
}

bool FG_isMove(G_node n) {
    AS_instr ins = G_nodeInfo(n);
    return ins->kind == I_MOVE;
}