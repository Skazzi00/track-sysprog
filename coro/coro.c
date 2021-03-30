#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <signal.h>
#include <stdint.h>

#include "coro.h"
static void free_coro(coro_t *coro) {
  free(coro->stack);
  free(coro->args);
  free(coro);
}


static void free_coro_node(coro_node *node) {
  free_coro(node->coro);
  free(node);
}

coro_scheduler new_coro_scheduler() {
  coro_scheduler result;
  result.list = NULL;
  return result;
}

static void *allocate_stack() {
  void *stack = malloc(stack_size);
  stack_t ss;
  ss.ss_sp = stack;
  ss.ss_size = stack_size;
  ss.ss_flags = 0;
  sigaltstack(&ss, NULL);
  return stack;
}

#ifdef __x86_64__
union ptr_splitter {
  void *ptr;
  uint32_t part[sizeof(void *) / sizeof(uint32_t)];
};

static void coro_entry(uint32_t p0, uint32_t p1) {
  union ptr_splitter p;
  p.part[0] = p0;
  p.part[1] = p1;
  coro_t *coro = p.ptr;
  coro->function(coro);
  coro->state = CORO_FINISHED;
  coro_yield(coro);
}

#else
static void coro_entry(coro_t * coro) {
    coro->function(coro);
    coro->state = CORO_FINISHED;
    coro_yield(coro);
}
#endif

static void add_coro(coro_scheduler *scheduler, coro_t *coro) {
  coro_node *node = calloc(1, sizeof(coro_node));
  node->coro = coro;
  node->next = scheduler->list;
  scheduler->list = node;
}

coro_t *new_coro(coro_scheduler *scheduler, coro_function_t func, void *args) {
  coro_t *coro = calloc(1, sizeof(coro_t));
  coro->state = CORO_NEW;
  coro->stack = allocate_stack();
  coro->function = func;
  coro->args = args;

  getcontext(&coro->context);
  coro->context.uc_stack.ss_sp = coro->stack;
  coro->context.uc_stack.ss_size = stack_size;
  coro->context.uc_link = 0;

#ifdef __x86_64__
  union ptr_splitter p;
  p.ptr = coro;
  makecontext(&coro->context, (void (*)()) coro_entry, 2, p.part[0], p.part[1]);
#else
  makecontext(&coro->context, (void (*)()) coro_entry, 1, coro);
#endif
  add_coro(scheduler, coro);
  return coro;
}

void coro_start(coro_scheduler *scheduler) {
  coro_node *prev = NULL;
  coro_node *cur = scheduler->list;
  while (scheduler->list != NULL) {
    coro_t *coro = cur->coro;
    if (coro->state == CORO_NEW)
      coro->state = CORO_RUNNING;
    if (coro->state == CORO_RUNNING) {
      swapcontext(&coro->sched_ctx, &coro->context);
    }
    coro_node *next = cur->next;
    if (coro->state == CORO_FINISHED) {
      if (prev == NULL)
        scheduler->list = cur->next;
      else
        prev->next = cur->next;

      free_coro_node(cur);
    } else {
      prev = cur;
    }
    cur = next;
    if (cur == NULL) {
      cur = scheduler->list;
      prev = NULL;
    }
  }
}

void coro_yield(coro_t *coro) {
  if (!coro) return;
  swapcontext(&coro->context, &coro->sched_ctx);
}
