#ifndef _SERVER_H_
#define _SERVER_H_

enum CLIENT_REQUEST {
    CR_HIST,
    CR_CLEAR,
    CR_SET_CLIPBOARD,
    CR_ACTIVATE_PASTABLE,
};

void server_handle_connection(int client);
int server_create(const char *path);
#endif
