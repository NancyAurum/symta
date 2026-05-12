#ifndef _CTX_H_
#define _CTX_H_

// page size
#define CTX_PGSZ 4096

/* Execution context related functions. */

typedef struct ctx_t {unsigned char data[2560];} ctx_t;
typedef struct ctx_error_t {
  int id;            // numeric id of this error
  char *text;        // textual description of this error
  void *mem;         // memory, accessing which resulted into this error
  void *ctx;         // CPU context, under which this error occured
} ctx_error_t;

// Context error codes
#define CTXE_OTHER    0x00  /* some unspecified error */
#define CTXE_ACCESS   0x01  /* memory access violation */
#define CTXE_DIV_BY_ZERO 0x02
#define CTXE_DIV_BY_ZERO_FPU 0x03
#define CTXE_STACK_OVERFLOW 0x04


#define CTXE_ABORT    0x0
#define CTXE_CONTINUE 0x1

void *ctx_function_at(void *ip);  /* gets function base address for given ip */
void *ctx_module_at(void *ip); /* gets module base address for given ip */
void *ctx_unwind(void *ctx); /* stores ctx's caller context into ctx */
void ctx_save(void *ctx); /* stores current context into ctx */
void ctx_load(void *ctx); /* restores current context from ctx */
void *ctx_ip(void *ctx); /* gets instruction pointer */
void *ctx_sp(void *ctx); /* gets stack pointer */
void ctx_set_error_handler(int (*error_handler)(ctx_error_t *info));

#endif /*  _CTX_H_ */
