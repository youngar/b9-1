

#include "b9.h"

static Instruction fib_function[] = {
    // one argument, 0 temps
    decl(1, 0),
    decl(0,0),
    decl(0,0),

    // if (arg1 < 3) return 1;
    // so converted to jump if 3 <= arg1
    createInstruction(PUSH_CONSTANT, 3),
    createInstruction(PUSH_FROM_VAR, 0),
    createInstruction(JMPLE, 2), // SKIP 2 instructions

    // return 1;
    createInstruction(PUSH_CONSTANT, 1), // 1
    createInstruction(RETURN, 0), // 2

    // return fib (n-1); + fib (n-2);
    createInstruction(PUSH_FROM_VAR, 0),
    createInstruction(PUSH_CONSTANT, 1),
    createInstruction(SUB, 0),
    createInstruction(CALL, 1), // fib (n-1)

    createInstruction(PUSH_FROM_VAR, 0),
    createInstruction(PUSH_CONSTANT, 2),
    createInstruction(SUB, 0),
    createInstruction(CALL, 1), // fib (n-2)

    createInstruction(ADD, 0),

    createInstruction(RETURN, 0),
    createInstruction(NO_MORE_BYTECODES, 0)
};

static Instruction test_function[] = {
    decl(0,0),
    decl(0,0),
    decl(0,0),
    createInstruction(PUSH_CONSTANT, 12),
    createInstruction(PUSH_CONSTANT, 99),
    createInstruction(CALL, 2),
    createInstruction(RETURN, 0),
    createInstruction(NO_MORE_BYTECODES, 0)
};

static Instruction test_function2[] = {
    decl(2,0),
    decl(0,0),
    decl(0,0),
    createInstruction(PUSH_FROM_VAR, 0),
    createInstruction(PUSH_FROM_VAR, 1),
    createInstruction(ADD, 0), 
    createInstruction(RETURN, 0),
    createInstruction(NO_MORE_BYTECODES, 0)
};

static Instruction main_function[] = {
    decl(0, 0),
    decl(0,0),
    decl(0,0),
    createInstruction(PUSH_CONSTANT, 12),
    createInstruction(CALL, 1),
    createInstruction(RETURN, 0),
    createInstruction(NO_MORE_BYTECODES, 0)
};

static Instruction loop_call_fib12_function[] = {
    decl(1, 1),
    decl(0,0),
    decl(0,0),
    createInstruction(PUSH_FROM_VAR, 0),
    createInstruction(POP_INTO_VAR, 1), // t = 50000

    // if (t <= 0)  jmp exit
    // loop test
    createInstruction(PUSH_FROM_VAR, 1),
    createInstruction(PUSH_CONSTANT, 0),
    createInstruction(JMPLE, 8), // SKIP to past the JMP

    createInstruction(PUSH_CONSTANT, 2), // 1
    createInstruction(CALL, 1), // 2
    createInstruction(DROP, 0), // 3

    // t--;
    createInstruction(PUSH_FROM_VAR, 1), // 4
    createInstruction(PUSH_CONSTANT, 1), // 5
    createInstruction(SUB, 0), // 6
    createInstruction(POP_INTO_VAR, 1), // 7

    createInstruction(JMP, -11), // 8

    // exit
    createInstruction(PUSH_CONSTANT, 999),
    createInstruction(RETURN, 0),
    createInstruction(NO_MORE_BYTECODES, 0)
};

/* Byte Code Program */
static Instruction* functions[] = {
    main_function,
    fib_function,
    test_function2
};

/* Byte Code Implementations */

void push(ExecutionContext* context, uint16_t value)
{
    *context->stackPointer = value;
    ++(context->stackPointer);
}

uint16_t
pop(ExecutionContext* context)
{
    return *(--context->stackPointer);
}

void bc_call(ExecutionContext* context, uint16_t value)
{
    Instruction* program = functions[value];
    // printf("inside call\n");
    uint16_t result = interpret(context, program);
    //  printf("return from  call %d\n", result);

    push(context, result);
}

void bc_push_from_arg(ExecutionContext* context, uint16_t* args, uint16_t offset)
{
  //  printf("bc_push_from_arg[%d] %d\n", offset, args[offset]);
    push(context, args[offset]);
}

void bc_pop_into_arg(ExecutionContext* context, uint16_t* args, uint16_t offset)
{
    args[offset] = pop(context);
 //   printf("bc_pop_into_arg[%d] %d\n", offset, args[offset]);
}

void bc_drop(ExecutionContext* context)
{
    pop(context);
}

void bc_push_constant(ExecutionContext* context, uint16_t value)
{
 //   printf("bc_push_constant %d\n", value);
    push(context, value);
}

uint16_t
bc_jmple(ExecutionContext* context, uint16_t delta)
{

    // push(left); push(right); if (left <= right) jmp
    int16_t right = pop(context);
    int16_t left = pop(context);
   // printf("jmple left %d, right %d\n", left, right);
    if (left <= right) {
        return delta;
    }
    return 0;
}

void bc_add(ExecutionContext* context)
{
    // a+b is push(a);push(b); add
    uint16_t right = pop(context);
    uint16_t left = pop(context);
    // printf("add right %d left %d\n", right, left);
    uint16_t result = left + right;

    push(context, result);
}

