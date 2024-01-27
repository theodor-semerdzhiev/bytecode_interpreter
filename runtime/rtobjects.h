#pragma once
#include <stdbool.h>
#include "builtins.h"
#include "rtlists.h"
#include "rtfunc.h"
#include "rtmap.h"
#include "rtclass.h"
#include "rtstring.h"

// // Possible runtime types
typedef enum RtType
{
    UNDEFINED_TYPE,
    NULL_TYPE,
    NUMBER_TYPE,
    STRING_TYPE,
    CLASS_TYPE,
    FUNCTION_TYPE,
    LIST_TYPE,
    HASHMAP_TYPE,
    HASHSET_TYPE,
} RtType;

#define DEFAULT_REF 8

// Forward declaration
typedef struct RtObject RtObject;
typedef struct ByteCodeList ByteCodeList;
typedef struct RtList RtList;
typedef struct RtMap RtMap;
typedef struct Identifier Identifier;
typedef struct RtClass RtClass;

// Generic object for all variables
typedef struct RtObject
{
    RtType type;

    union
    {
        long Integer;

        long double Number;

        RtString *String;

        RtFunction *Func;

        RtList *List;

        RtMap *Map;

        RtClass *Class;
        // HashSet *set;
    } data;

    bool mark; // used for garbage collector
} RtObject;

RtObject *init_RtObject(RtType type);

char *rtobj_toString(const RtObject *obj);
RtObject *multiply_objs(RtObject *obj1, RtObject *obj2);

RtObject *add_objs(RtObject *obj1, RtObject *obj2);
RtObject *substract_objs(RtObject *obj1, RtObject *obj2);
RtObject *divide_objs(RtObject *obj1, RtObject *obj2);
RtObject *modulus_objs(RtObject *obj1, RtObject *obj2);
RtObject *exponentiate_obj(RtObject *base, RtObject *exponent);
RtObject *bitwise_and_objs(RtObject *obj1, RtObject *obj2);
RtObject *bitwise_or_objs(RtObject *obj1, RtObject *obj2);
RtObject *bitwise_xor_objs(RtObject *obj1, RtObject *obj2);
RtObject *shift_left_objs(RtObject *obj1, RtObject *obj2);
RtObject *shift_right_objs(RtObject *obj1, RtObject *obj2);
RtObject *greater_than_op(RtObject *obj1, RtObject *obj2);
RtObject *greater_equal_op(RtObject *obj1, RtObject *obj2);
RtObject *lesser_than_op(RtObject *obj1, RtObject *obj2);
RtObject *lesser_equal_op(RtObject *obj1, RtObject *obj2);
RtObject *equal_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_and_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_or_op(RtObject *obj1, RtObject *obj2);
RtObject *logical_not_op(RtObject *target);

bool eval_obj(const RtObject *obj);
unsigned int rtobj_hash(const RtObject *obj);
bool rtobj_equal(const RtObject *obj1, const RtObject *obj2);

RtObject *rtobj_shallow_cpy(const RtObject *obj);
RtObject *rtobj_deep_cpy(const RtObject *obj);

RtObject *rtobj_mutate(RtObject *target, const RtObject *new_value, bool deepcpy);
RtObject *rtobj_getindex(RtObject *obj, RtObject *index);

RtObject **rtobj_getrefs(const RtObject *obj);
void *rtobj_getdata(const RtObject *obj);

void rtobj_free_data(RtObject *obj, bool free_immutable);
void rtobj_free(RtObject *obj, bool free_immutable);
void rtobj_shallow_free(RtObject *obj);
void rtobj_deconstruct(RtObject *obj, int offset);