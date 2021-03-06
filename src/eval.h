#ifndef EVAL_H
#define EVAL_H

#include "defs.h"
#include "types.h"
#include "lobject.h"

/* the virtual machine in CMOO uses a stack where elements can be accessed 
 * in an indexed, register-like fashion as well. to do this we have a stack 
 * pointer SP, a frame pointer FP and an instruction pointer IP. 
 * The instructions are 1-octet in length but may be followed by variable 
 * number of arguments. The VM executes the current opcode at IP and then 
 * increases IP accordingly. The instructions can modify SP (e.g. with PUSH, 
 * POP) or access cells relative to FP. a sample 
 * layout within a method call:
 *
 * | intermediate0  | <- SP
 * | local0         |
 * | argument1      |
 * | argument0      | <- FP
 * +----------------+
 * | return address 
 * | previous FP    |
 * | ...            |
 *
 * In this case the current method has been called with two arguments, and has
 * reserved one local variable (at FP[2]). the intermediate value has been 
 * created with e.g. PUSH. Note that the arguments, locals and intermediate 
 * cells hold *values*, the return address and previous FP hold VM-internal 
 * addresses.
 *
 * When calling a method, the steck needs to have the arguments at the top, 
 * followed by an ignored slot, a string naming the method and an objref naming 
 * the object to call on:
 *
 * | argument0      | <- SP
 * | <ignored>      |
 * | method name    |
 * | callee objref  |
 * | intermediate0  | 
 * | local0         |
 * | argument1      |
 * | argument0      | <- FP
 * +----------------+
 * | return address | 
 * | previous FP    |
 * | ...            |
 * 
 * the code then executes a "CALL nargs" where "nargs" is the number of 
 * arguments on the stack. The CALL opcode will overwrite the ignored slot, the 
 * method name and objref before executing the callee, so the Stack, SP and FP 
 * in the called method look like this:
 *
 * | argument0      | <- SP, FP
 * +----------------+
 * | return object  |
 * | return address |
 * | previous FP    | --,
 * | intermediate0  |   |
 * | local0         |   |
 * | argument1      |   |
 * | argument0      | <-'
 * | return address |
 * | previous FP    |
 * | ...            |
 *
 * At some point the callee will call a "RETURN" instruction, which will 
 * restore the previous FP, put the returned value in the slot that 
 * contained it, and set SP to that as well:
 *
 * | returned value | <- SP
 * | intermediate0  | 
 * | local0         |
 * | argument1      |
 * | argument0      | <- FP
 * +----------------+
 * | return address | 
 * | previous FP    |
 * | ...            |
 * 
 * Note that you can use intermediate (PUSHed) values for the callee 
 * declaration and the arguments, or local variables. But if you do use local
 * variables then of course their contents will be undefined after the return.
 *
 * Also note that if you store your callee objref, method name and arguments
 * in locals, then after the callee returned these are the last members on the
 * stack, and you may have shrunk your args_locals guarantees, it is therefore
 * recommended to use the last locals on your stack for this.
 *
 * Also note that in a method with no arguments and locals, FP is initially 
 * beyond SP!
 *
 * A full description of the instruction set is in the documentation, the 
 * below is only a brief reference. things like reg8:src mean type:name, reg8 
 * is an 8-bit value denoting a cell relative to FP, src is what we read in 
 * the instruction.
 * */

#define OP_NOOP           0x00 // no-op
#define OP_HALT           0x01 // halt code execution
#define OP_DEBUGI         0x02 // int32:msg      
#define OP_DEBUGR         0x03 // reg8:msg
#define OP_MOV            0x04 // reg8:dst <= reg8:src
#define OP_PUSH           0x05 // SP++; SP <= reg8:src
#define OP_POP            0x06 // reg8:dst <= SP; SP--
#define OP_CALL           0x07 // int8:nargs
#define OP_RETURN         0x08 // reg8:value
#define OP_ARGS_LOCALS    0x09 // assert we have int8:nargs and 
                               // reserve int8:nlocals on the stack
