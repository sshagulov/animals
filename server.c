#define _POSIX_C_SOURCE 200112L

#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>

#define MAP_SIZE 30
#define MAX_SATIETY 40
#define MAX_AGE 60
// #define SAME_ORIGINALS_NUM 0
#define SAME_ORIGINALS_NUM MAP_SIZE
#define SPEED_MAP 100000

#define MAX_CLIENTS 10
#define PORT "3490"
#define BACKLOG 10

typedef struct {
    int x, y;
    int type;
    int satiety;
    int age;
    pthread_t thread;
} Animal;

Animal* grid[MAP_SIZE][MAP_SIZE];
int terra[30][30] = {
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t c = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_listen_client = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_build_map = PTHREAD_MUTEX_INITIALIZER;

const char* animal_types[3] = {"🐱", "🐭", "🦊"};
const char* terra_types[4] = {"🌲", "🌊", "🗿", "🔥"};

const int meteor[12][2] = {
    {1, 0}, {0, 1},
    {2, 0}, {0, 2},
    {1, 1}, {2, 1},
    {1, 2}, {0, 3},
    {3, 1}, {3, 0},
    {1, 3}, {2, 2},
};

const int directions[2][4] = {{1, -1, 0, 0},{0, 0, 1, -1}};
int animals_num[3] = {0, 0, 0};

int originals_created_cnt = 0;
int children_created_cnt = 0;
int deads_cnt = 0;
int generations_cnt = 0;
int empty_cells_cnt = 0;

int** generation;
char* map_str = NULL;

int keepRunning = 1;
int wait = 1;

char clientAddr[INET_ADDRSTRLEN];
int clients[MAX_CLIENTS];
int client_cnt;
struct pollfd pfds[2];
struct sockaddr_storage theirAddr;
socklen_t addrSize;
struct addrinfo hints;
struct addrinfo* res;
int servSockFd;

void build_map();
void seed_map();
void print_generation();
void create_animal(int type);
void create_child(int type, int x, int y, int satiety);
void kill_animal(Animal* animal);
void* animal_thread(void* arg);

void build_map() {
    pthread_mutex_lock(&mtx_build_map);
    
    free(map_str);
    asprintf(&map_str, "%s", "");
    empty_cells_cnt = 0;

    asprintf(&map_str, "%s\033[2J\033[H%d итерация:\n", map_str, generations_cnt);
    
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            if (grid[i][j] == NULL) {
                empty_cells_cnt++;
                asprintf(&map_str, "%s%s ", map_str, terra_types[terra[i][j]]);
            } else
                asprintf(&map_str, "%s%s ", map_str, animal_types[grid[i][j]->type]);
        }
        asprintf(&map_str, "%s%s", map_str, "\n");
    }
    asprintf(
        &map_str, "%s\nВсего животных: %d 🐱, %d 🐭, %d 🦊, %d 🌲"
        "\nПервородных: %d, Потомков: %d, Умерших: %d\n",
        map_str, animals_num[0], animals_num[1], animals_num[2], empty_cells_cnt,
        originals_created_cnt, children_created_cnt, deads_cnt
    );

    asprintf(&map_str, "%s\033[3mНажмите ENTER, чтобы ввести команду\033[0m\n", map_str);

    //printf("%s\n", map_str);
    pthread_mutex_unlock(&mtx_build_map);
    for (int i = 0; i < 3; i++) {
        int* temp = (int*)realloc(generation[i], sizeof(int) * (generations_cnt + 1));
        if (temp == NULL) return;
        generation[i] = temp;
        generation[i][generations_cnt] = animals_num[i];
    }
    generations_cnt++;
}

void print_generation() {
    printf("\n\nИстория популяций:\n");
    for (int i = 0; i < generations_cnt; i++) {
        printf(
            "%*d поколение: %*d 🐱, %*d 🐭, %*d 🦁\n",
            2, i, 3, generation[0][i], 3, generation[1][i], 3, generation[2][i]
        );
    }
    printf("\n");
}

void seed_map(int type, int num) {
    for (int i = 0; i < num; i++)
        create_animal(type);
}

void create_animal(int type) {

    int x, y;

    while(1) {
        x = rand() % MAP_SIZE;
        y = rand() % MAP_SIZE;
        if (grid[x][y] == NULL) break;
    }
    Animal* animal = (Animal*)malloc(sizeof(Animal));
    animal->type = type;
    animal->age = 0;
    animal->satiety = MAX_SATIETY;
    animal->x = x;
    animal->y = y;
    pthread_mutex_lock(&a);
    animals_num[type]++;
    originals_created_cnt++;
    grid[animal->x][animal->y] = animal;
    pthread_mutex_unlock(&a);
    
    pthread_create(&animal->thread, NULL, animal_thread, animal);
}

