#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <aio.h>
#include <unistd.h>
#include <string.h>

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct {
  int* data;
  int size;
} IntArray;

typedef struct {
  char* data;
  int size;
} StringView;

int count_numbers(const char* data) {
  int cnt = 0;
  while (*data && (data = strchr(data, ' ')) != NULL) {
    cnt++;
    data++;
  }
  return cnt + 1;
}

IntArray parse_numbers(StringView str) {
  IntArray result = {0};
  result.size = count_numbers(str.data);
  result.data = malloc(result.size * sizeof(int));
  if (result.data == NULL)
    handle_error("malloc");

  char* cur = str.data;
  for (int i = 0; i < result.size; i++) {
    result.data[i] = atoi(cur);
    cur = strchr(cur, ' ');
    cur++;
  }

  return result;
}

StringView read_file(const char* name) {
  StringView result = {0};
  int fd = open(name, O_RDONLY);

  struct stat st = {0};
  if (fstat(fd, &st) == -1)
    handle_error("fstat");

  result.size = st.st_size;
  result.data = malloc(result.size + 1);
  if (result.data == NULL)
    handle_error("malloc");

  struct aiocb aio_req = {0};
  aio_req.aio_fildes = fd;
  aio_req.aio_buf = result.data;
  aio_req.aio_offset = 0;
  aio_req.aio_nbytes = result.size;
  if (aio_read(&aio_req) == -1)
    handle_error("aio_read");

  while (aio_error(&aio_req) == EINPROGRESS) {
    //todo: yield
  }

  if (aio_return(&aio_req) == -1) {
    handle_error("aio_return");
  }

  close(fd);
  result.data[result.size] = '\0';

  return result;
}

void merge(IntArray a, IntArray b, IntArray* dest) {
  IntArray tmp = {0};
  tmp.size = a.size + b.size;
  tmp.data = malloc(tmp.size * sizeof(int));

  int curPos = 0;
  int aPos = 0;
  int bPos = 0;
  while (aPos < a.size && bPos < b.size) {
    if (a.data[aPos] < b.data[bPos]) {
      tmp.data[curPos++] = a.data[aPos++];
    } else {
      tmp.data[curPos++] = b.data[bPos++];
    }
  }

  while (aPos < a.size) {
    tmp.data[curPos++] = a.data[aPos++];
  }
  while (bPos < b.size) {
    tmp.data[curPos++] = b.data[bPos++];
  }

  if (!dest->data) {
    dest->data = tmp.data;
    dest->size = tmp.size;
    return;
  }

  for (int i = 0; i < tmp.size; i++) {
    dest->data[i] = tmp.data[i];
  }
  free(tmp.data);
}

void merge_sort(IntArray array) {
  if (array.size <= 1) return;
  IntArray left = {array.data, array.size / 2};
  IntArray right = {array.data + left.size, array.size - left.size};
  merge_sort(left);
  merge_sort(right);
  merge(left, right, &array);
}

IntArray sorted_read_file(const char* name) {
  StringView data = read_file(name);
  IntArray array = parse_numbers(data);
  merge_sort(array);
  free(data.data);
  return array;
}

void sort_files(int size, char* files[]) {
  IntArray* arrays = malloc(size * sizeof(IntArray));

  for (int i = 0; i < size; i++) {
    arrays[i] = sorted_read_file(files[i]);
  }


  for (int i = 1; i < size; i++) {
    IntArray tmp = {NULL, 0};
    merge(arrays[0], arrays[1], &tmp);
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

int main(int argc, char* argv[]) {
  sort_files(argc - 1, argv + 1);
  return 0;
}