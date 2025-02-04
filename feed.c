#include "util.h"
#include <signal.h>

int fd_srv, fd_cli;
char fifo_cli[50];


void encerra_programa(int sinal) {
    printf("\nA encerrar o programa...\n");


    PEDIDO p;
    RESPOSTA r;
    p.pid = getpid();
    strcpy(p.comando, "exit");
    write(fd_srv, &p, sizeof(PEDIDO));




    close(fd_srv);
    close(fd_cli);
    unlink(fifo_cli);

    exit(0);
}

int main(int argc, char *argv[]) {
    char str[TAM];

    PEDIDO p;
    RESPOSTA r;

    fd_set fds;
    int n;
    int deve_sair = 0;

    if (argc != 2) {
        printf("Uso: %s <nome_de_utilizador>\n", argv[0]);
        exit(1);
    }

    if (access(FIFO_SRV, F_OK) != 0) {
        printf("[ERRO] O servidor não está a correr.\n");
        exit(3);
    }

    printf("Iniciando cliente...\n");

    // Cria FIFO do cliente
    sprintf(fifo_cli, FIFO_CLI, getpid());
    if (mkfifo(fifo_cli, 0600) == -1) {
        perror("[ERRO] Falha ao criar FIFO do cliente");
        exit(2);
    }

    // Abre FIFO do servidor
    fd_srv = open(FIFO_SRV, O_WRONLY);
    if (fd_srv == -1) {
        perror("[ERRO] Falha ao abrir FIFO do servidor");
        unlink(fifo_cli);
        exit(3);
    }

    // Abre FIFO do cliente
    fd_cli = open(fifo_cli, O_RDWR);
    if (fd_cli == -1) {
        perror("[ERRO] Falha ao abrir FIFO do cliente");
        close(fd_srv);
        unlink(fifo_cli);
        exit(4);
    }

    signal(SIGINT, encerra_programa);

    // Envia pedido de login
    p.pid = getpid();
    strcpy(p.comando, "login");
    strcpy(p.mensagem, argv[1]); // Nome do usuário
    write(fd_srv, &p, sizeof(PEDIDO));

    // Aguarda resposta do servidor
    read(fd_cli, &r, sizeof(RESPOSTA));
    if (strcmp(r.str, "OK") == 0) {
        printf("Login bem-sucedido! Bem-vindo, %s.\n", argv[1]);
    } else if (strcmp(r.str, "Erro: Nome já está em uso.") == 0) {
        printf("[ERRO] Nome já está em uso. Por favor, escolha outro nome.\n");
        close(fd_srv);
        close(fd_cli);
        unlink(fifo_cli);
        exit(6);
    } else if (strcmp(r.str, "Usuário já conectado.") == 0) {
        printf("[ERRO] Usuário já está conectado. Por favor, desconecte antes de tentar novamente.\n");
        close(fd_srv);
        close(fd_cli);
        unlink(fifo_cli);
        exit(7);
    } else {
        printf("[ERRO] %s\n", r.str);
        close(fd_srv);
        close(fd_cli);
        unlink(fifo_cli);
        exit(5);
    }





while (!deve_sair) {

    FD_ZERO(&fds);
    FD_SET(0, &fds);       // Entrada do teclado
    FD_SET(fd_cli, &fds);  // FIFO do cliente

    printf("USER> ");
    fflush(stdout);

    n = select(fd_cli + 1, &fds, NULL, NULL, NULL);

    if (n == -1) {
        perror("[ERRO] no select");
        continue;
    }

    if (FD_ISSET(0, &fds)) {
        // Entrada do teclado
        if (fgets(str, sizeof(str), stdin) == NULL) {
            printf("[ERRO] Falha na entrada.\n");
            continue;
        }
        str[strcspn(str, "\n")] = '\0';

        char comando[50];

        if (sscanf(str, "%49s", comando) == 1) {

            if (strcmp(comando, "subscribe") == 0) {
                if (sscanf(str, "subscribe %s", p.topico) == 1) {
                    strcpy(p.comando, "subscribe");
                    write(fd_srv, &p, sizeof(PEDIDO));
                    if (read(fd_cli, &r, sizeof(RESPOSTA)) > 0) {
                        printf("\n[SERVIDOR]: %s\n", r.str);
                    }
                } else {
                    printf("[ERRO] Uso correto: subscribe <topico>\n");
                }
            } else if (strcmp(comando, "unsubscribe") == 0) {
                if (sscanf(str, "unsubscribe %s", p.topico) == 1) {
                    strcpy(p.comando, "unsubscribe");
                    write(fd_srv, &p, sizeof(PEDIDO));
                    if (read(fd_cli, &r, sizeof(RESPOSTA)) > 0) {
                        printf("\n[SERVIDOR]: %s\n", r.str);
                    }
                } else {
                    printf("[ERRO] Uso correto: unsubscribe <topico>\n");
                }
            } else if (strcmp(comando, "msg") == 0) {
                char topico[50];
                int duracao;
                char mensagem[300];

                if (sscanf(str, "msg %49s %d %[^\n]", topico, &duracao, mensagem) == 3) {
                    if (duracao < 0) {
                        printf("[ERRO] Duração deve ser um número inteiro positivo.\n");
                        continue;
                    }

                    // Prepara e envia o pedido
                    strcpy(p.comando, "msg");
                    strcpy(p.topico, topico);
                    p.duracao = duracao;
                    strcpy(p.mensagem, mensagem);

                    write(fd_srv, &p, sizeof(PEDIDO));

                } else {
                    printf("[ERRO] Uso correto: msg <topico> <duracao> <mensagem>\n");
                }
            } else if (strcmp(str, "exit") == 0) {
                strcpy(p.comando, "exit");
                write(fd_srv, &p, sizeof(PEDIDO));
                printf("Encerrando o cliente...\n");
                deve_sair = 1;
            } else if (strcmp(str, "topics") == 0) {
                strcpy(p.comando, "topics");
                write(fd_srv, &p, sizeof(PEDIDO));
            } else {
                printf("[ERRO] Comando inválido.\n");
            }
        }
    }

    if (FD_ISSET(fd_cli, &fds)) {  // Resposta do servidor
        int res = read(fd_cli, &r, sizeof(RESPOSTA));
        if (res > 0) {
            if (strcmp(r.str, "EXIT") == 0) {
                printf("\n[SERVIDOR]: Você foi removido pelo administrador. Encerrando o cliente...\n");
                deve_sair = 1;
            } else {
                printf("\n[SERVIDOR]: %s\n", r.str);
            }
        }
    }
}


    close(fd_srv);
    close(fd_cli);
    unlink(fifo_cli);
    return 0;
}


