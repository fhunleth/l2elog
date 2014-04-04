#include <err.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SYSLOG_PATH       "/dev/log"
#define MAX_LOG_MESSAGE   256

struct erlang_msg {
    uint16_t length_be;
    uint8_t priority;
    char message[MAX_LOG_MESSAGE];
};

static char buffer[MAX_LOG_MESSAGE * 16];
static struct erlang_msg notification;

static int open_syslog_fd()
{
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
	err(EXIT_FAILURE, "socket");

    // Erase the old log file (if any) so that we
    // can bind to it
    unlink(SYSLOG_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SYSLOG_PATH);

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
	err(EXIT_FAILURE, "bind");

    // Make sure that users can write to the log or we won't
    // get much.
    chmod(SYSLOG_PATH, 0666);

    return fd;
}

static int open_kmsg_fd()
{
    int fd = open("/proc/kmsg", O_RDONLY);
    if (fd < 0)
	err(EXIT_FAILURE, "open /proc/kmsg");

    return fd;
}

static void parse_msgs(const char *buf, int len)
{
    const char *end = buf + len;

    while (buf != end) {
        while (*buf != '<') {
           buf++;
           if (buf == end)
               return;
        }

        buf++;
        if (buf == end)
            return;

        const char *prio = buf;
        while (*buf != '>') {
            buf++;
            if (buf == end)
                return;
        }
        int priority = strtol(prio, 0, 10);

        buf++;
        const char *msg = buf;
        while (buf != end && *buf != '\0' && *buf != '\n')
            buf++;

	int msglen = (int) (buf - msg);
	if (msglen > MAX_LOG_MESSAGE)
	    msglen = MAX_LOG_MESSAGE;

	notification.length_be = htons(msglen + 1);
	notification.priority = (uint8_t) priority;
	memcpy(notification.message, msg, msglen);
	write(STDOUT_FILENO, &notification, msglen + 3);
    }
}

int main(int argc, char *argv[])
{
    int syslogfd = open_syslog_fd();
    int kmsgfd = open_kmsg_fd();
    struct pollfd fds[2];
    fds[0].fd = syslogfd;
    fds[0].events = POLLIN;
    fds[1].fd = kmsgfd;
    fds[1].events = POLLIN;

    for (;;) {
	if (poll(fds, 2, -1) < 0)
	    err(EXIT_FAILURE, "poll");

	if (fds[0].revents) {
	    ssize_t amt = read(syslogfd, buffer, sizeof(buffer));
	    if (amt < 0)
		err(EXIT_FAILURE, "read syslog");
            parse_msgs(buffer, amt);
	}
	if (fds[1].revents) {
	    ssize_t amt = read(kmsgfd, buffer, sizeof(buffer));
	    if (amt < 0)
		err(EXIT_FAILURE, "read kmsg");
            parse_msgs(buffer, amt);
	}
    }
}
