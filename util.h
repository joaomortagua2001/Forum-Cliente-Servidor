#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>

#define TAM 1000
#define FIFO_SRV "tubo"
#define FIFO_CLI "f_%d"
#define MAX_USERS 10
#define MAX_TOPICOS 20
#define MAX_MENSAGENS 5


typedef struct {
    char comando[TAM];    // Comando enviado pelo cliente
    char topico[20];     // Nome do tópico
    char mensagem[300];
    int duracao;
    int pid;              // PID do cliente que enviou o comando
} PEDIDO;

typedef struct {
    char texto[300];  // Texto da mensagem
    int duracao;      // Duração da mensagem
    char usuario[30];
} MENSAGEM;

typedef struct {
    char nome[20];                        // Nome do tópico
    MENSAGEM mensagens[MAX_MENSAGENS];   // Mensagens persistentes do tópico
    int contador;                         // Contador de mensagens armazenadas
    int subscritores[MAX_USERS];          // Índices dos usuários inscritos
    int n_subs;                           // Número de subscritores
    int bloqueado;
} TOPICO;

typedef struct {
    int pid;           // PID do processo do cliente
    char nome[30];     // Nome do usuário
} USER;

typedef struct {
    char str[1000];            // Maneira à "trapalhão"
} RESPOSTA;

typedef struct {
    int executa;
    USER usuarios[MAX_USERS];       // Lista de usuários conectados
    TOPICO topicos[MAX_TOPICOS];    // Lista de tópicos criados
    pthread_mutex_t mutex_usuarios; // Mutex para sincronizar acesso aos usuários
    pthread_mutex_t mutex_topicos;  // Mutex para sincronizar acesso aos tópicos
} DADOS;