/**
 * Copyright (c) 2010 Tom Burdick <thomas.burdick@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <octonaut/server.h>

#include <check.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef struct mock_server
{
    octo_server server;
    int errors;
    int connects;
} mock_server;

void mock_server_connect(octo_server *server, int fd, struct sockaddr_storage *addr, socklen_t len)
{
    octo_logger_debug(server->logger, "connect called");
    mock_server *mserver = ptr_offset(server, mock_server, server);
    mserver->connects += 1;
    close(fd);
}

void mock_server_error(octo_server *server)
{
    octo_logger_debug(server->logger, "error called");
    mock_server *mserver = ptr_offset(server, mock_server, server);
    mserver->connects += 1;
    mserver->errors += 1;
}

START_TEST (test_octo_server_init)
{
    mock_server server;

    struct ev_loop *loop = EV_DEFAULT;

    octo_server_init(&server.server, loop, 1,
            mock_server_connect, mock_server_error); 
    server.errors = 0;
    server.connects = 0;
    
    octo_server_destroy(&server.server);
}
END_TEST

START_TEST (test_octo_server_serve)
{
    int result = 0;
    int sockfd = -1;
    mock_server server;

    struct ev_loop *loop = EV_DEFAULT;
    octo_server_init(&server.server, loop, 1,
            mock_server_connect, mock_server_error); 
    server.errors = 0;
    server.connects = 0;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, "7357", &hints, &res);
    fail_unless(result >= 0,
        gai_strerror(result));

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    fail_unless(sockfd >= 0,
        strerror(errno));

    result = bind(sockfd, res->ai_addr, res->ai_addrlen);

    fail_unless(result >= 0,
        strerror(errno));

    result = octo_server_serve(&server.server, sockfd);

    fail_unless(result,
        "server failed to return true when it should have");

    fail_unless(octo_server_isactive(&server.server),
        "server not active even though it should be");

    int csockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    int flags = fcntl(csockfd, F_GETFL, 0);
    result = fcntl(csockfd, F_SETFL, flags | O_NONBLOCK);

    fail_unless(result >= 0,
        "fcntl failed to set the socket to be non-blocking");

    result = connect(csockfd, res->ai_addr, res->ai_addrlen);

    fail_unless(result != EINPROGRESS || result >= 0,
        strerror(errno));

    ev_run(loop, EVRUN_ONCE);

    fail_unless(server.connects == 1,
        "server failed to call connect callback");

    close(csockfd);

    octo_server_destroy(&server.server);

    fail_unless(!octo_server_isactive(&server.server),
        "server active even though it should not be");

    freeaddrinfo(res);
    close(sockfd);
}
END_TEST

START_TEST (test_octo_server_serve_error)
{
    int result = 0;
    int sockfd = -1;
    mock_server server;

    struct ev_loop *loop = EV_DEFAULT;

    octo_server_init(&server.server, loop, 1,
            mock_server_connect, mock_server_error);
    server.errors = 0;
    server.connects = 0;

    result = octo_server_serve(&server.server, sockfd);

    fail_unless(!result,
        "server serve returned true when it should not have");

    fail_unless(!octo_server_isactive(&server.server),
        "server active even though it should not be");

    octo_server_destroy(&server.server);

    fail_unless(!octo_server_isactive(&server.server),
        "server active even though it should not be");
}
END_TEST

START_TEST (test_octo_server_connect_error)
{
    int result = 0;
    int sockfd = -1;
    mock_server server;

    struct ev_loop *loop = EV_DEFAULT;
    octo_server_init(&server.server, loop, 1,
            mock_server_connect, mock_server_error); 
    server.errors = 0;
    server.connects = 0;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, "7358", &hints, &res);
    fail_unless(result >= 0,
        gai_strerror(result));

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    fail_unless(sockfd >= 0,
        strerror(errno));

    result = bind(sockfd, res->ai_addr, res->ai_addrlen);

    fail_unless(result >= 0,
        strerror(errno));

    octo_server_serve(&server.server, sockfd);

    fail_unless(octo_server_isactive(&server.server),
        "server not active even though it should be");

    /* set a bogus fd here to force an error to occur on accept() */
    server.server.fd = -1;

    int csockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    int flags = fcntl(csockfd, F_GETFL, 0);
    result = fcntl(csockfd, F_SETFL, flags | O_NONBLOCK);

    fail_unless(result >= 0,
        "fcntl failed to set the socket to be non-blocking");

    result = connect(csockfd, res->ai_addr, res->ai_addrlen);

    fail_unless(result != EINPROGRESS || result >= 0,
        strerror(errno));

    ev_run(loop, EVRUN_ONCE);

    fail_unless(server.errors == 1,
        "server failed to call error callback");

    close(csockfd);

    octo_server_destroy(&server.server);

    fail_unless(!octo_server_isactive(&server.server),
        "server active even though it should not be");

    freeaddrinfo(res);
    close(sockfd);
}
END_TEST


TCase* octo_server_tcase()
{
    TCase* tc_octo_server = tcase_create("octo_server");
    tcase_add_test(tc_octo_server, test_octo_server_init);
    tcase_add_test(tc_octo_server, test_octo_server_serve);
    tcase_add_test(tc_octo_server, test_octo_server_serve_error);
    tcase_add_test(tc_octo_server, test_octo_server_connect_error);
    return tc_octo_server;
}


