#include "type.h"

void print_type_table() {
    FOR_URANGE(i, 0, type_table.len) {
        if (type_table.at[i].disabled) continue;
        printf("[%zu] = ", i);
        switch (type_table.at[i].tag) {
        case t_void:
            printf("void\n"); continue;
        case t_int:
            printf("int\n"); continue;
        case t_float:
            printf("float\n"); continue;
        case t_ptr:
            printf("pointer -> %zu\n", type_table.at[i].as_ptr);
            continue;
        case t_distinct:
            printf("distinct -> %zu\n", type_table.at[i].as_distinct);
            continue;
        case t_alias:
            printf("'%s' -> %zu\n", type_table.at[i].as_alias.name, type_table.at[i].as_alias.subtype);
            continue;
        case t_struct:
            printf("struct\n");
            FOR_URANGE(j, 0, type_table.at[i].as_struct.len) {
                printf("      .%s -> %d\n", 
                    type_table.at[i].as_struct.at[j].name,
                    type_table.at[i].as_struct.at[j].subtype);
            }
            continue;
        }
    }
}

bool are_simply_equivalent(type_idx a, type_idx b) {
    if (a == b) return true;
    switch (type_table.at[a].tag){
    case t_distinct: return false;
    case t_struct:
        if (type_table.at[a].as_struct.len != type_table.at[b].as_struct.len) return false;
        FOR_URANGE(i, 0, type_table.at[a].as_struct.len) {
            if (strcmp(type_table.at[a].as_struct.at[i].name, type_table.at[b].as_struct.at[i].name) != 0) return false;
            if (type_table.at[a].as_struct.at[i].subtype != type_table.at[b].as_struct.at[i].subtype) return false;
        }
        return true;
    case t_ptr:      return type_table.at[b].tag == t_ptr && are_simply_equivalent(type_table.at[a].as_ptr, type_table.at[b].as_ptr);
    default:         return type_table.at[a].tag == type_table.at[b].tag;
    }
}

da_typedef(u64);

void number_types(u64* type_numbers, type_idx a, u64* next_number) {
    if (type_numbers[a] != 0) return;

    type_numbers[a] = (*next_number)++;

    switch (type_table.at[a].tag) {
    case t_ptr:
        number_types(type_numbers, type_table.at[a].as_ptr, next_number);
        break;
    case t_struct:
        FOR_URANGE(i, 0, type_table.at[a].as_struct.len) {
            number_types(type_numbers, type_table.at[a].as_struct.at[i].subtype, next_number);
        }
    default: break;
    }

}

type_idx type_from_number(u64 t, u64* type_nums) {
    FOR_URANGE(i, 0, type_table.len) {
        if (type_nums[i] == t) return i;
    }
    return -1;
}

bool are_equivalent(type_idx a, type_idx b, bool ignore_field_names) {
    if (a >= type_table.len || b >= type_table.len)   return false;
    if (type_table.at[a].tag != type_table.at[b].tag) return false;
    if (are_simply_equivalent(a, b))                  return true;

    u64* type_numbers_A = malloc(type_table.len * sizeof(u64));
    memset(type_numbers_A, 0, type_table.len * sizeof(u64));
    u64 next_number_A = 1;
    number_types(type_numbers_A, a, &next_number_A);

    u64* type_numbers_B = malloc(type_table.len * sizeof(u64));
    memset(type_numbers_B, 0, type_table.len * sizeof(u64));
    u64 next_number_B = 1;
    number_types(type_numbers_B, b, &next_number_B);

    if (next_number_A != next_number_B) return false;

    FOR_URANGE(i, 0, next_number_A) {
        type_idx t_a = type_from_number(i, type_numbers_A);
        type_idx t_b = type_from_number(i, type_numbers_B);

        if (type_table.at[t_a].tag != type_table.at[t_b].tag) return false;

        switch (type_table.at[t_a].tag) {
        case t_ptr:
            if (type_numbers_A[type_table.at[t_a].as_ptr] != type_numbers_B[type_table.at[t_b].as_ptr] &&
                type_table.at[t_a].as_ptr != type_table.at[t_b].as_ptr)
                    return false;
            break;
        case t_struct:
            FOR_URANGE(i, 0, type_table.at[t_a].as_struct.len) {
                if (!ignore_field_names && strcmp(type_table.at[t_a].as_struct.at[i].name, type_table.at[t_b].as_struct.at[i].name) != 0)
                        return false;
                if (type_numbers_A[type_table.at[t_a].as_struct.at[i].subtype] != type_numbers_B[type_table.at[t_b].as_struct.at[i].subtype] &&
                    type_table.at[t_a].as_struct.at[i].subtype != type_table.at[t_b].as_struct.at[i].subtype)
                        return false;
            }
        }
    }

    free(type_numbers_A);
    free(type_numbers_B);

    return true;
}

