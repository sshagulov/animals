#define _POSIX_C_SOURCE 200112L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ADDR "127.0.0.1"
#define PORT "3490"

int keepRunning = 1;

void int_handler(int) {
    keepRunning = 0;
}

void* get_in_addr(struct sockaddr* sa) {
    return sa->sa_family == AF_INET
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {

    signal(SIGINT, int_handler);

    int sockFd;
    struct addrinfo hints;
    struct addrinfo* servinfo;
    struct addrinfo* p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int result = getaddrinfo(argc > 1 ? argv[1] : ADDR, PORT, &hints, &servinfo);
    if (result != 0) {
        printf(
            "Не удалось получить информацию об адресе: %s\n",
            gai_strerror(result)
        );
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockFd == -1) {
            perror("Неудачная попытка создать сокет");
            continue;
        }
        result = connect(sockFd, p->ai_addr, p->ai_addrlen);
        if (result == -1) {
            close(sockFd);
            perror("Неудачная попытка подключения");
            continue;
        }
        break;
    }
    if (p == NULL) return 2;

    char s[INET6_ADDRSTRLEN];
    inet_ntop(
        p->ai_family,
        get_in_addr((struct sockaddr*)p->ai_addr),
        s, sizeof(s)
    );
    printf("Соединился с %s\n", s);

    freeaddrinfo(servinfo);

    int tableSize;
    result = recv(sockFd, &tableSize, 4, 0);
    if (result == -1)
        perror("Не удалось получить размер таблицы");

    printf("Получил размер таблицы: %d\n", tableSize);

    char* table = malloc(tableSize);

    struct pollfd pfds[2];
    pfds[0].fd = 0;
    pfds[0].events = POLLIN;
    pfds[1].fd = sockFd;
    pfds[1].events = POLLIN;
    
    while (keepRunning) {

        poll(pfds, 2, -1);

        if (pfds[0].revents & POLLIN) {
            printf(
                "\033[1mДоступные команды:\033[0m\n"
                "• spawn <тип животного> <количество животных>\n"
                "• spawnall <количество животных>\n"
                "• meteor\n> "
            );
            char str[100];
            getchar();
            if (fgets(str, sizeof(str), stdin) != NULL) {
                str[strlen(str) - 1] = '\0'; // убираем \n с конца
                send(sockFd, &str, 100, 0);
            }
            else {
                fprintf(stderr, "Ошибка при считывании строки\n");
            }
        }

        int ret;
        if (ret = recv(sockFd, table, tableSize + 1, 0)) {
            printf("%s", table);
        }
    }

    char str[5] = "close";
    if (keepRunning == 0) {
        printf("Соединение было закрыто\n");
        send(sockFd, &str, 5, 0);
    }

    close(sockFd);
    return 0;
}