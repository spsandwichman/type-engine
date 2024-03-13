#include "type.h"

void print_type_table() {
    FOR_URANGE(i, 0, type_table.len) {
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
    case t_struct:  return false;
    case t_ptr:     return type_table.at[b].tag == t_ptr && are_simply_equivalent(type_table.at[a].as_ptr, type_table.at[b].as_ptr);
    default:        return type_table.at[a].tag == type_table.at[b].tag;
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
    if (a >= type_table.len || b >= type_table.len) return false;
    if (are_simply_equivalent(a, b)) return true;
    if (type_table.at[a].tag != type_table.at[b].tag) return false;


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
            if (type_numbers_A[type_table.at[t_a].as_ptr] != type_numbers_B[type_table.at[t_b].as_ptr]) 
                return false;
            break;
        case t_struct:
            FOR_URANGE(i, 0, type_table.at[t_a].as_struct.len) {
                if (!ignore_field_names && strcmp(type_table.at[t_a].as_struct.at[i].name, type_table.at[t_b].as_struct.at[i].name) != 0)
                    return false;
                if (type_numbers_A[type_table.at[t_a].as_struct.at[i].subtype] != type_numbers_B[type_table.at[t_b].as_struct.at[i].subtype])
                    return false;
            }
        }
    }

    free(type_numbers_A);
    free(type_numbers_B);

    return true;
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
    da_append(&type_table, ((type){.tag = t_ptr, .as_ptr = 5}));


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
    
    print_type_table();

    FOR_URANGE(i, 0, type_table.len) {
        FOR_URANGE(j, i, type_table.len) {
            if (i != j && are_equivalent(i, j, false)) printf("%zu == %zu\n", i, j);
        }
    }

}

da(type) type_table;