// all references to type (src) are replaced with references to (dest)
void merge_types(type_idx dest, type_idx src, bool disable_src) {
    FOR_URANGE(a, t_NON_BASE, type_table.len) {
        if (type_table.at[a].disabled) continue;
        
        switch (type_table.at[a].tag) {
        case t_struct:
            FOR_URANGE(i, 0, type_table.at[a].as_struct.len) {
                if (type_table.at[a].as_struct.at[i].subtype == src) type_table.at[a].as_struct.at[i].subtype = dest;
            }
            break;
        case t_ptr:
            if (type_table.at[a].as_ptr == src) type_table.at[a].as_ptr = dest;
            break;
        case t_distinct:
            if (type_table.at[a].as_distinct == src) type_table.at[a].as_distinct = dest;
            break;
        case t_alias:
            if (type_table.at[a].as_alias.subtype == src) type_table.at[a].as_alias.subtype = dest;
            break;
        default:
            CRASH("bruh");
        }
    }
    type_table.at[src].disabled = disable_src;
}

void canonicalize_type_graph() {

    // make aliases transparent
    FOR_URANGE(i, 0, type_table.len) {
        if (type_table.at[i].tag == t_alias) {
            merge_types(type_table.at[i].as_alias.subtype, i, false);
        }
    }

    // coalesce
    restart:
    FOR_URANGE(a, 0, type_table.len) {
        if (type_table.at[a].disabled) continue;
        FOR_URANGE(b, a+1, type_table.len) {
            if (type_table.at[b].disabled) continue;
            if (are_equivalent(a, b, false)) {
                merge_types(a, b, true);
                // goto restart;
                // ^^^ if something isnt working, put this back in
            }
        }
    }
}

int main() {

    printf("start\n");

    // init type table
    da_init(&type_table, 10);
    da_append(&type_table, ((type){.tag = t_void}));
    da_append(&type_table, ((type){.tag = t_int}));
    da_append(&type_table, ((type){.tag = t_float}));


    // make a linked-list node type
    type struct_type_one = {
        .tag = t_struct,
    };
    da_init(&struct_type_one.as_struct, 1);
    da_append(&struct_type_one.as_struct, ((field){
        .name = "content",
        .subtype = t_int,
    }));
    da_append(&struct_type_one.as_struct, ((field){
        .name = "next",
        .subtype = 4,
    }));
    da_append(&type_table, struct_type_one);
    da_append(&type_table, ((type){.tag = t_ptr, .as_ptr = 10}));


    // make another one
    type struct_type_two = {
        .tag = t_struct,
    };
    da_init(&struct_type_two.as_struct, 1);
    da_append(&struct_type_two.as_struct, ((field){
        .name = "content",
        .subtype = t_int,
    }));
    da_append(&struct_type_two.as_struct, ((field){
        .name = "next",
        .subtype = 6,
    }));
    da_append(&type_table, struct_type_two);
    da_append(&type_table, ((type){.tag = t_ptr, .as_ptr = 3}));

    // make another one
    type struct_type_three = {
        .tag = t_struct,
    };
    da_init(&struct_type_two.as_struct, 1);
    da_append(&struct_type_two.as_struct, ((field){
        .name = "content",
        .subtype = t_int,
    }));
    da_append(&struct_type_two.as_struct, ((field){
        .name = "next",
        .subtype = 8,
    }));
    da_append(&type_table, struct_type_two);
    da_append(&type_table, ((type){.tag = t_ptr, .as_ptr = 7}));
    da_append(&type_table, ((type){.tag = t_ptr, .as_ptr = 10}));

    type an_alias = {
        .tag = t_alias,
        .as_alias.name = "node",
        .as_alias.subtype = 5,
    };
    da_append(&type_table, an_alias);
    

    printf("--------------------------\n");
    print_type_table();
    canonicalize_type_graph();
    printf("--------------------------\n");
    print_type_table();
    printf("--------------------------\n");


    // FOR_URANGE(i, 0, type_table.len) {
    //     FOR_URANGE(j, i, type_table.len) {
    //         if (i != j && are_equivalent(i, j, false)) printf("%zu == %zu\n", i, j);
    //     }
    // }

}

da(type) type_table;