#ifndef UBIQUITOUS_OCTO_CARNIVAL_HTTP__FILE_H_
#define UBIQUITOUS_OCTO_CARNIVAL_HTTP__FILE_H_

struct file_data {
  int size;
  void *data;
};

extern struct file_data *file_load(char *filename);
extern void file_free(struct file_data *filedata);

#endif //UBIQUITOUS_OCTO_CARNIVAL_HTTP__FILE_H_
