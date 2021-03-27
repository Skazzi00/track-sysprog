
#ifndef TRACK_SYSPROG_CORO_CORO_H_
#define TRACK_SYSPROG_CORO_CORO_H_

#define stack_size 1024 * 1024

typedef struct coro_t_ coro_t;

typedef void (*coro_function_t)(coro_t *coro);

typedef enum {
  CORO_NEW,
  CORO_RUNNING,
  CORO_FINISHED
} coro_state_t;

struct coro_t_ {
  coro_state_t state;
  coro_function_t function;
  void *args;

  ucontext_t sched_ctx;
  ucontext_t context;
  char *stack;
};


typedef struct coro_node_ {
  struct coro_node_ *next;
  coro_t *coro;
} coro_node;

typedef struct {
  coro_node *list;
} coro_scheduler;

coro_scheduler new_coro_scheduler();

void coro_yield(coro_t *coro);

coro_t *new_coro(coro_scheduler *scheduler, coro_function_t func, void *args);

void coro_start(coro_scheduler *scheduler);

#endif //TRACK_SYSPROG_CORO_CORO_H_