void create_child(int type, int x, int y, int satiety) {
    for (int i = 0; i < 4; i++) {

        int new_x = ((x + directions[0][i]) + MAP_SIZE) % MAP_SIZE;
        int new_y = ((y + directions[1][i]) + MAP_SIZE) % MAP_SIZE;

        if (grid[new_x][new_y] == NULL) {
            Animal* animal = (Animal*)malloc(sizeof(Animal));
            animal->type = type;
            animal->age = 0;
            animal->satiety = satiety;
            animal->x = new_x;
            animal->y = new_y;

            pthread_mutex_lock(&a);
            grid[animal->x][animal->y] = animal;
            animals_num[type]++;
            children_created_cnt++;
            pthread_mutex_unlock(&a);

            pthread_create(&animal->thread, NULL, animal_thread, animal);
            break;
        }
    }
}

void kill_animal(Animal* animal) {
    pthread_mutex_lock(&a);
    deads_cnt++;
    animals_num[animal->type]--;
    grid[animal->x][animal->y] = NULL;
    pthread_mutex_unlock(&a);
    free(animal);
    pthread_exit(NULL);
}

void* animal_thread (void* arg) {
    pthread_detach(pthread_self());
    
    Animal* animal = (Animal*)arg;
    while(wait){}

    while (1) {

        usleep(SPEED_MAP);
    
        if (animal->satiety == 0 || animal->age == MAX_AGE) {
            kill_animal(animal);
            break;
        }
        else {
            animal->satiety--;
            animal->age++;
        }

        int k = rand() % 4;
        int new_x = ((animal->x + directions[0][k]) + MAP_SIZE) % MAP_SIZE;
        int new_y = ((animal->y + directions[1][k]) + MAP_SIZE) % MAP_SIZE;

        pthread_mutex_lock(&c);
        // создание ребёнка
        if (grid[new_x][new_y] != NULL
            && grid[new_x][new_y]->type == animal->type
        ) {
            create_child(animal->type, new_x, new_y, animal->satiety);
            animal->satiety = (animal->satiety + 1) / 3 * 2;
            pthread_mutex_unlock(&c);
            continue;
        }

        // самоубийство
        if (grid[new_x][new_y] != NULL
            && (grid[new_x][new_y]->type + 1) % 3 == animal->type
        ) {
            grid[new_x][new_y]->satiety = MAX_SATIETY;
            pthread_mutex_unlock(&c);
            kill_animal(animal);
            break;
        }
        
        // убийство
        if (grid[new_x][new_y] != NULL
            && (grid[new_x][new_y]->type + 2) % 3 == animal->type
        ) {
            grid[animal->x][animal->y] = NULL;
            animal->x = new_x;
            animal->y = new_y;
            animal->satiety = MAX_SATIETY;
            grid[new_x][new_y]->age = MAX_AGE;
            grid[new_x][new_y] = animal;
            pthread_mutex_unlock(&c);
            continue;
        }
        
        grid[animal->x][animal->y] = NULL;
        animal->x = new_x;
        animal->y = new_y;
        grid[new_x][new_y] = animal;
        pthread_mutex_unlock(&c);
    }
}

void intHandler(int) {
    keepRunning = 0;
}

