#include "type.h"
#include "patterns.h"

#include <locale.h>

type_graph tg;

#define LOG(...) printf(__VA_ARGS__)
// #define LOG(...)

int main() {
    setlocale(LC_NUMERIC, "");

    make_type_graph();

    zipper_ll(10);

    

    print_type_graph();
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

    // preliminary normalization
    FOR_URANGE(i, 0, tg.len) {
        type* t = tg.at[i];
        switch (t->tag) {
        case T_ALIAS: // alias retargeting
            while (t->tag == T_ALIAS) {
                merge_type_references(get_target(t), t, false);
                t = get_target(t);
            }
            break;
        case T_ENUM: // variant sorting
            // using insertion sort for really nice best-case complexity
            FOR_URANGE(i, 1, t->as_enum.variants.len) {
                u64 j = i;
                while (j > 0 && variant_less(get_variant(t, j), get_variant(t, j-1))) {
                    enum_variant temp = *get_variant(t, j);
                    t->as_enum.variants.at[j] = t->as_enum.variants.at[j-1];
                    t->as_enum.variants.at[j-1] = temp;
                    j--;
                }
            }
        default: break;
        }
    }

    da(type_pair) equalities;
    da_init(&equalities, 1);

    u64 num_of_types = 0;
    while (num_of_types != tg.len) {
        num_of_types = tg.len;
        FOR_URANGE(i, 0, tg.len) {
            if (tg.at[i]->tag == T_ALIAS) continue;
            FOR_URANGE(j, i+1, tg.len) {
                if (tg.at[j]->tag == T_ALIAS) continue;
                if (are_equivalent(tg.at[i], tg.at[j])) {
                    da_append(&equalities, ((type_pair){tg.at[i], tg.at[j]}));
                }
            }
            LOG("compared %p (%'zu/%'zu)\n", tg.at[i], i, tg.len);
        }
        for (int i = equalities.len-1; i < equalities.len; --i) {
            if (equalities.at[i].src->disabled) continue;
            merge_type_references(equalities.at[i].dest, equalities.at[i].src, true);
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

bool are_equivalent(type* a, type* b) {

    while (a->tag == T_ALIAS) a = get_target(a);
    while (b->tag == T_ALIAS) b = get_target(b);

    // simple checks
    if (a == b) return true;
    if (a->tag != b->tag) return false;
    if (a->tag < T_meta_INTEGRAL) return true;
    
    // a little more complex
    switch (a->tag) {
    case T_POINTER:
    case T_SLICE:
        if (get_target(a) == get_target(b)) return true;
        break;
    case T_STRUCT:
        if (a->as_struct.fields.len != b->as_struct.fields.len) return false;
        bool subtype_equals = true;
        FOR_URANGE(i, 0, a->as_struct.fields.len) {
            if (get_field(a, i)->subtype != get_field(b, i)->subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case T_ARRAY:
        if (a->as_array.len != b->as_array.len) return false;
        break;
    case T_DISTINCT: // distinct types are VERY strict
        return a == b;

    case T_ADDR:
        return a == b; // this should really not do anything

    default: break;
    }

    // deep structure numbering

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
    case T_NONE:
    case T_I8:  case T_I16: case T_I32: case T_I64:
    case T_U8:  case T_U16: case T_U32: case T_U64:
    case T_F16: case T_F32: case T_F64: case T_ADDR:
        return true;
        // if (a->type_nums[num_set_a] == b->type_nums[num_set_b]) return true;
        break;

    case T_ALIAS:
    case T_DISTINCT:
        return a == b;

    case T_ENUM:
        if (a->as_enum.variants.len != b->as_enum.variants.len) {
            return false;
        }
        if (a->as_enum.backing_type != b->as_enum.backing_type) {
            return false;
        }

        FOR_URANGE(i, 0, a->as_enum.variants.len) {
            if (get_variant(a, i)->enum_val != get_variant(b, i)->enum_val) {
                return false;
            }
            if (strcmp(get_variant(a, i)->name, get_variant(b, i)->name) != 0) {
                return false;
            }
        }
        break;
    case T_STRUCT:
        if (a->as_struct.fields.len != b->as_struct.fields.len) {
            return false;
        }
        FOR_URANGE(i, 0, a->as_struct.fields.len) {
            if (strcmp(get_field(a, i)->name, get_field(b, i)->name) != 0) {
                return false;
            }
            if (get_field(a, i)->subtype->type_nums[num_set_a] != get_field(b, i)->subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        break;
    case T_ARRAY:
        if (a->as_array.len != b->as_array.len) {
            return false;
        }
        if (a->as_array.subtype->type_nums[num_set_a] != b->as_array.subtype->type_nums[num_set_b]) {
            return false;
        }
        break;
    case T_POINTER:
    case T_SLICE:
        if (get_target(a)->type_nums[num_set_a] != get_target(b)->type_nums[num_set_b]) {
            return false;
        }
        break;
    default:
        printf("encountered %d", a->tag);
        CRASH("unknown type kind encountered");
        break;
    }

    return true;
}

void merge_type_references(type* dest, type* src, bool disable) {

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
        case T_SLICE:
        case T_ALIAS:
        case T_DISTINCT:
            if (get_target(t) == src) {
                set_target(t, dest);
            }
            break;
        case T_ARRAY:
            if (t->as_array.subtype == src) {
                t->as_array.subtype = dest;
            }
            break;
        default:
            break;
        }
    }

    // printf("--- %zu\n", src_index);
    if (disable) src->disabled = true;
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
    case T_ARRAY:
        locally_number(t->as_array.subtype, number, num_set);
    case T_POINTER:
    case T_SLICE:
    case T_DISTINCT:
    case T_ALIAS:
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
    if (tag < T_meta_INTEGRAL && (tg.len >= T_meta_INTEGRAL)) {
        return tg.at[tag];
    }

    type* t = malloc(sizeof(type));
    *t = (type){0};
    t->tag = tag;

    switch (tag) {
    case T_STRUCT:
        da_init(&t->as_struct.fields, 1);
        break;
    case T_ENUM:
        da_init(&t->as_enum.variants, 1);
        t->as_enum.backing_type = T_I64;
        break;
    default: break;
    }
    da_append(&tg, t);
    return t;
}

void make_type_graph() {
    tg = (type_graph){0};
    da_init(&tg, 3);

    make_type(T_NONE);
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
    make_type(T_ADDR);
}

void add_field(type* s, char* name, type* sub) {
    if (s->tag != T_STRUCT) return;
    da_append(&s->as_struct.fields, ((struct_field){name, sub}));
}

struct_field* get_field(type* s, size_t i) {
    if (s->tag != T_STRUCT) return NULL;
    return &s->as_struct.fields.at[i];
}

void add_variant(type* e, char* name, i64 val) {
    if (e->tag != T_ENUM) return;
    da_append(&e->as_enum.variants, ((enum_variant){name, val}));
}

enum_variant* get_variant(type* e, size_t i) {
    if (e->tag != T_ENUM) return NULL;
    return &e->as_enum.variants.at[i];
}

bool variant_less(enum_variant* a, enum_variant* b) {
    if (a->enum_val == b->enum_val) {
        // use string names
        return strcmp(a->name, b->name) < 0;
    } else {
        return a->enum_val < b->enum_val;
    }
}

void set_target(type* p, type* dest) {
    if (p->tag != T_POINTER && 
        p->tag != T_SLICE && 
        p->tag != T_ALIAS &&
        p->tag != T_DISTINCT) return;
    p->as_reference.subtype = dest;
}

type* get_target(type* p) {
    if (p->tag != T_POINTER && 
        p->tag != T_SLICE && 
        p->tag != T_ALIAS &&
        p->tag != T_DISTINCT) return NULL;
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
        // printf("%-2zu   [%-2hu, %-2hu]\t", i, t->type_nums[0], t->type_nums[1]);
        printf("%-2zu\t", i);
        switch (t->tag){
        case T_NONE:    printf("(none)\n"); break;
        case T_I8:      printf("i8\n");     break;
        case T_I16:     printf("i16\n");    break;
        case T_I32:     printf("i32\n");    break;
        case T_I64:     printf("i64\n");    break;
        case T_U8:      printf("u8\n");     break;
        case T_U16:     printf("u16\n");    break;
        case T_U32:     printf("u32\n");    break;
        case T_U64:     printf("u64\n");    break;
        case T_F16:     printf("f16\n");    break;
        case T_F32:     printf("f32\n");    break;
        case T_F64:     printf("f64\n");    break;
        case T_ADDR:    printf("addr\n");   break;
        case T_ALIAS:
            printf("alias %zu\n", get_index(get_target(t)));
            break;
        case T_DISTINCT:
            printf("distinct %zu\n", get_index(get_target(t)));
            break;
        case T_POINTER:
            printf("pointer %zu\n", get_index(get_target(t)));
            break;
        case T_SLICE:
            printf("slice %zu\n", get_index(get_target(t)));
            break;
        case T_ARRAY:
            printf("array %zu\n", get_index(t->as_array.subtype));
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
