#pragma once

#include "orbit.h"

enum {
    // does not really do anything. serves as an invalid/none type.
    T_NONE,

    // signed integer types
    T_I8,
    T_I16,
    T_I32,
    T_I64,
    // unsigned integer types
    T_U8,
    T_U16,
    T_U32,
    T_U64,
    // floating point types
    T_F16,
    T_F32,
    T_F64,
    // bare address type
    T_ADDR,

    // for checking purposes, is not an actual type lol
    T_meta_INTEGRAL,

    // reference to another type
    T_POINTER,

    // reference to an array of another type + len
    T_SLICE,

    // array of a type
    T_ARRAY,

    // enumeration over an integral type
    T_ENUM,

    // all-purpose aggregate type.
    T_STRUCT,

    // function type! has sort of the same semantics as a struct
    T_FUNCTION,

    // an alias is an "entrypoint" for the type graph
    // an outside system can point to an alias and
    // the canonicalizer will make sure the alias never becomes invalid
    // references to an alias type get canonicalized into 
    // references to the underlying type
    T_ALIAS,
    // like an alias, except checks against other types always fail.
    // references do not get canonicalized away like aliases
    T_DISTINCT,

};

typedef struct struct_field {
    char* name;
    struct type* subtype;
} struct_field;

da_typedef(struct_field);

typedef struct enum_variant {
    char* name;
    i64 enum_val;
} enum_variant;

da_typedef(enum_variant);

typedef struct type {
    union {
        struct {
            struct type* subtype;
        } as_reference; // used by T_POINTER, T_SLICE, T_ALIAS, and T_DISTINCT
        struct {
            struct type* subtype;
            u64 len;
        } as_array;
        struct {
            da(struct_field) fields;
        } as_struct;
        struct {
            da(struct_field) params;
            da(struct_field) returns;
        } as_function;
        struct {
            da(enum_variant) variants;
            u8 backing_type;
        } as_enum;
    };
    u8 tag;
    // bool disabled;
    struct type* moved;
    bool dirty;

    u16 size;

    // data for comparison shit
    u16 type_nums[2];
} type;

typedef struct {
    type** at;
    size_t len;
    size_t cap;
} type_graph;

extern type_graph tg;

void K5();
void ll_int_float_p2();

void  make_type_graph();
type* restrict make_type(u8 tag);

void          add_field(type* restrict s, char* name, type* restrict sub);
struct_field* get_field(type* restrict s, size_t i);

void          add_variant(type* restrict e, char* name, i64 val);
enum_variant* get_variant(type* restrict e, size_t i);

void  set_target(type* restrict p, type* restrict dest);
type* restrict get_target(type* restrict p);

u64   get_index(type* restrict t);

type* restrict get_type_from_num(u16 num, int num_set);

bool are_equivalent(type* restrict a, type* restrict b, bool* executed_DSA);
bool is_element_equivalent(type* restrict a, type* restrict b, int num_set_a, int num_set_b);

void locally_number(type* restrict t, u64* number, int num_set);
void reset_numbers(int num_set);

void canonicalize();
void merge_type_references(type* restrict dest, type* restrict src, bool disable);

void print_type_graph();

// a < b
bool variant_less(enum_variant* a, enum_variant* b);