#include "liveness.h"
#include "stdint.h"
#include "flowgraph.h"
#include "frame.h"

LV_moveList LV_MoveList(G_node src, G_node dst, LV_moveList tail) {
    LV_moveList p = checked_malloc(sizeof(*p));
    p->src = src;
    p->dst = dst;
    p->tail = tail;
    return p;
}

Temp_temp LV_gtemp(G_node n) {
    return G_nodeInfo(n);
}

// The LV_set is an ordered set, aka ORDER BY temp.num ASC
typedef Temp_tempList LV_set;

static LV_set LV_Set(Temp_temp head, LV_set tail) {
    return Temp_TempList(head, tail);
}

static LV_set set_union(LV_set x, LV_set y) {
    LV_set res = NULL, tail = NULL;

    while (x && y) {
        Temp_temp ins;
        if (x->head->num < y->head->num) {
            ins = x->head;
            x = x->tail;
        } else {
            ins = y->head;
            y = y->tail;
        }

        if (tail && ins == tail->head) {
            continue;
        }

        if (!res) {
            res = tail = LV_Set(ins, NULL);
        } else {
            assert(tail);
            tail->tail = LV_Set(ins, NULL);
            tail = tail->tail;
        }
    }

    // Make x longer than y
    if (y) {
        x = y;
    }

    for (; x; x = x->tail) {
        if (tail && x->head == tail->head) {
            continue;
        }

        if (!res) {
            res = tail = LV_Set(x->head, NULL);
        } else {
            assert(tail);
            tail->tail = LV_Set(x->head, NULL);
            tail = tail->tail;
        }
    }

    return res;
}

static LV_set set_inplace_union(LV_set dst, LV_set src) {
    // If dst is an empty set, we make a copy of src
    if (!dst) {
        LV_set tail;
        dst = tail = NULL;
        for (; src; src = src->tail) {
            if (!dst) {
                dst = tail = Temp_TempList(src->head, NULL);
            } else {
                tail->tail = Temp_TempList(src->head, NULL);
                tail = tail->tail;
            }
        }
        return dst;
    }

    LV_set cur = dst;

    // First, we insert each {x | x in src and x <= min{dst}}, if equal, do not insert
    LV_set head = NULL, tail = NULL;
    for (; src; src = src->tail) {
        if (src->head->num > dst->head->num) {
            break;
        }
        if (src->head->num == dst->head->num) {
            continue;
        }
        if (!head) {
            head = tail = Temp_TempList(src->head, NULL);
        } else {
            tail->tail = Temp_TempList(src->head, NULL);
            tail = tail->tail;
        }
    }
    if (tail != NULL) {
        tail->tail = dst;
        dst = head;
    }

    // Then, we can assert min{src} > min{dst}
    for (; src; src = src->tail) {
        // Find cur so that, cur <= src < the next element of cur
        for (; cur->tail && cur->tail->head->num <= src->head->num; cur = cur->tail);

        // If cur == src, skip
        if (cur->head->num == src->head->num) {
            continue;
        }

        // otherwise, insert src after cur
        cur->tail = Temp_TempList(src->head, cur->tail);
    }

    return dst;
}

static LV_set set_diff(LV_set lhs, LV_set rhs) {
    LV_set res = NULL, res_tail = NULL;

    // Loop through each element in lhs
    for (; lhs; lhs = lhs->tail) {
        // Find the first element in rhs bigger than or equal to current element in lhs
        for (; rhs && rhs->head->num < lhs->head->num; rhs = rhs->tail);

        // If equal, do not insert
        if (rhs && rhs->head->num == lhs->head->num) {
            continue;
        }

        // insert to res
        if (!res) {
            res = res_tail = LV_Set(lhs->head, NULL);
        } else {
            res_tail->tail = LV_Set(lhs->head, NULL);
            res_tail = res_tail->tail;
        }
    }

    return res;
}

static bool set_is_equal(LV_set lhs, LV_set rhs) {
    while (lhs && rhs) {
        // ensure value equal
        if (lhs->head != rhs->head) {
            return FALSE;
        }

        lhs = lhs->tail;
        rhs = rhs->tail;
    }

    // ensure length equal
    return !lhs && !rhs;
}

LV_graph LV_liveness(G_graph flow) {
    // Calculate IN and OUT set first
    // Data flow equation:
    // IN(n)  = use(n) UNION (OUT(n) - def(n))
    // OUT(n) = the UNION of IN(x), x in succ[n]

    G_table use_set = G_empty(), def_set = G_empty();
    G_table in = G_empty();
    G_table out = G_empty();

    for (G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
        G_node node = nodes->head;
        LV_set use = NULL, def = NULL;
        for (Temp_tempList uses = FG_use(node); uses; uses = uses->tail) {
            use = set_inplace_union(use, LV_Set(uses->head, NULL));
        }
        G_enter(use_set, node, use);
        for (Temp_tempList defs = FG_def(node); defs; defs = defs->tail) {
            def = set_inplace_union(def, LV_Set(defs->head, NULL));
        }
        G_enter(def_set, node, def);
    }

    bool fix_point;
    do {
        fix_point = TRUE;
        for (G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
            G_node node = nodes->head;

            LV_set prev_in = set_union(G_look(in, node), NULL);
            LV_set prev_out = set_union(G_look(out, node), NULL);

            LV_set new_in = set_union(G_look(use_set, node), set_diff(G_look(out, node), G_look(def_set, node)));
            LV_set new_out = NULL;
            for (G_nodeList succ = G_succ(node); succ; succ = succ->tail) {
                new_out = set_inplace_union(new_out, G_look(in, succ->head));
            }

            G_enter(in, node, new_in);
            G_enter(out, node, new_out);

            if (!set_is_equal(new_in, prev_in) || !set_is_equal(new_out, prev_out)) {
                fix_point = FALSE;
            }
        }
    } while (!fix_point);

    // For debugging
    printf("Dataflow Analysis: IN and OUT set\n");
    for (G_nodeList nodes = G_nodes(flow); nodes; nodes = nodes->tail) {
        AS_print(stdout, FG_instr(nodes->head), Temp_layerMap(F_TempMap(), Temp_name()));
        printf("IN: ");
        LV_set in_set = G_look(in, nodes->head);
        for (Temp_tempList l = in_set; l; l = l->tail) {
            printf("%s ", Temp_look(Temp_layerMap(F_TempMap(), Temp_name()), l->head));
        }
        printf("\nOUT: ");
        LV_set out_set = G_look(out, nodes->head);
        for (Temp_tempList l = out_set; l; l = l->tail) {
            printf("%s ", Temp_look(Temp_layerMap(F_TempMap(), Temp_name()), l->head));
        }
        printf("\n\n");
    }
}