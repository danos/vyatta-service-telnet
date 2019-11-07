/*
 * Copyright (c) 2017, AT&T Intellectual Property.  All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
/*
 * mini-inetd.c from tcputils 0.6
 * ftp://ftp.lysator.liu.se/pub/unix/tcputils/
 * bellman@lysator.liu.se.
 *
 * These programs are released into the public domain.  You may do
 * anything you like with them, including modifying them and selling
 * the binaries without source for ridiculous amounts of money without
 * saying who made them originally.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


extern	char	** environ;
extern	int	   getopt(int, char *const*, const char *);
extern	int	   optind;
extern	char	 * optarg;


char	* progname		= "mini-inetd";
int	  debug			= 0;
int	  max_connections	= 0;


static	void
usage()
{
    fprintf(stderr,
	    "Usage:  %s [-d] [-m max] port program [argv0 argv1 ...]\n",
	    progname);
    exit(1);
}


static	void
vfatal(const char *fmt, va_list args)
{
    fputs(progname, stderr);
    fputs(": ", stderr);
    vfprintf(stderr, fmt, args);
    putc('\n', stderr);
    exit(1);
}


static	void
fatal(const char *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    vfatal(fmt, args);
    va_end(args);
}


static int
tcp_listen(const char	* address,
	   const char	* port)
{
    int			  s;
    int			  rc;
    struct sockaddr_storage server;
    struct sockaddr_in  * server4 = (struct sockaddr_in *) &server;
    struct sockaddr_in6 * server6 = (struct sockaddr_in6 *) &server;
    struct protoent	* proto;
    int                   on = 1;
    struct addrinfo       hint, *res = NULL;
    unsigned short        family = AF_INET6;

    memset(&server, 0, sizeof(server));
    if (address == NULL) {
	/* Listen on IPv4 and IPv6 */
	server6->sin6_addr = in6addr_any;
	server6->sin6_family = AF_INET6;
	server6->sin6_port = htons(atoi(port));
    } else {
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_flags = AI_NUMERICHOST;

	rc = getaddrinfo(address, NULL, &hint, &res);
	if (rc)
	    return -1;

	if (res->ai_family == AF_INET) {
	    family = AF_INET;
	    inet_pton(family, address, &server4->sin_addr);
	    server4->sin_family = AF_INET;
	    server4->sin_port = htons(atoi(port));
	} else if (res->ai_family == AF_INET6) {
	    inet_pton(family, address, &server6->sin6_addr);
	    server6->sin6_family = AF_INET6;
	    server6->sin6_port = htons(atoi(port));
	}
	else
	    return -1;
    }
    freeaddrinfo(res);

    proto = getprotobyname("tcp");
    if (proto == NULL)
	return -1;
    s = socket(family, SOCK_STREAM, proto->p_proto);
    if (s < 0)
	return -1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (bind(s, (struct sockaddr *) &server, sizeof(server)) < 0) {
	close(s);
	return -1;
    }
    if (listen(s, 10) < 0) {
	close(s);
	return -1;
    }

    return s;
}


static	void
reap_children(int signo)
{
    pid_t child;
    int status;
    char buf[64 + sizeof(long) * CHAR_BIT];

    while ((child = waitpid(-1, &status, WNOHANG)) > (pid_t)0)
    {
	if (debug)
	{
	    sprintf(buf, "Child %ld died\n", (long)child);
	    write(2, buf, strlen(buf));
	}
    }
    signal(signo, &reap_children);
}



int
main(int argc, char **argv)
{
    int sd;
    pid_t child;
    int option;
    char *host;
    char *port;

    while ((option = getopt (argc, argv, "dm:")) != EOF)
    {
	switch ((char)option)
	{
	case 'd':
	    debug++;
	    break;
	case 'm':
	    max_connections = atoi(optarg);
	    break;
	default:
	    usage();
	    /* NOTREACHED */
	}
    }

    if (argc - optind < 2)
	usage();

    port = strrchr(argv[optind], ':');
    if (port != NULL) {
	*(port++) = '\0';
	host = argv[optind];
    } else {
	port = argv[optind];
	host = NULL;
    }
    if ((sd = tcp_listen(host, port)) < 0)
	fatal("Can't listen to port %s: %s", argv[optind], strerror(errno));

    /* Reap children */
#if defined(SIGCHLD)
    signal(SIGCHLD, &reap_children);
#elif defined(SIGCLD)
    signal(SIGCLD, &reap_children);
#endif

    while (1)
    {
	struct sockaddr_in them;
	int len = sizeof them;
	int ns = accept(sd, (struct sockaddr*)&them, &len);
	if (ns < 0) {
	    if (errno == EINTR)
		continue;
	    fatal("Accept failed: %s", strerror(errno));
	}
	switch (child = fork()) {
	case -1:
	    perror("Can't fork");
	    break;
	case 0:
	    /* Child */
	    close(sd);
	    dup2(ns, 0);
	    dup2(ns, 1);
	    execve(argv[optind+1], argv+optind+2, environ);
	    fatal("Can't start %s: %s", argv[optind+1], strerror(errno));
	default:
	    /* Parent */
	    if (debug)
		fprintf(stderr, "Forked child %ld\n", (long)child);
	    close(ns);
	    break;
	}
	if (max_connections > 0  &&  --max_connections <= 0)
	    break;
    }

    return 0;
}