void* get_in_addr(struct sockaddr* sa) {
    return sa->sa_family == AF_INET
        ? (void*)&(((struct sockaddr_in*)sa)->sin_addr)
        : (void*)&(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* listen_client(void* arg) {

    int client_fd = *((int*)arg);

    while (1) {
        char str[100];
        memset(str, 0, 100);

        int resp = recv(client_fd, str, 100, 0);
        if (resp != -1) {

            printf("\nПолучил \"%s\"\n", str);

            if (strncmp(str, "close", 5) == 0) {
                pthread_mutex_lock(&mtx_listen_client);
                for (int i = 0; i < client_cnt; i++){
                    if (client_fd == clients[i]) {
                        printf("client %d left\n", i);
                        for(int j = i + 1; j < client_cnt; j++)
                            clients[i++] = clients[j];
                        break;
                    }
                }
                client_cnt--;
                pthread_mutex_unlock(&mtx_listen_client);
                close(client_fd);
                pthread_exit(NULL);
            }

            if (strncmp(str, "spawn ", 6) == 0) {
                int type = atoi(&str[6]);
                int num = atoi(&str[8]);
                if (empty_cells_cnt > num){
                    printf("Создалось %d животных %d-го типа\n", num, type);
                    pthread_mutex_lock(&mtx_listen_client);
                    seed_map(type, num);
                    pthread_mutex_unlock(&mtx_listen_client);
                }
            }

            if (strncmp(str, "spawnall", 8) == 0) {
                int num = atoi(&str[9]);
                if (empty_cells_cnt > 3 * num) {
                    printf("Создалось %d животных всех типов\n", num);
                    pthread_mutex_lock(&mtx_listen_client);
                    seed_map(0, num);
                    seed_map(1, num);
                    seed_map(2, num);
                    pthread_mutex_unlock(&mtx_listen_client);
                }
            }

            if (strncmp(str, "meteor", 6) == 0) {
                int x = rand() % (MAP_SIZE - 8) + 4;
                int y = rand() % (MAP_SIZE - 8) + 4;
                printf("Упал метеор x: %d y: %d\n", x, y);
                
                int sign1 = 1, sign2 = 1;
                terra[x][y] = 2;
                for (int x_move = 0; x_move < 2; x_move++) {
                    for (int y_move = 0; y_move < 2; y_move++) {
                        for (int i = 0; i < 12; i++) {
                            int new_x = x + meteor[i][0] * sign1;
                            int new_y = y + meteor[i][1] * sign2;

                            terra[new_x][new_y] = (i >= 7) ? 3 : 2;

                            if (grid[new_x][new_y] != NULL) {
                                grid[new_x][new_y]->age = MAX_AGE;
                            }
                        }
                        sign1 = -1;
                    }
                    sign1 = 1;
                    sign2 = -1;
                }
            }
            
        }

    }
}

void* add_clients(void*) {
    printf("Жду подключения...\n");
    while (1) {
        int result = poll(pfds, 2, -1);
        if (result == -1) {
            perror("Ошибка опроса");
            break;
        }

        // если получено событие и это не консольный ввод,
        // то принимаем входящее соединение
        addrSize = sizeof(theirAddr);
        clients[client_cnt] = accept(
            servSockFd,
            (struct sockaddr*)&theirAddr,
            &addrSize
        );

        // преобразуем адрес в текстовый формат
        inet_ntop(
            theirAddr.ss_family,
            get_in_addr((struct sockaddr*)&theirAddr),
            clientAddr,
            INET_ADDRSTRLEN
        );
        printf("%s подключился\n", clientAddr);

        int str_len = strlen(map_str) + 100;
        send(clients[client_cnt], &str_len, 4, 0);
        printf("Отправил размер строки для вывода: %d\n", str_len);

        pthread_t thread;
        pthread_create(&thread, NULL, listen_client, (void*)&clients[client_cnt++]);
    }
}

void send_map() {
    pthread_mutex_lock(&mtx_build_map);
    for (int i = 0; i < client_cnt; i++) {
        send(clients[i], map_str, strlen(map_str) + 100, 0);
    }
    pthread_mutex_unlock(&mtx_build_map);
}

void cancel_clients() {
    char data[6] = "cancel";
    for (int i = 0; i < client_cnt; i++) {
       send(clients[i], &data, 6, 0);
    }
}

int main() {

    setbuf(stdout, NULL);
    srand(clock());
    signal(SIGINT, intHandler);

    generation = (int**)malloc(sizeof(int*) * 3);
    for (int i = 0; i < 3; i++) {
        generation[i] = (int*)malloc(sizeof(int));
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);

    servSockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int yes = 1;

    // предотвращаем ошибку при повторной привязке сокета к локальному адресу
    if (setsockopt(servSockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }

    int result = bind(servSockFd, res->ai_addr, res->ai_addrlen);
    if (result == -1) {
        perror("Failed to bind");
        return 1;
    }

    listen(servSockFd, BACKLOG);

    pfds[0].fd = 0;
    pfds[0].events = POLLIN;
    pfds[1].fd = servSockFd;
    pfds[1].events = POLLIN;

    seed_map(0, SAME_ORIGINALS_NUM);
    seed_map(1, SAME_ORIGINALS_NUM);
    seed_map(2, SAME_ORIGINALS_NUM);
    wait = 0;

    pthread_t thread;
    pthread_create(&thread, NULL, add_clients, NULL);

    while (keepRunning) {
        build_map();
        send_map();
        usleep(SPEED_MAP);
    }

    cancel_clients();

    pthread_mutex_destroy(&a);
    pthread_mutex_destroy(&b);
    pthread_mutex_destroy(&c);
    pthread_mutex_destroy(&mtx_listen_client);
    pthread_mutex_destroy(&mtx_build_map);
    
    print_generation();

    for (int i = 0; i < 3; i++)
        free(generation[i]);
    free(generation);

    return 0;
}