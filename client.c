#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "server.h"
#include "client.h"
#include "history.h"

void client_activate(int sd, size_t num) {
    int res;
    enum CLIENT_REQUEST type = CR_ACTIVATE_PASTABLE;

    send(sd, &type, sizeof(type), 0);
    send(sd, &num, sizeof(num), 0);
    recv(sd, &res, sizeof(res), 0);
    printf("activated %ld\n", num);
}

int client_connect(const char *path) {
    int sd;
    struct sockaddr_un addr;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    sd = socket(AF_UNIX, SOCK_STREAM, 0);
    return connect(sd, (struct sockaddr*)&addr, sizeof addr) == -1 ? -1 : sd;
}

void client_set_clipboard(int sd, char *content, ssize_t sz) {
    enum CLIENT_REQUEST type = CR_SET_CLIPBOARD;
    send(sd, &type, sizeof(type), 0);
    send(sd, &sz, sizeof(sz), 0);
    send(sd, content, sz, 0);
}

void client_clear_history(int sd) {
    enum CLIENT_REQUEST type = CR_CLEAR;
    send(sd, &type, sizeof(type), 0);
}

void client_print_history(int sd, int num) {
    char *resp;
    size_t sz, cap;
    ssize_t n, i;

    char buf[200];
    enum CLIENT_REQUEST type = CR_HIST;
    send(sd, &type, sizeof(type), 0);

    resp = NULL;
    cap = 0;
    sz = 0;

    while((n=recv(sd, buf, sizeof buf, 0)) > 0) {
        if(sz+n > cap)
            resp = realloc(resp, (sz+n)*2);
        memcpy(resp + sz, buf, n);
        sz+=n;
    }

    i = 0;
    n = 0;
    while(n < (size_t)sz) {
        size_t item_sz;
        char *item;

        item_sz = *(size_t*)(resp+n);
        item = resp+n+sizeof(size_t);

        if(num == -1) {
            char *c, *res;
            res = malloc(item_sz+3);
            memcpy(res, item, item_sz);
            if((c=strchr(res, '\n')))
                strcpy(c, "...");
            printf("%ld: %s\n", i, res);
            free(res);
        } else if(num == i) {
            printf("%s", item);
            return;
        }

        n += item_sz + sizeof(size_t);
        i++;
    }
}
