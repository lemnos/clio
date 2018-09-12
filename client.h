#ifndef _CLIENT_H_
#define _CLIENT_H_
void client_print_history(int sd, int num);
int client_connect(const char *path);
void client_activate(int sd, size_t num);
void client_clear_history(int sd);
void client_name(int sd, const char *name);
void client_set_clipboard(int sd, char *content, ssize_t sz);
#endif
