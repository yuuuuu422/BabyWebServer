//
// Created by theoyu on 5/1/22.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "signal.h"
#include "assert.h"
#include "threadpool/threadpool.h"
#include "lock/locker.h"
#include "http/http_conn.h"

#define MAX_FD 65536           //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数

extern int addfd(int epollfd, int fd, bool one_shot);
extern int setnonblocking(int fd);

void addsig(int sig, void(handler)(int)){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//给客户端返回错误信息
void show_error(int connfd, const char *info){
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}
int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("please enter your port_number~\n");
        exit(-1);
    }
    int port = atoi(argv[1]);
    //对SIGPIPE信号进行捕捉
    addsig(SIGPIPE, SIG_IGN);
    //创建线程池
    threadpool<http_conn> *pool = NULL;
    try {
        pool = new threadpool<http_conn>;
    } catch (...) {
        exit(-1);
    }

    http_conn *users = new http_conn[MAX_FD];

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    //端口复用
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd, (struct sockaddr *) &address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenfd, 5);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(10);
    assert(epollfd != -1);

    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;
    while (true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);

                if (connfd < 0) {
                    printf("errno : %d\n", errno);
                    continue;
                }
                //连接数已满
                if (http_conn::m_user_count >= MAX_FD) {
                    show_error(connfd, "Internal server busy");
                    close(connfd);
                    continue;
                }
                users[connfd].init(connfd, client_address);
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {

                users[sockfd].close_conn();

            } else if (events[i].events & EPOLLIN) {

                if (users[sockfd].read()) {
                    pool->append(users + sockfd); //添加 第sockfd个user
                } else {
                    users[sockfd].close_conn();
                }

            } else if (events[i].events & EPOLLOUT) {

                if (!users[sockfd].write()) {
                    users[sockfd].close_conn();
                }

            }
        }
    }

}

