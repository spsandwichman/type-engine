#include "type.h"

type_graph tg;

#define COALESCE_LIMIT 2

int main() {
    make_type_graph();
    K5();

    // printf("%s\n", are_equivalent(tg.at[3], tg.at[5]) ? "true" : "false");

    print_type_graph();

    FOR_URANGE(i, 0, COALESCE_LIMIT) coalesce();

    print_type_graph();

}

typedef struct {
    type* dest;
    type* src;
} type_pair;

da_typedef(type_pair);

void coalesce() {
    da(type_pair) equalities;
    da_init(&equalities, 1);

    FOR_URANGE(i, 0, tg.len) {
        FOR_URANGE(j, i+1, tg.len) {
            if (are_equivalent(tg.at[i], tg.at[j])) {
                da_append(&equalities, ((type_pair){tg.at[i], tg.at[j]}));
            }
        }
    }

    for (i64 i = equalities.len-1; i >= 0; i--) {
        // printf("%zx\n", i);
        merge_type_references(equalities.at[i].dest, equalities.at[i].src);
    }

    FOR_URANGE(i, 0, tg.len) {
        if (tg.at[i]->disabled) {
            da_unordered_remove_at(&tg, i);
            i--;
        }
    }
}

void merge_type_references(type* dest, type* src) {

    u64 src_index = get_index(src);
    if (src_index == UINT64_MAX) {
        return;
    }

    FOR_URANGE(i, 0, tg.len) {
        type* t = tg.at[i];
        switch (t->tag) {
        case T_STRUCT:
            FOR_URANGE(i, 0, t->as_struct.fields.len) {
                if (get_field(t, i)->subtype == src) {
                    get_field(t, i)->subtype = dest;
                }
            }
            break;
        case T_POINTER:
            if (get_target(t) == src) {
                set_target(t, dest);
            }
            break;
        default:
            break;
        }
    }

    // printf("--- %zu\n", src_index);
    src->disabled = true;
}

bool are_equivalent(type* a, type* b) {

    if (a->tag != b->tag) return false;

    if (a->tag == T_POINTER) {
        if (get_target(a) == get_target(b)) return true;
    }



    u64 a_numbers = 1;
    locally_number(a, &a_numbers, 0);

    u64 b_numbers = 1;
    locally_number(b, &b_numbers, 1);

    if (a_numbers != b_numbers) return false;


    FOR_URANGE(i, 1, a_numbers) {
        if (!is_element_equivalent(
            get_type_from_num(i, 0), 
            get_type_from_num(i, 1),
            0, 
            1
        )) {
            return false;
        }
    }

    FOR_URANGE(i, 0, tg.len) {
        tg.at[i]->type_nums[0] = 0;
        tg.at[i]->type_nums[1] = 0;
    }

    return true;
}

bool is_element_equivalent(type* a, type* b, int num_set_a, int num_set_b) {
    if (a->tag != b->tag) return false;
    switch (a->tag) {
    case T_STRUCT:
        FOR_URANGE(i, 0, a->as_struct.fields.len) {
            if (strcmp(get_field(a, i)->name, get_field(b, i)->name) != 0) {
                return false;
            }
            if (get_field(a, i)->subtype->type_nums[num_set_a] != get_field(b, i)->subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        break;
    case T_POINTER:
        if (get_target(a)->type_nums[num_set_a] != get_target(b)->type_nums[num_set_b]) {
            return false;
        }
        break;
    default:
        break;
    }

    return true;
}

void locally_number(type* t, u64* number, int num_set) {
    if (t->type_nums[num_set] != 0) return;

    t->type_nums[num_set] = (*number)++;

    switch (t->tag) {
    case T_STRUCT:
        FOR_URANGE(i, 0, t->as_struct.fields.len) {
            locally_number(get_field(t, i)->subtype, number, num_set);
        }
        break;
    case T_POINTER:
            locally_number(get_target(t), number, num_set);
        break;
    default:
        break;
    }
}

type* get_type_from_num(u16 num, int num_set) {
    FOR_URANGE(i, 0, tg.len) {
        if (tg.at[i]->type_nums[num_set] == num) return tg.at[i];
    }
    printf("FUCK %hu %d", num, num_set);
    return NULL;
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

    printf("K5 constructed\n");
}

type* make_type(u8 tag) {
    type* t = malloc(sizeof(type));
    *t = (type){0};
    t->tag = tag;
    if (t->tag == T_STRUCT) {
        da_init(&t->as_struct.fields, 1);
    }
    da_append(&tg, t);
    return t;
}

void make_type_graph() {
    tg = (type_graph){0};
    da_init(&tg, 3);
    make_type(T_VOID);
    make_type(T_INT);
    make_type(T_FLOAT);
}

struct_field* get_field(type* s, size_t i) {
    if (s->tag != T_STRUCT) return NULL;
    return &s->as_struct.fields.at[i];
}

void add_field(type* s, char* name, type* sub) {
    if (s->tag != T_STRUCT) return;
    da_append(&s->as_struct.fields, ((struct_field){name, sub}));
}

void set_target(type* p, type* dest) {
    if (p->tag != T_POINTER) return;
    p->as_pointer.subtype = dest;
}

type* get_target(type* p) {
    if (p->tag != T_POINTER) return NULL;
    return p->as_pointer.subtype;
}

u64 get_index(type* t) {
    FOR_URANGE(i, 0, tg.len) {
        if (tg.at[i] == t) return i;
    }
    return UINT64_MAX;
}

void print_type_graph() {
    printf("-------------------------\n");
    FOR_URANGE(i, 0, tg.len) {
        type* t = tg.at[i];
        printf("%-2zu   [%-2hu, %-2hu]\t", i, t->type_nums[0], t->type_nums[1]);
        switch (t->tag){
        case T_VOID:  printf("VOID\n");  break;
        case T_INT:   printf("INT\n");   break;
        case T_FLOAT: printf("FLOAT\n"); break;
        case T_POINTER:
            printf("^ %zu\n", get_index(get_target(t)));
            break;
        case T_STRUCT:
            printf("struct\n");
            FOR_URANGE(field, 0, t->as_struct.fields.len) {
                printf("\t\t.%s : %zu\n", get_field(t, field)->name, get_index(get_field(t, field)->subtype));
            }
            break;
        
        default:
            break;
        }
    }
}