#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* cmoo is a dynamic language, so types are associated with values rather 
 * than with variables. we differ between immediate values where the whole
 * state can be represented by the value itself, and non-immediates where
 * there is external information that the value only refers to. examples
 * of an immediate are integers and booleans, an example for a non-immediate
 * is a string, because the actual character buffer is kept somewhere on the 
 * heap. there is a type for "special" items like file handles and a type
 * for object references. these two are somewhere inbetween or are not very
 * well classified by the system above. supported types: 
 * */

#define TYPE_NIL        0
#define TYPE_BOOL       1
#define TYPE_INT        2
#define TYPE_FLOAT      3
#define TYPE_STRING     4
#define TYPE_OBJREF     5
#define TYPE_SPECIAL    6

typedef uint64_t val;

/* this returns the type of a value */
int val_type(val v);

/* create values of a given type and initial value */
val val_make_nil(void);
val val_make_bool(bool i);
val val_make_int(int i);
val val_make_float(float i);
val val_make_string(char *s); // copies, does not consume argument
// XXX more creators

/* sets the value pointed to to NIL and runs cleanups for non-immediates
 * as required */
void val_clear(val *v);
/* also set to NIL, but performs no cleanups */
void val_init(val *v);

void val_inc_ref(val v);
void val_dec_ref(val v);

/* get the value assuming that the type is correct */
bool val_get_bool(val v);
int val_get_int(val v);
float val_get_float(val v);
char* val_get_string(val v);
// XXX more getters


#endif /* TYPES_H */
