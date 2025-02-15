#include "statistics.h"
#include "handler.h"
#include "util.h"
#include "cli.h"
#include "config.h"
#include "third/nfqueue-mnl.h"

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

volatile int should_exit = false;

void signal_handler(int signum) {
    syslog(LOG_ERR, "Signal %s received, exiting...", strsignal(signum));
    should_exit = true;
}

int main(int argc, char *argv[]) {
    openlog("UA2F", LOG_PID, LOG_SYSLOG);

    load_config();

    try_print_info(argc, argv);

    init_statistics();
    init_handler();

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);

    struct nf_queue queue[1];
    memset(queue, 0, sizeof(struct nf_queue));
    struct nf_buffer buf[1];
    memset(buf, 0, sizeof(struct nf_buffer));

    __auto_type ret = nfqueue_open(queue, QUEUE_NUM, 0);
    if (!ret) {
        syslog(LOG_ERR, "Failed to open nfqueue");
        return EXIT_FAILURE;
    }

    while (!should_exit) {
        if (nfqueue_receive(queue, buf, 0) == IO_READY) {
            struct nf_packet packet[1];
            while (nfqueue_next(buf, packet) == IO_READY) {
                handle_packet(queue, packet);
            }
        }
    }

    free(buf->data);
    nfqueue_close(queue);
}

#pragma clang diagnostic pop