void bc_sub(ExecutionContext* context)
{
    // left-right is push(left);push(right); sub
    uint16_t right = pop(context);
    uint16_t left = pop(context);
    uint16_t result = left - right;

    push(context, result);
}

/* ByteCode Interpreter */

uint16_t interpret(ExecutionContext* context, Instruction* program)
{

    uint64_t *address = (uint64_t *) (&program[1]);
    Interpret jitedcode = (Interpret) *address;
    if (jitedcode != NULL) {
        printf("about to call jit");
        uint16_t result = (*jitedcode) (context, program);
        printf("jit result is: %d\n", result);
        return result;
    }

    int nargs = progArgCount(*program);
    int tmps = progTmpCount(*program);

    //printf("Prog Arg Count %d, tmp count %d\n", nargs, tmps);

    Instruction* instructionPointer = program + 3;
    uint16_t* args = context->stackPointer - nargs;
    context->stackPointer += tmps; // local storage for temps

    while (*instructionPointer != NO_MORE_BYTECODES) {
        // uint16_t* base = context->stack;
        // printf("------\n");
        // while (base < context->stackPointer) {
        //     printf("Stack[%ld] = %d\n", base - context->stack, *base);
        //     base++;
        // }
        // printf("^^^^^^^^^^^^^^^^^\n");
        //printf("about to run %d %d\n", getByteCodeFromInstruction(*instructionPointer), getParameterFromInstruction(*instructionPointer));
        switch (getByteCodeFromInstruction(*instructionPointer)) {
        case PUSH_CONSTANT:
           // printf("push\n");
            bc_push_constant(context, getParameterFromInstruction(*instructionPointer));
            break;
        case DROP:
          //  printf("drop\n");
            bc_drop(context);
            break;
        case ADD:
          //  printf("add\n");
            bc_add(context);
            break;
        case SUB:
          //  printf("sub\n");
            bc_sub(context);
            break;
        case JMPLE:
          //  printf("jmple\n");
            instructionPointer += bc_jmple(context, getParameterFromInstruction(*instructionPointer));
            break;
        case CALL:
           // printf("call\n");
            bc_call(context, getParameterFromInstruction(*instructionPointer));
            break;
        case PUSH_FROM_VAR:
           // printf("push_from_var\n");
            bc_push_from_arg(context, args, getParameterFromInstruction(*instructionPointer));
            break;
        case POP_INTO_VAR:
          //  printf("pop_into_var\n");
            bc_pop_into_arg(context, args, getParameterFromInstruction(*instructionPointer));
            break;
        case JMP:
           // printf("jumping %d\n", getParameterFromInstruction(*instructionPointer));
            instructionPointer += getParameterFromInstruction(*instructionPointer);
            break;
        case RETURN:
          //  printf("return\n");
            /* bc_return */
            int16_t result = *(context->stackPointer - 1);
            context->stackPointer = args;
            return result;
            break;
        }
        instructionPointer++;
    }
    return *(context->stackPointer - 1);
}

void b9PrintStack(ExecutionContext *context) {

  uint16_t *base = context->stack;
  printf("------\n");
  while (base < context->stackPointer) {
    printf("%p: Stack[%ld] = %d\n", base, base - context->stack, *base);
    base++;
  }
  printf("^^^^^^^^^^^^^^^^^\n");
}

/* Main Loop */


int main()
{
    b9_jit_init();

    ExecutionContext context;
    context.functions = functions;

    uint16_t result = 0;

    push (&context, 1);
    push (&context, 2);
    printf("main: context.stackPointer = %p\n",context.stackPointer);
    result = interpret(&context, test_function2); 
    printf("!!!!!!!  interpreted: result is supposed to be 3, result is: %d\n", result);

    generateCode(test_function2);
    push (&context, 3);
    push (&context, 4);
    printf("main: context.stackPointer = %p\n",context.stackPointer);
    result = interpret(&context, test_function2); 
    printf("!!!!!!!  jitted: result is supposed to be 7, result is: %d\n", result);


    printf("main: context.stackPointer = %p\n",context.stackPointer);
    result = interpret(&context, test_function);
    printf("!!!!!!!  interpreted: result is supposed to be 111, result is: %d\n", result);
    printf("result is: %d\n", result);

    generateCode(test_function);
    printf("main: context.stackPointer = %p\n",context.stackPointer);
    result = interpret(&context, test_function); 
    printf("!!!!!!!  jitted: result is supposed to be 111, result is: %d\n", result);

    // generateCode(test_function);
    // uint64_t *address = (uint64_t *) (&test_function[1]);
    // Interpret jitedcode = (Interpret) *address;
    // result = (*jitedcode) (&context, test_function);
    // printf("result is: %d\n", result);

    // run main, which just calls fib 12
    // result = interpret(&context, main_function);
    // printf("Program result is: %d\n", result);

    // run a hard loop of fib(12)
    // int count = 1000;
    // while (count--) { 
    //     push (&context, 10000);
    //     result = interpret(&context, loop_call_fib12_function); 
    //     //context.stackPointer = context.stack;
    // }
    // printf("result is: %d\n", result);

    return 0;
}