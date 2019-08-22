#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <fcntl.h>

#include "server.h"
#include "client.h"
#include "history.h"

#define dbg(...) if(debug) fprintf(stderr, __VA_ARGS__);
char SERVER_SOCKET[PATH_MAX];
char SERVER_SOCKET_DIR[PATH_MAX];
Display *dpy;
Window win;
Atom XA_UTF8, XA_CLIPBOARD, XA_TARGETS;
int own_clipboard = 0;
int own_primary = 0;
int debug = 0;

static inline Atom intern(char *name) {
    Atom a = XInternAtom(dpy, name, False);
    if(a == None) {
        fprintf(stderr, "X11 ERROR: Failed to intern atom: %s\n", name);
        exit(1);
    }

    return a;
}

static void init_atoms() {
    XA_CLIPBOARD = intern("CLIPBOARD");
    XA_UTF8 = intern("UTF8_STRING");
    XA_TARGETS = intern("TARGETS");
}

//Note, the result is not NULL terminated (sz is all bytes excluding a NULL character).
static char *get_utf8_property(Display *dpy, Window win, Atom prop, size_t *sz) {
    Atom type;
    int fmt;
    unsigned long ni, nb;
    unsigned char *data;

    XGetWindowProperty(dpy,
            win,
            prop,
            0,
            0,
            False,
            AnyPropertyType,
            &type,
            &fmt,
            &ni,
            &nb,
            &data);

    if(type == None) {
        *sz = 0;
        return NULL;
    }

    assert(type == XA_UTF8 || type == XA_STRING);

    *sz = nb;
    XGetWindowProperty(dpy,
            win,
            prop,
            0,
            nb/4+1,
            False,
            type,
            &type,
            &fmt,
            &ni,
            &nb,
            &data);

    assert(nb == 0);
    return (char*)data;
}

static void handle_xev(XEvent *_ev) {
    switch(_ev->type) {
    case SelectionClear: {
        XSelectionClearEvent *ev = (XSelectionClearEvent*)_ev;
        if(ev->selection == XA_PRIMARY)
            own_primary = 0;
        else
            own_clipboard = 0;
        break;
    }
    case SelectionRequest: {
        XSelectionRequestEvent *ev = (XSelectionRequestEvent*)_ev;

        XSelectionEvent reply = {0};

        reply.type = SelectionNotify;
        reply.display = dpy;
        reply.selection = ev->selection;
        reply.property = ev->property;
        reply.target = ev->target;
        reply.requestor = ev->requestor;
        reply.time = ev->time;

        if (ev->target == XA_TARGETS) {
            const Atom targets[] = {XA_UTF8, XA_STRING};

            XChangeProperty(dpy,
                    ev->requestor,
                    ev->property,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char*)&targets,
                    sizeof(targets)/sizeof(targets[0]));

            XSendEvent(dpy, ev->requestor, True, 0, (XEvent*)&reply);
        } else if (ev->target == XA_UTF8 || ev->target == XA_STRING) {
            if(history) {
                XChangeProperty(dpy,
                        ev->requestor,
                        ev->property,
                        ev->target,
                        8,
                        PropModeReplace,
                        (unsigned char*)history->data,
                        history->data_sz-1); //Do not send padded NULL byte

                XSendEvent(dpy, ev->requestor, True, 0, (XEvent*)&reply);
            }
        } else {
            fprintf(stderr, "Unrecognized selection request target: %s from %lu (cwin = %lu)\n",
                    XGetAtomName(dpy, ev->target), ev->requestor, win);
        }

        break;
    }
    case SelectionNotify: {
        size_t sz;
        char *content;

        XSelectionEvent *ev = (XSelectionEvent*)_ev;

        if((own_clipboard && ev->selection == XA_CLIPBOARD) ||
                (own_primary && ev->selection == XA_PRIMARY)) 
            break;

        content = get_utf8_property(dpy, win, ev->selection, &sz);
        if(!content) break;

        if(!history || history->data_sz-1 != sz ||
                memcmp(history->data, content, sz)) {
            history_add_paste(content, sz);

            if(ev->selection == XA_PRIMARY) {
                XSetSelectionOwner(dpy, XA_CLIPBOARD, win, CurrentTime);
                own_clipboard = 1;
            } else {
                XSetSelectionOwner(dpy, XA_PRIMARY, win, CurrentTime);
                own_primary = 1;
            }

            XFlush(dpy);
        }

        free(content);

        break;
    }
    }
}

void cleanup(int _) {
    unlink(SERVER_SOCKET);
    exit(1);
}

int x11_cleanup(Display *dpy) {
    cleanup(0);
    return 0;
}

