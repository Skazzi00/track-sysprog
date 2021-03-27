#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <aio.h>
#include <unistd.h>
#include <string.h>

#include "coro.h"

#define $yield coro_yield(coro)

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct {
  int *data;
  int size;
} IntArray;

typedef struct {
  char *data;
  int size;
} StringView;

int count_numbers(const char *data, coro_t *coro) {
  $yield;
  int cnt = 0;
  $yield;
  while (*data && (data = strchr(data, ' ')) != NULL) {
    $yield;
    cnt++;
    $yield;
    data++;
  }
  return cnt + 1;
}

IntArray parse_numbers(StringView str, coro_t *coro) {
  IntArray result = {0};
  $yield;
  result.size = count_numbers(str.data, coro);
  $yield;
  result.data = calloc(result.size, sizeof(int));
  $yield;
  if (result.data == NULL)
    handle_error("calloc");

  $yield;
  char *cur = str.data;
  $yield;
  for (int i = 0; i < result.size; i++) {
    $yield;
    result.data[i] = atoi(cur);
    $yield;
    cur = strchr(cur, ' ');
    $yield;
    cur++;
    $yield;
  }

  return result;
}

StringView read_file(const char *name, coro_t *coro) {
  StringView result = {0};
  $yield;
  int fd = open(name, O_RDONLY);
  $yield;

  struct stat st = {0};
  $yield;

  if (fstat(fd, &st) == -1)
    handle_error("fstat");
  $yield;

  result.size = st.st_size;
  $yield;
  result.data = calloc(result.size + 1, sizeof(char));
  $yield;
  if (result.data == NULL)
    handle_error("calloc");
  $yield;

  struct aiocb aio_req = {0};
  $yield;
  aio_req.aio_fildes = fd;
  $yield;
  aio_req.aio_buf = result.data;
  $yield;
  aio_req.aio_offset = 0;
  $yield;
  aio_req.aio_nbytes = result.size;
  $yield;
  if (aio_read(&aio_req) == -1)
    handle_error("aio_read");
  $yield;

  while (aio_error(&aio_req) == EINPROGRESS) {
    $yield;
  }

  if (aio_return(&aio_req) == -1) {
    handle_error("aio_return");
  }
  $yield;

  close(fd);
  $yield;
  result.data[result.size] = '\0';

  return result;
}

void merge(IntArray a, IntArray b, IntArray *dest, coro_t *coro) {
  IntArray tmp = {0};
  $yield;
  tmp.size = a.size + b.size;
  $yield;
  tmp.data = calloc(tmp.size, sizeof(int));
  $yield;

  int curPos = 0;
  $yield;
  int aPos = 0;
  $yield;
  int bPos = 0;
  $yield;
  while (aPos < a.size && bPos < b.size) {
    $yield;
    if (a.data[aPos] < b.data[bPos]) {
      $yield;
      tmp.data[curPos++] = a.data[aPos++];
    } else {
      $yield;
      tmp.data[curPos++] = b.data[bPos++];
    }
    $yield;
  }

  while (aPos < a.size) {
    $yield;
    tmp.data[curPos++] = a.data[aPos++];
  }
  while (bPos < b.size) {
    $yield;
    tmp.data[curPos++] = b.data[bPos++];
  }

  $yield;
  if (!dest->data) {
    $yield;
    dest->data = tmp.data;
    $yield;
    dest->size = tmp.size;
    return;
  }

  $yield;
  for (int i = 0; i < tmp.size; i++) {
    $yield;
    dest->data[i] = tmp.data[i];
  }
  $yield;
  free(tmp.data);
}

void merge_sort(IntArray array, coro_t *coro) {
  if (array.size <= 1) return;
  $yield;
  IntArray left = {array.data, array.size / 2};
  $yield;
  IntArray right = {array.data + left.size, array.size - left.size};
  $yield;
  merge_sort(left, coro);
  $yield;
  merge_sort(right, coro);
  $yield;
  merge(left, right, &array, coro);
}

typedef struct {
  const char *name;
  IntArray *ret_arr;
} sorted_read_file_args;

void sorted_read_file(coro_t *coro) {
  sorted_read_file_args *args = coro->args;
  $yield;
  StringView data = read_file(args->name, coro);
  $yield;
  IntArray array = parse_numbers(data, coro);
  $yield;
  merge_sort(array, coro);
  $yield;
  free(data.data);
  $yield;
  args->ret_arr->size = array.size;
  $yield;
  args->ret_arr->data = array.data;
}

void sort_files(int size, char *files[]) {
  IntArray *arrays = calloc(size, sizeof(IntArray));
  coro_scheduler scheduler = new_coro_scheduler();
  for (int i = 0; i < size; i++) {
    sorted_read_file_args *args = calloc(1, sizeof(sorted_read_file_args));
    args->name = files[i];
    args->ret_arr = &arrays[i];
    new_coro(&scheduler, sorted_read_file, args);
  }
  coro_start(&scheduler);

  for (int i = 1; i < size; i++) {
    IntArray tmp = {NULL, 0};
    merge(arrays[0], arrays[1], &tmp, NULL);
    free(arrays[0].data);
    free(arrays[1].data);
    arrays[0] = tmp;
    arrays[1].data = NULL;
    arrays[1].data = NULL;
  }

  for (int i = 0; i < arrays[0].size; i++) {
    printf("%d ", arrays[0].data[i]);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  sort_files(argc - 1, argv + 1);
  return 0;
}