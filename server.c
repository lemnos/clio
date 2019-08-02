#include <sys/un.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "server.h"
#include "history.h"

static void clear_history() {
    history_clear();
}

static void send_history(int sd) {
    for(struct hist_ent *ent = history;ent;ent=ent->next) {
        //We can ignore type and endianness issues since the client
        //will be run on the same machine.
        send(sd, &ent->data_sz, sizeof(size_t), 0);
        send(sd, ent->data, ent->data_sz, 0);
    }

    close(sd);
}

void server_handle_connection(int client) {
    enum CLIENT_REQUEST type;
    recv(client, &type, sizeof type, 0);

    switch(type) {
        size_t num;
        int res = 0;
        char *content;
        ssize_t sz;

    case CR_SET_CLIPBOARD:
        recv(client, &sz, sizeof(sz), MSG_WAITALL);
        content = malloc(sz);
        recv(client, content, sz, MSG_WAITALL);

        history_add_paste(content, sz);
        free(content);
        break;
    case CR_CLEAR:
        clear_history();
        break;
    case CR_HIST:
        send_history(client);
        break;
    case CR_ACTIVATE_PASTABLE:
        recv(client, &num, sizeof(num), MSG_WAITALL);

        history_raise(num);

        send(client, &res, sizeof(res), 0);
        break;
    }

    close(client);
}

int server_create(const char *path) {
    int sd;
    struct sockaddr_un addr;

    if((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("server socket");
        exit(1);
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    if(bind(sd, (struct sockaddr*)&addr, sizeof addr) == -1) {
        perror("binding server socket");
        exit(1);
    }

    if(listen(sd, 10) == -1) {
        perror("listening on server socket");
        exit(1);
    }

    return sd;
}

