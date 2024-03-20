#include "type.h"

void ll_int_float_p2() {
    type* struct_nodes[4] = {0};
    FOR_URANGE(i, 0, 4) {
        struct_nodes[i] = make_type(T_STRUCT);
        add_field(struct_nodes[i], "content", i % 2 == 0 ? tg.at[T_I64] : tg.at[T_F64]);
    }

    FOR_URANGE(i, 0, 4) {
        type* next = make_type(T_POINTER);
        set_target(next, struct_nodes[(i+1)%4]);
        add_field(struct_nodes[i], "next", next);
    }
}

void complete(u64 magnitude) {
    type** struct_nodes = malloc(sizeof(type*) * magnitude);
    memset(struct_nodes, 0, sizeof(type*) * magnitude);

    FOR_URANGE(i, 0, magnitude) {
        struct_nodes[i] = make_type(T_STRUCT);
    }

    FOR_URANGE(i, 0, magnitude) {
        FOR_URANGE(c, i+1, i+1+magnitude) {
            u64 conn = c % magnitude;
            if (i == conn) continue;

            type* p = make_type(T_POINTER);
            set_target(p, struct_nodes[conn]);
            
            char* field_str = malloc(50);
            memset(field_str, 0, 50);
            sprintf(field_str, "conn_%zu", c-i);
            add_field(struct_nodes[i], field_str, p);
        }
    }

}

void K5() {
    type* struct_nodes[5] = {0};
    FOR_URANGE(i, 0, 5) {
        struct_nodes[i] = make_type(T_STRUCT);
    }

    FOR_URANGE(i, 0, 5) {
        FOR_URANGE(c, i+1, i+6) {
            u64 conn = c % 5;
            if (i == conn) continue;

            type* p = make_type(T_POINTER);
            set_target(p, struct_nodes[conn]);

            // printf("connecting %d to %d (%p to %p)\n", i, conn, struct_nodes[i], p);

            char* field_str = malloc(20);
            memset(field_str, 0, 20);
            sprintf(field_str, "conn_%d", c-i);
            add_field(struct_nodes[i], field_str, p);
        }
    }
}

void K5_slice() {
    type* struct_nodes[5] = {0};
    FOR_URANGE(i, 0, 5) {
        struct_nodes[i] = make_type(T_STRUCT);
    }

    FOR_URANGE(i, 0, 5) {
        FOR_URANGE(c, i+1, i+6) {
            u64 conn = c % 5;
            if (i == conn) continue;

            type* p = make_type(T_SLICE);
            set_target(p, struct_nodes[conn]);

            // printf("connecting %d to %d (%p to %p)\n", i, conn, struct_nodes[i], p);

            char* field_str = malloc(20);
            memset(field_str, 0, 20);
            sprintf(field_str, "conn_%d", c-i);
            add_field(struct_nodes[i], field_str, p);
        }
    }
}

void K3_3() {
    type* top_set[3] = {0};
    type* btm_set[3] = {0};
    
    FOR_URANGE(i, 0, 3) {
        top_set[i] = make_type(T_STRUCT);
    }
    FOR_URANGE(i, 0, 3) {
        btm_set[i] = make_type(T_STRUCT);
    }

    FOR_URANGE(i, 0, 3) {
        FOR_URANGE(j, 0, 3) {
            type* p = make_type(T_POINTER);
            set_target(p, btm_set[j]);

            char* field_str = malloc(20);
            memset(field_str, 0, 20);
            sprintf(field_str, "conn_%d", j);
            add_field(top_set[i], field_str, p);
        }
    }

    FOR_URANGE(i, 0, 3) {
        FOR_URANGE(j, 0, 3) {
            type* p = make_type(T_POINTER);
            set_target(p, top_set[j]);

            char* field_str = malloc(20);
            memset(field_str, 0, 20);
            sprintf(field_str, "conn_%d", j);
            add_field(btm_set[i], field_str, p);
        }
    }
}

void zipper(u64 magnitude) {
    
    type* p = make_type(T_I64);

    FOR_URANGE(i, 0, magnitude) {
        type* s = make_type(T_STRUCT);
        add_field(s, "inner", p);
        p = s;
    }

    p = make_type(T_I64);

    FOR_URANGE(i, 0, magnitude) {
        type* s = make_type(T_STRUCT);
        add_field(s, "inner", p);
        p = s;
    }
}

void zipper_ll(u64 magnitude) {
    
    type* real_h = make_type(T_STRUCT);
    type* h = real_h;


    FOR_URANGE(i, 0, magnitude) {
        type* p = make_type(T_POINTER);
        type* s = make_type(T_STRUCT);
        set_target(p, h);
        add_field(s, "inner", p);
        add_field(s, "content", make_type(T_I64));
        h = s;
    }

    h = real_h;

    FOR_URANGE(i, 0, magnitude) {
        type* s = make_type(T_STRUCT);
        type* p = make_type(T_POINTER);
        set_target(p, h);
        add_field(s, "inner", p);
        add_field(s, "content", make_type(T_I64));
        h = s;
    }
    
    type* p = make_type(T_POINTER);
    set_target(p, h);
    add_field(real_h, "inner", p);
    add_field(real_h, "content", make_type(T_I64));
}

void linked_list(u64 magnitude, bool const_ptrs) {

    type* head;
    type* last;

    FOR_URANGE(i, 0, magnitude) {
        type* node = make_type(T_STRUCT);
        if (i == 0) {
            head = node;
            last = node;
        }
        type* ptr = make_type(T_POINTER);
        ptr->as_reference.constant = const_ptrs;
        set_target(ptr, last);
        add_field(node, "next", ptr);
        add_field(node, "content", make_type(T_I16));
        last = node;
    }

    get_field(head, 0)->subtype->as_reference.subtype = last;
}