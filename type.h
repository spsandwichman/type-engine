#pragma once

#include "orbit.h"

typedef u64 type_idx;

enum {
    t_void,
    t_int,
    t_float,

    t_struct,
    t_ptr,
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
    };
    u8 tag;
} type;

da_typedef(type);

extern da(type) type_table;