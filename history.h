#ifndef _HISTORY_H_
#define _HISTORY_H_
#include <stddef.h>

struct hist_ent {
    char *data;
    size_t data_sz;
    struct hist_ent *next;
};

extern struct hist_ent *history;
void history_add_paste(char *data, size_t data_sz);
void history_raise(size_t num);
void history_clear();
#endif