static int start_server() {
    int xfd, sd;

    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);
    if(!access(SERVER_SOCKET, F_OK)) {
        int sd = client_connect(SERVER_SOCKET);
        if(sd == -1) {
            fprintf(stderr, "Socket found, but failed to connect. Removing socket file and trying again...\n");
            if(unlink(SERVER_SOCKET) == -1) {
                perror("unlink");
                exit(1);
            }
            return start_server();
        } else {
            fprintf(stderr, "Daemon is already running.\n");
            exit(1);
        }
    }

    printf("Successfully started server.\n");
    if(!debug) {
        int fd;
        char logfile[1024];

        if(fork()) return 0;
        setsid();

        snprintf(logfile, sizeof(logfile), "%s/.clio/clio.log", getenv("HOME"));
        fd = open(logfile, O_CREAT | O_TRUNC | O_WRONLY);
        dup2(fd, 1);
        dup2(fd, 2);
        setvbuf(stdout, NULL, _IOLBF, 0);
    }

    dpy = XOpenDisplay(NULL);
    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0,0,1,1,0,0,0);
    xfd = XConnectionNumber(dpy);
    sd = server_create(SERVER_SOCKET);
    XSetIOErrorHandler(x11_cleanup);
    init_atoms();

    dbg("Entering main loop..\n");
    while(1) {
        struct timeval timeout;
        fd_set fds;
        int maxfd = sd > xfd ? sd : xfd;

        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; //Check every 100ms or so.

        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        FD_SET(sd, &fds);

        /* Poll instead of grabbing the selection and listening for acquisition
         * events (the ICCCM recommendation for implementing clipboard
         * managers). This is done in order to prevent applications from
         * producing visual artifacts when they lose the selection (e.g urxvt).
         */

        select(maxfd+1, &fds, NULL, NULL, &timeout);

        if(FD_ISSET(xfd, &fds)) {
            XEvent ev;

            while(XPending(dpy)) {
                XNextEvent(dpy, &ev);
                handle_xev(&ev);
            }
        } else if (FD_ISSET(sd, &fds)) {
            int con;
            con = accept(sd, NULL, 0);
            server_handle_connection(con);

            //Ensure any changes to the clipboard propagate.

            XSetSelectionOwner(dpy, XA_PRIMARY, win, CurrentTime);
            XSetSelectionOwner(dpy, XA_CLIPBOARD, win, CurrentTime);
            own_primary = 1;
            own_clipboard = 1;
            XFlush(dpy);
        } else {
            //Use the selection type as the target property name for convenience.

            if(!own_primary)
                XConvertSelection(dpy, XA_PRIMARY, XA_UTF8, XA_PRIMARY, win, CurrentTime); 
            if(!own_clipboard)
                XConvertSelection(dpy, XA_CLIPBOARD, XA_UTF8, XA_CLIPBOARD, win, CurrentTime);

            XFlush(dpy);
        }
    }
}


void die(char *fmt, ...) {
    va_list lst;
    va_start(lst, fmt);
    vfprintf(stderr, fmt, lst);
    va_end(lst);
    exit(1);
}

ssize_t read_stdin(char **res) {
    ssize_t sz = 0, cap = 0, n;
    char *content = NULL;
    char buf[2048];
    while((n=read(0, buf, sizeof buf)) > 0) {
        if(n + sz > cap) {
            cap = (n+sz)*1.6;
            content = realloc(content, cap);
        }
        memcpy(content + sz, buf, n);
        sz+=n;
    }

    *res = content;
    return sz;
}

int main(int argc, char *argv[]) {
    int sd, c;
    const char usage[] = "Usage: %s [-a <num>] [-n <num>] [-l] [-d] [-v]\n\n"
        "-l: Lists all pastables by number (from most to least recent).\n"
        "  Multiline pastables are truncated.\n"
        "-a <num>: Activates pastable number <num>, this moves it to the top of history\n"
        "  and allows it to be pasted.\n"
        "-p <num>: Prints pastable number <num> in its entirety.\n"
        "-c: Removes all pastables from memory.\n"
        "-d: Starts the daemon.\n"
        "-v: Prints version info.\n";

    umask(0077);
    snprintf(SERVER_SOCKET_DIR, sizeof(SERVER_SOCKET_DIR), "%s/.clio", getenv("HOME"));
    mkdir(SERVER_SOCKET_DIR, 0700);
    snprintf(SERVER_SOCKET, sizeof(SERVER_SOCKET), "%s/server.socket", SERVER_SOCKET_DIR);

    if(!isatty(0) && argc == 1) {
        char *content;
        ssize_t sz = read_stdin(&content);

        if ((sd = client_connect(SERVER_SOCKET)) == -1) 
            die("ERROR: Failed to connect to %s. (make sure daemon is running (clio -d)).\n", SERVER_SOCKET);

        client_set_clipboard(sd, content, sz);
        return 0;
    }

    if(argc > 1 && !strcmp(argv[1], "--debug")) {
        argv++;
        debug++;
    }

    while((c=getopt(argc, argv, "hdp:la:vc")) != -1) {
        int sd;

        switch(c) {
        case 'd':
            return start_server(SERVER_SOCKET);
        case 'v':
            fprintf(stderr, "Author: Aetnaeus\nVersion: "VERSION"\nGit Commit: "GIT_COMMIT"\n");
            return 0;
        case 'h':
            fprintf(stderr, usage, argv[0]);
            return 0;
        }

        if ((sd = client_connect(SERVER_SOCKET)) == -1) 
            die("ERROR: Failed to connect to %s. (make sure daemon is running (clio -d)).\n", SERVER_SOCKET);

        switch(c) {
        case 'c': client_clear_history(sd); break;
        case 'p': client_print_history(sd, atoi(optarg)); break;
        case 'a': client_activate(sd, atoi(optarg)); break;
        case 'l': client_print_history(sd, -1); break;
        }

        return 0;
    }

    fprintf(stderr, usage, argv[0]);

    return -1;
}
