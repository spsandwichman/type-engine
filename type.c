#include "type.h"
#include "patterns.h"

#include <locale.h>

type_graph tg;

#define LOG(...) printf(__VA_ARGS__)
// #define LOG(...)

int main() {
    setlocale(LC_NUMERIC, "");

    make_type_graph();

    // FOR_URANGE(i, 0, 50) {
    //     K3_3();
    //     K5();
    //     ll_int_float_p2();
    // }

    // print_type_graph();
    printf("start\n");

    canonicalize();

    print_type_graph();

}

typedef struct {
    type* dest;
    type* src;
} type_pair;

da_typedef(type_pair);

void canonicalize() {
    da(type_pair) equalities;
    da_init(&equalities, 1);

    u64 num_of_types = UINT64_MAX;

    int count = 0;
    while (num_of_types != tg.len) {
        num_of_types = tg.len;
        FOR_URANGE(i, 0, tg.len) {
            FOR_URANGE(j, i+1, tg.len) {
                if (are_equivalent(tg.at[i], tg.at[j])) {
                    // printf("%d == %d\n", i, j);
                    da_append(&equalities, ((type_pair){tg.at[i], tg.at[j]}));
                }
            }
            LOG("compared %p (%'zu/%'zu)\n", tg.at[i], i, tg.len);

        }
        for (int i = equalities.len-1; i < equalities.len; --i) {
            if (equalities.at[i].src->disabled) continue;
            merge_type_references(equalities.at[i].dest, equalities.at[i].src);
            LOG("merged %p <- %p (%'zu/%'zu)\n", equalities.at[i].dest, equalities.at[i].src, equalities.len-1-i, equalities.len);
        }
        // LOG("equalities merged\n");
        da_clear(&equalities);


        FOR_URANGE(i, 0, tg.len) {
            if (tg.at[i]->disabled) {
                da_unordered_remove_at(&tg, i);
                i--;
            }
        }
    }

    da_destroy(&equalities);
}

void merge_type_references(type* dest, type* src) {

    if (src->disabled) return;

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
    if (a->tag < T_meta_INTEGRAL) return true;

    if (a->tag == T_POINTER || a->tag == T_SLICE) {
        if (get_target(a) == get_target(b)) return true;
    }

    if (a->tag == T_STRUCT) {
        if (a->as_struct.fields.len != b->as_struct.fields.len) return false;
        bool subtype_equals = true;
        FOR_URANGE(i, 0, a->as_struct.fields.len) {
            if (get_field(a, i)->subtype != get_field(b, i)->subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
    }



    u64 a_numbers = 1;
    locally_number(a, &a_numbers, 0);

    u64 b_numbers = 1;
    locally_number(b, &b_numbers, 1);

    if (a_numbers != b_numbers) {
        reset_numbers();
        return false;
    }

    FOR_URANGE(i, 1, a_numbers) {
        if (!is_element_equivalent(get_type_from_num(i, 0), get_type_from_num(i, 1), 0, 1)) {
            reset_numbers();
            return false;
        }
    }

    // print_type_graph();

    reset_numbers();
    return true;
}

bool is_element_equivalent(type* a, type* b, int num_set_a, int num_set_b) {
    if (a->tag != b->tag) return false;

    switch (a->tag) {
    case T_VOID:
    case T_I8:  case T_I16: case T_I32: case T_I64:
    case T_U8:  case T_U16: case T_U32: case T_U64:
    case T_F16: case T_F32: case T_F64:
        if (a->tag != b->tag) return false;
        if (a->type_nums[num_set_a] == b->type_nums[num_set_b]) return true;
        // printf("---- %zu %zu\n", a->type_nums[num_set_a], b->type_nums[num_set_b]);
        break;
    case T_STRUCT:
        
        if (a->as_struct.fields.len != b->as_struct.fields.len) return false;

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

void reset_numbers() {
    FOR_URANGE(i, 0, tg.len) {
        tg.at[i]->type_nums[0] = 0;
        tg.at[i]->type_nums[1] = 0;
    }
}

type* get_type_from_num(u16 num, int num_set) {
    // printf("FUCK %hu %d\n", num, num_set);
    FOR_URANGE(i, 0, tg.len) {
        if (tg.at[i]->type_nums[num_set] == num) return tg.at[i];
    }
    return NULL;
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
    make_type(T_I8);
    make_type(T_I16);
    make_type(T_I32);
    make_type(T_I64);
    make_type(T_U8);
    make_type(T_U16);
    make_type(T_U32);
    make_type(T_U64);
    make_type(T_F16);
    make_type(T_F32);
    make_type(T_F64);
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
    if (p->tag != T_POINTER || p->tag != T_SLICE) return;
    p->as_reference.subtype = dest;
}

type* get_target(type* p) {
    if (p->tag != T_POINTER) return NULL;
    return p->as_reference.subtype;
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
        case T_VOID:  printf("(void)\n");  break;
        case T_I64:   printf("i64\n");   break;
        case T_F64:   printf("f64\n"); break;
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
            printf("unknown tag %d\n", t->tag);
            break;
        }
    }
}