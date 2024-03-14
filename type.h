#pragma once

#include "orbit.h"

typedef u64 type_idx;

enum {
    t_void,
    t_int,
    t_float,

    t_NON_BASE,

    t_struct,
    t_ptr,

    t_distinct,
    t_alias,
};

typedef struct field {
    char*    name;
    type_idx subtype;
} field;

da_typedef(field);

typedef struct type {
    union {
        type_idx  as_ptr;
        da(field) as_struct;
        type_idx  as_distinct;
        struct {
            char*    name;
            type_idx subtype;
        } as_alias;
    };
    u8 tag;
    bool disabled;
} type;

da_typedef(type);

extern da(type) type_table;