#define OP_CLEAR          0x0A // reg8:dst <= NIL
#define OP_TRUE           0x0B // reg8:dst <= BOOL(TRUE)
#define OP_LOAD_INT       0x0C // reg8:dst <= int32:value
#define OP_LOAD_FLOAT     0x0D // reg8:dst <= float32:value
#define OP_LOAD_STRING    0x0E // reg8:dst <= int16 length, string:value
#define OP_TYPE           0x0F // reg8:dst <= type(reg8:src)
#define OP_LOGICAL_AND    0x10 // reg8:dst <= logical_and(reg8:src1, reg8:src2)
#define OP_LOGICAL_OR     0x11 // reg8:dst <= logical_or(reg8:src1, reg8:src2)
#define OP_LOGICAL_NOT    0x12 // reg8:dst <= local_not(reg8:src)
#define OP_EQ             0x13 // reg8:dst <= eq(reg8:src1, reg8:src2)
#define OP_LE             0x14 // reg8:dst <= le(reg8:src1, reg8:src2)
#define OP_LT             0x15 // reg8:dst <= lt(reg8:src1, reg8:src2)
#define OP_ADD            0x16 // reg8:dst <= reg8:src1 + reg8:src2
#define OP_SUB            0x17 // reg8:dst <= reg8:src1 - reg8:src2
#define OP_MUL            0x18 // reg8:dst <= reg8:src1 * reg8:src2
#define OP_DIV            0x19 // reg8:dst <= reg8:src1 / reg8:src2
#define OP_MOD            0x1A // reg8:dst <= reg8:src1 % reg8:src2
#define OP_JUMP           0x1B // IP += int32:offset
#define OP_JUMP_IF        0x1C // if reg8:src is TRUE then IP += int32:offset
#define OP_JUMP_EQ        0x1D // if eq(reg8:src1, reg8:src2) then IP += int32:offset
#define OP_JUMP_NE        0x1E // if ne(reg8:src1, reg8:src2) then IP += int32:offset
#define OP_JUMP_LE        0x1F // if le(reg8:src1, reg8:src2 then IP += int32:offset
#define OP_JUMP_LT        0x20 // if lt(reg8:src1, reg8:src2) then IP += int32:offset

// XXX need to be reordered
#define OP_SYSCALL        0x21 // reg8:ret <= reg8:nargs, args consumed from stack
#define OP_LENGTH         0x22 // reg8:dst <= strlen(reg8:src)
#define OP_CONCAT         0x23 // reg8:dst <= concat(reg8:src1, reg8:src2)

#define OP_GETGLOBAL      0x24 // reg8:ret <= reg8:name
#define OP_SETGLOBAL      0x25 // reg8:name, reg8:val
#define OP_MAKE_OBJ       0x26 // reg8:new_ref <= reg:parent_ref
#define OP_SELF           0x27 // reg8:id <= ID of current object
// XXX this is a stopgap, we could have multiple parents. but until we have
// lists...
#define OP_PARENT         0x28 // reg8:id <= ID of first parent or NIL if no parent
// XXX this is a temporary/debug aid to expose concurrency issues, this opcode
// should not be used in actual code
#define OP_USLEEP         0x29 // int32:microseconds to sleep

// XXX more ops

#define EVAL_OK             0   // evaluation finished successfully
#define EVAL_RETRY_TX       1   // indicates that evaluation failed recoveraby, e.g. a deadlock
// XXX need nonrecoverable error

struct eval_ctx;

struct syscall_table;
struct store_tx;

struct eval_ctx* eval_new_ctx(uint64_t task_id, struct store_tx *stx);
void eval_free_ctx(struct eval_ctx *ctx);

// set a callback that gets executed whenever OP_DEBUGI or OP_DEBUGR gets
// executed, this mainly for testing. whenever the callback gets executed, the
// argument a gets passed to it as well
void eval_set_dbg_handler(struct eval_ctx *ctx, 
    void (*callback)(val v, void *a), 
    void *a);

// execute a method on an object
// returns 0 on success and EVAL_RETRY_TX if the attempt should be retried
// because of a deadlock or stale lock
int eval_exec_method(struct eval_ctx *ctx, struct lobject *obj, val method, int num_args, ...);

// debug clutch to allow direct execution of code without objects and the like.
// This is only used by test drivers and debug code, not by an actual CMOO server
// returns like eval_exec_method (which is largely a wrapper around this)
int eval_exec(struct eval_ctx *ctx, opcode *code);
void eval_push_arg(struct eval_ctx *ctx, val v);

// create/destroy/get/set a syscall table
struct syscall_table* syscall_table_new(void);
void syscall_table_free(struct syscall_table *st);
struct syscall_table* eval_get_syscall_table(struct eval_ctx *ctx);
void eval_set_syscall_table(struct eval_ctx *ctx, struct syscall_table *st);

// set a context object that is being passed into each sycall invocation
void syscall_table_set_ctx(struct syscall_table *st, void *ctx);

// add syscalls to table, by arity
void syscall_table_add_a0(struct syscall_table *st, char *name, val (*syscall)(void*));
void syscall_table_add_a1(struct syscall_table *st, char *name, val (*syscall)(void*, val v1));
void syscall_table_add_a2(struct syscall_table *st, char *name, val (*syscall)(void*, val v1, val v2));
void syscall_table_add_a3(struct syscall_table *st, char *name, val (*syscall)(void*, val v1, val v2, val v3));

void eval_set_syscall_table(struct eval_ctx *ctx, struct syscall_table *st);

#endif /* EVAL_H */
