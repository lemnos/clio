#include "history.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

struct hist_ent *history = NULL;

void history_clear() {
    struct hist_ent *it = history;

    while (it) {
        struct hist_ent *tmp;
        free(it->data);
        tmp = it;
        it = it->next;
        free(tmp);
    }

    history = NULL;
}

void history_raise(size_t num) {
    size_t i;
    struct hist_ent **ent, *tmp;

    for(i=0,ent = &history;*ent;ent=&(*ent)->next,i++) {
        if(i == num) {
            tmp = *ent;
            *ent = (*ent)->next;
            tmp->next = history;
            history = tmp;
            break;
        }
    }
}

void history_add_paste(char *data, size_t data_sz) {
    struct hist_ent *nh;

    for(struct hist_ent **ent = &history;*ent;ent = &(*ent)->next) {
        if(data_sz == (*ent)->data_sz-1 && 
           !memcmp((*ent)->data, data, data_sz)) {
            nh = *ent;
            *ent = (*ent)->next;
            nh->next = history;
            history = nh;
            return;
        }
    }

    nh = malloc(sizeof(struct hist_ent));

    nh->data = malloc(data_sz+1);
    memcpy(nh->data, data, data_sz);
    nh->data[data_sz] = '\0';

    nh->data_sz = data_sz+1;
    nh->next = history;
    history = nh;
}
