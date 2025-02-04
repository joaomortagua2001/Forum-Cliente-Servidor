#include "util.h"




int encontrar_topico(DADOS* dados, const char* nome_topico) {
    for (int i = 0; i < MAX_TOPICOS; i++) {
        if (strcmp(dados->topicos[i].nome, nome_topico) == 0) {
            return i;
        }
    }
    return -1; // Tópico não encontrado
}


int criar_topico(DADOS* dados, const char* nome_topico) {
    for (int i = 0; i < MAX_TOPICOS; i++) {
        if (dados->topicos[i].nome[0] == '\0') {
            strcpy(dados->topicos[i].nome, nome_topico);
            dados->topicos[i].contador = 0;
            dados->topicos[i].n_subs = 0;
            dados->topicos[i].bloqueado = 0;
            printf("Tópico '%s' criado com sucesso.\n", nome_topico);
            return i;
        }
    }
    return -1; // Limite de tópicos atingido
}



void apagar_topico(DADOS* dados, int indice_topico) {
    TOPICO* topico = &dados->topicos[indice_topico];


    if (topico->n_subs == 0) {

        int ha_mensagens = 0;
        for (int i = 0; i < MAX_MENSAGENS; i++) {
            if (topico->mensagens[i].texto[0] != '\0' && topico->mensagens[i].duracao > 0) {
                ha_mensagens = 1;
                break;
            }
        }


        if (!ha_mensagens) {
            printf("O tópico '%s' foi apagado por não ter subscritores nem mensagens persistentes.\n", topico->nome);
            memset(topico, 0, sizeof(TOPICO)); // Limpa a estrutura
        } else {
            printf(" Tópico '%s' não foi apagado porque ainda possui mensagens persistentes.\n", topico->nome);
        }
    }
}



int adicionar_usuario(DADOS* dados, int pid, const char* nome) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(dados->usuarios[i].nome, nome) == 0) {
            printf("[ERRO] Nome já existe: %s\n", nome);
            return -9; // Nome já existe
        }
    }

    for (int i = 0; i < MAX_USERS; i++) {
        if (dados->usuarios[i].pid == 0) {
            dados->usuarios[i].pid = pid;
            strcpy(dados->usuarios[i].nome, nome);
            printf("Utilizador adicionado: %s\n", nome);
            return i; // Índice do usuário
        }
    }
    printf("Lista cheia. Não é possível adicionar o utilizador: %s\n", nome);
    return -1; // Lista cheia
}



int encontrar_usuario_por_nome(DADOS* dados, const char* nome) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (dados->usuarios[i].pid != 0 && strcmp(dados->usuarios[i].nome, nome) == 0) {
            return i;
        }
    }
    return -1; // Usuário não encontrado
}


int encontrar_usuario_por_pid(DADOS* dados, int pid) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (dados->usuarios[i].pid == pid) {
            return i;
        }
    }
    return -1; // Usuário não encontrado
}


int remover_subscritor(DADOS* dados, int indice_topico, int indice_usuario) {
    TOPICO* topico = &dados->topicos[indice_topico];
    for (int i = 0; i < topico->n_subs; i++) {
        if (topico->subscritores[i] == indice_usuario) {

            topico->subscritores[i] = topico->subscritores[--topico->n_subs];
            printf("Utilizador removido do tópico '%s'.\n", topico->nome);
            apagar_topico(dados, indice_topico); // Verifica se o tópico ficou vazio
            return 1; // Sucesso
        }
    }
    return 0; // Usuário não encontrado
}


void remover_usuario_de_topicos(DADOS* dados, int indice_usuario) {
    for (int i = 0; i < MAX_TOPICOS; i++) {
        if (dados->topicos[i].nome[0] != '\0') { // Tópico existe
            remover_subscritor(dados, i, indice_usuario);
        }
    }
}


void remover_usuario(DADOS* dados, const char* nome) {
    int indice_usuario = encontrar_usuario_por_nome(dados, nome);
    if (indice_usuario == -1) {
        printf("[ERRO] Utilizador '%s' não encontrado.\n", nome);
        return;
    }

    USER* usuario = &dados->usuarios[indice_usuario];
    printf("A remover o utilizador '%s'...\n", usuario->nome);

    // Remove o usuário de todos os tópicos
    remover_usuario_de_topicos(dados, indice_usuario);

    // Notifica todos os outros usuários
    RESPOSTA r;
    sprintf(r.str, "Utilizador '%s' foi removido pelo administrador.", nome);
    for (int i = 0; i < MAX_USERS; i++) {
        if (dados->usuarios[i].pid != 0 && i != indice_usuario) {
            char fifo_cli[20];
            sprintf(fifo_cli, FIFO_CLI, dados->usuarios[i].pid);
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
        }
    }


    // Envia uma mensagem ao cliente para que ele encerre
    sprintf(r.str, "EXIT");
    char fifo_cli[20];
    sprintf(fifo_cli, FIFO_CLI, usuario->pid);
    int fd_cli = open(fifo_cli, O_WRONLY);
    if (fd_cli != -1) {
        write(fd_cli, &r, sizeof(RESPOSTA));
        close(fd_cli);
    } else {
        perror("[ERRO] Não foi possível enviar a mensagem de encerramento ao cliente");
    }

    // Remove o usuário da lista global
    memset(usuario, 0, sizeof(USER));
    printf("Utilizador '%s' removido com sucesso.\n", nome);
}
//Quando é o utilizador que faz o comando exit
void remover_usuario_por_pid(DADOS* dados, int pid) {
    int indice_usuario = encontrar_usuario_por_pid(dados, pid); // Localiza o índice do usuário pelo PID
    if (indice_usuario == -1) {
        printf("[ERRO] Utilizador com PID %d não encontrado.\n", pid);
        return;
    }

    USER* usuario = &dados->usuarios[indice_usuario];
    printf("O utilizador %s terminou sessao...\n", usuario->nome);


    remover_usuario_de_topicos(dados, indice_usuario);

    memset(usuario, 0, sizeof(USER));

}




void listar_usuarios(DADOS* dados) {
    printf("Usuários conectados:\n");
    for (int i = 0; i < MAX_USERS; i++) {
        if (dados->usuarios[i].pid != 0) {
            printf("USER[%d]: PID = %d, Nome = %s\n", i, dados->usuarios[i].pid, dados->usuarios[i].nome);
        }
    }
}


void listar_topicos(DADOS* dados) {
    printf("Tópicos criados:\n");
    for (int i = 0; i < MAX_TOPICOS; i++) {
        if (dados->topicos[i].nome[0] != '\0') {
            printf("TOPICO[%d]: Nome = %s, Subscritores = %d, Mensagens = %d\n",
                   i, dados->topicos[i].nome, dados->topicos[i].n_subs, dados->topicos[i].contador);
        }
    }
}


int inscrever_usuario(DADOS* dados, int indice_topico, int indice_usuario) {
    TOPICO* topico = &dados->topicos[indice_topico];
    if (topico->n_subs < MAX_USERS) {
        for (int i = 0; i < topico->n_subs; i++) {
            if (topico->subscritores[i] == indice_usuario) {
                printf("Usuário já inscrito no tópico '%s'.\n", topico->nome);
                return 0; // Usuário já está inscrito
            }
        }
        topico->subscritores[topico->n_subs++] = indice_usuario; // Adiciona o usuário
        printf("Usuário inscrito no tópico '%s'.\n", topico->nome);
        return 1; // Sucesso
    }
    return 0; // Falha (limite de inscritos atingido)
}


void enviar_para_subscritores(DADOS* dados, int indice_topico, MENSAGEM* mensagem) {
    TOPICO* topico = &dados->topicos[indice_topico];
    RESPOSTA r;


    snprintf(r.str, sizeof(r.str), "[%s] [%s] %s", topico->nome, mensagem->usuario, mensagem->texto);

    for (int i = 0; i < topico->n_subs; i++) {
        int indice_usuario = topico->subscritores[i];
        USER* usuario = &dados->usuarios[indice_usuario];

        if (usuario->pid != 0) {
            char fifo_cli[20];
            sprintf(fifo_cli, FIFO_CLI, usuario->pid);
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
        }
    }
}



void mostrar_topico(DADOS* dados, const char* nome_topico) {
    int indice_topico = encontrar_topico(dados, nome_topico);
    if (indice_topico == -1) {
        printf("[ERRO] Tópico '%s' não encontrado.\n", nome_topico);
        return;
    }

    TOPICO* topico = &dados->topicos[indice_topico];
    printf("Informações do Tópico '%s':\n", topico->nome);
    printf("Subscritores: %d\n", topico->n_subs);
    printf("Mensagens Persistentes Armazenadas:\n");

    int mensagens_exibidas = 0;
    for (int i = 0; i < MAX_MENSAGENS; i++) {
        MENSAGEM* mensagem = &topico->mensagens[i];
        if (mensagem->texto[0] != '\0') {
            printf("\n" );
            printf("  Mensagem: [%s] [%s] %s\n", topico->nome, mensagem->usuario, mensagem->texto);
            printf("  Duração restante: %d segundos\n", mensagem->duracao);
            mensagens_exibidas++;
        }
    }

    if (mensagens_exibidas == 0) {
        printf("Nenhuma mensagem persistente armazenada neste tópico.\n");
    }
}




void processar_mensagem(DADOS* dados, PEDIDO* p, int indice_usuario) {
    int indice_topico = encontrar_topico(dados, p->topico);


    if (indice_topico == -1) {
        indice_topico = criar_topico(dados, p->topico);
        if (indice_topico == -1) {
            printf("[ERRO] Limite de tópicos atingido.\n");


            RESPOSTA r;
            snprintf(r.str, sizeof(r.str), "[ERRO] Não foi possível criar o tópico '%s'.", p->topico);
            char fifo_cli[20];
            snprintf(fifo_cli, sizeof(fifo_cli), FIFO_CLI, dados->usuarios[indice_usuario].pid);
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
            return;
        }


        inscrever_usuario(dados, indice_topico, indice_usuario);
    } else {

        TOPICO* topico = &dados->topicos[indice_topico];
        int esta_inscrito = 0;
        for (int i = 0; i < topico->n_subs; i++) {
            if (topico->subscritores[i] == indice_usuario) {
                esta_inscrito = 1;
                break;
            }
        }


        if (!esta_inscrito) {
            printf("[INFO] O Usuário '%s' tentou enviar mensagem para o tópico '%s' sem estar inscrito.\n",
                   dados->usuarios[indice_usuario].nome, topico->nome);


            RESPOSTA r;
            snprintf(r.str, sizeof(r.str), "[ERRO] Você não está inscrito no tópico '%s'.", topico->nome);
            char fifo_cli[20];
            snprintf(fifo_cli, sizeof(fifo_cli), FIFO_CLI, dados->usuarios[indice_usuario].pid);
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
            return;
        }


        if (topico->bloqueado) {
            printf("[INFO] O Usuário '%s' tentou enviar mensagem para o tópico bloqueado '%s'.\n",
                   dados->usuarios[indice_usuario].nome, topico->nome);


            RESPOSTA r;
            snprintf(r.str, sizeof(r.str), "[ERRO] O tópico '%s' está bloqueado e não aceita mensagens.", topico->nome);
            char fifo_cli[20];
            snprintf(fifo_cli, sizeof(fifo_cli), FIFO_CLI, dados->usuarios[indice_usuario].pid);
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
            return;
        }
    }


    TOPICO* topico = &dados->topicos[indice_topico];
    MENSAGEM nova_mensagem;
    strcpy(nova_mensagem.texto, p->mensagem);
    nova_mensagem.duracao = p->duracao;
    strcpy(nova_mensagem.usuario, dados->usuarios[indice_usuario].nome);


    if (p->duracao != 0) {

        if (topico->contador >= MAX_MENSAGENS) {
            printf("[ERRO] Limite de mensagens persistentes atingido no tópico '%s'.\n", p->topico);


            RESPOSTA r;
            snprintf(r.str, sizeof(r.str), "[ERRO] O limite de mensagens do tópico '%s' foi atingido. Não é possível adicionar novas mensagens.", p->topico);
            char fifo_cli[20];
            snprintf(fifo_cli, sizeof(fifo_cli), FIFO_CLI, dados->usuarios[indice_usuario].pid);
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
            return; // Sai da função sem armazenar a mensagem
        }


        topico->mensagens[topico->contador] = nova_mensagem;
        topico->contador++;
        printf("Mensagem armazenada e enviada ao tópico '%s'.\n", p->topico);

    } else {

        printf("Mensagem temporária enviada ao tópico '%s'.\n", p->topico);
    }


    enviar_para_subscritores(dados, indice_topico, &nova_mensagem);
}





void salvar_mensagens_persistentes(DADOS* dados, char* nome_ficheiro) {
    FILE* ficheiro = fopen(nome_ficheiro, "w");
    if (ficheiro == NULL) {
        perror("[ERRO] Não foi possível abrir o ficheiro para salvar as mensagens persistentes");
        return;
    }

    for (int i = 0; i < MAX_TOPICOS; i++) {
        TOPICO* topico = &dados->topicos[i];
        if (topico->nome[0] != '\0') { // Tópico existe
            for (int j = 0; j < MAX_MENSAGENS; j++) {
                MENSAGEM* mensagem = &topico->mensagens[j];
                if (mensagem->texto[0] != '\0' && mensagem->duracao > 0) {
                    // Formata os dados e escreve cada mensagem separadamente
                    fprintf(ficheiro, "%s %s %d %s\n",
                            topico->nome, mensagem->usuario, mensagem->duracao, mensagem->texto);
                }
            }
        }
    }

    fclose(ficheiro);
}



void enviar_mensagens_persistentes(DADOS* dados, int indice_topico, int pid_usuario) {
    TOPICO* topico = &dados->topicos[indice_topico];
    char fifo_cli[20];
    sprintf(fifo_cli, FIFO_CLI, pid_usuario);

    for (int i = 0; i < MAX_MENSAGENS; i++) {
        MENSAGEM* mensagem = &topico->mensagens[i];
        if (mensagem->texto[0] != '\0' && mensagem->duracao > 0) {
            // Formata a mensagem no formato desejado
            RESPOSTA msg_persistente;
            snprintf(msg_persistente.str, sizeof(msg_persistente.str), "[%s] [%s] %s",
                     topico->nome, mensagem->usuario, mensagem->texto);

            // Envia a mensagem ao cliente
            int fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &msg_persistente, sizeof(RESPOSTA));
                close(fd_cli);
            } else {
                perror("[ERRO] Não foi possível enviar a mensagem persistente ao cliente");
            }
        }
    }
}




void carregar_mensagens_persistentes(DADOS* dados, char* nome_ficheiro) {
    FILE* ficheiro = fopen(nome_ficheiro, "r");
    if (ficheiro == NULL) {
        perror("[AVISO] Não foi possível abrir o ficheiro para carregar as mensagens persistentes");
        return;
    }

    char linha[1024];
    while (fgets(linha, sizeof(linha), ficheiro)) {

        linha[strcspn(linha, "\n")] = '\0';

        char nome_topico[30];
        char nome_usuario[30];
        int duracao;
        char mensagem_texto[300];


        sscanf(linha, "%29s %29s %d %[^\n]", nome_topico, nome_usuario, &duracao, mensagem_texto);



        int indice_topico = encontrar_topico(dados, nome_topico);
        if (indice_topico == -1) {
            indice_topico = criar_topico(dados, nome_topico);
            if (indice_topico == -1) {
                printf("[ERRO] Limite de tópicos atingido ao carregar mensagens persistentes.\n");
                continue;
            }
        }
        TOPICO* topico = &dados->topicos[indice_topico];


        MENSAGEM nova_mensagem;
        strcpy(nova_mensagem.texto, mensagem_texto);
        nova_mensagem.duracao = duracao;
        strcpy(nova_mensagem.usuario, nome_usuario);

        topico->mensagens[topico->contador] = nova_mensagem;
        topico->contador++;
    }

    fclose(ficheiro);
}









// Função da thread do administrador
void* thread_administrador(void* arg) {
    DADOS* dados = (DADOS*)arg; // Recebe o ponteiro para os dados
    char str[TAM];
    char comando[20], argumento[30] = "";

    while (dados->executa) {
        printf("ADMIN> ");
        fflush(stdout);

        if (fgets(str, sizeof(str), stdin) == NULL) {
            continue; //
        }

        str[strcspn(str, "\n")] = '\0'; // Remove newline
        if (sscanf(str, "%s %29s", comando, argumento) < 1) {
            printf("[ERRO] Comando inválido.\n");
            continue;
        }

        if (strcmp(comando, "users") == 0) {
            pthread_mutex_lock(&dados->mutex_usuarios);
            listar_usuarios(dados);
            pthread_mutex_unlock(&dados->mutex_usuarios);
        } else if (strcmp(comando, "show") == 0) {
            if (strlen(argumento) == 0) {
                printf("[ERRO] Comando 'show' requer o nome de um tópico.\n");
            } else {
                pthread_mutex_lock(&dados->mutex_topicos);
                mostrar_topico(dados, argumento);
                pthread_mutex_unlock(&dados->mutex_topicos);
            }
        } else if (strcmp(comando, "topics") == 0) {
            pthread_mutex_lock(&dados->mutex_topicos);
            listar_topicos(dados);
            pthread_mutex_unlock(&dados->mutex_topicos);
        } else if (strcmp(comando, "lock") == 0) {
            if (strlen(argumento) == 0) {
                printf("[ERRO] Comando 'lock' requer o nome de um tópico.\n");
            } else {
                pthread_mutex_lock(&dados->mutex_topicos);
                int indice_topico = encontrar_topico(dados, argumento);
                if (indice_topico == -1) {
                    printf("[ERRO] Tópico '%s' não encontrado.\n", argumento);
                } else {
                    dados->topicos[indice_topico].bloqueado = 1;
                    printf("Tópico '%s' foi bloqueado com sucesso.\n", dados->topicos[indice_topico].nome);
                }
                pthread_mutex_unlock(&dados->mutex_topicos);
            }
        } else if (strcmp(comando, "unlock") == 0) {
            if (strlen(argumento) == 0) {
                printf("[ERRO] Comando 'unlock' requer o nome de um tópico.\n");
            } else {
                pthread_mutex_lock(&dados->mutex_topicos);
                int indice_topico = encontrar_topico(dados, argumento);
                if (indice_topico == -1) {
                    printf("[ERRO] Tópico '%s' não encontrado.\n", argumento);
                } else {
                    dados->topicos[indice_topico].bloqueado = 0;
                    printf("Tópico '%s' foi desbloqueado com sucesso.\n", dados->topicos[indice_topico].nome);
                }
                pthread_mutex_unlock(&dados->mutex_topicos);
            }
        } else if (strcmp(comando, "remove") == 0) {
            if (strlen(argumento) == 0) {
                printf("[ERRO] Comando 'remove' requer o nome de um usuário.\n");
            } else {
                pthread_mutex_lock(&dados->mutex_usuarios);
                pthread_mutex_lock(&dados->mutex_topicos);
                remover_usuario(dados, argumento);
                pthread_mutex_unlock(&dados->mutex_topicos);
                pthread_mutex_unlock(&dados->mutex_usuarios);
            }
        } else if (strcmp(comando, "close") == 0) {
            printf("Encerrando o servidor e removendo todos os usuários...\n");

            pthread_mutex_lock(&dados->mutex_usuarios);
            pthread_mutex_lock(&dados->mutex_topicos);

            // Envia a mensagem de encerramento para todos os usuários
            for (int i = 0; i < MAX_USERS; i++) {
                if (dados->usuarios[i].pid != 0) { // Usuário conectado
                    char fifo_cli[20];
                    sprintf(fifo_cli, FIFO_CLI, dados->usuarios[i].pid);
                    int fd_cli = open(fifo_cli, O_WRONLY);
                    if (fd_cli != -1) {
                        RESPOSTA r;
                        strcpy(r.str, "EXIT"); // Mensagem de encerramento
                        write(fd_cli, &r, sizeof(RESPOSTA));
                        close(fd_cli);
                        printf("Mensagem de encerramento enviada para o utilizador com o PID %d.\n", dados->usuarios[i].pid);
                    } else {
                        printf("[ERRO] Não foi possível enviar mensagem para o utilizador com o PID %d.\n", dados->usuarios[i].pid);
                    }


                    remover_usuario_por_pid(dados, dados->usuarios[i].pid);
                }
            }

            pthread_mutex_unlock(&dados->mutex_topicos);
            pthread_mutex_unlock(&dados->mutex_usuarios);


            dados->executa = 0;


            PEDIDO quit_pedido = {0};
            strcpy(quit_pedido.comando, "close");
            int fd_srv = open(FIFO_SRV, O_WRONLY);
            if (fd_srv != -1) {
                write(fd_srv, &quit_pedido, sizeof(PEDIDO));
                close(fd_srv);
            }

            break;
        }

         else {
            printf("[ERRO] Comando inválido.\n");
        }
    }
    pthread_exit(NULL);
}


// Função da thread do servidor
void* thread_servidor(void* arg) {
    DADOS* dados = (DADOS*)arg;
    int fd = open(FIFO_SRV, O_RDWR);
    if (fd == -1) {
        perror("[ERRO] Falha ao abrir FIFO do servidor");
        return NULL;
    }

    PEDIDO p;
    RESPOSTA r;
    char fifo_cli[20];
    int fd_cli;

    while (dados->executa) {
        int res = read(fd, &p, sizeof(PEDIDO));
        if (res <= 0) {
            if (!dados->executa) break;
            perror("[ERRO] Erro ao ler do FIFO do servidor");
            continue;
        }

        if (strcmp(p.comando, "close") == 0) {
            printf("[SERVIDOR] O manager será encerrado.\n");
            break;
        }

        pthread_mutex_lock(&dados->mutex_usuarios);
        int indice_usuario = encontrar_usuario_por_pid(dados, p.pid);
        pthread_mutex_unlock(&dados->mutex_usuarios);

        if (strcmp(p.comando, "topics") == 0) {
            pthread_mutex_lock(&dados->mutex_topicos);

            char lista_topicos[1024] = "";
            for (int i = 0; i < MAX_TOPICOS; i++) {
                if (dados->topicos[i].nome[0] != '\0') {
                    char info_topico[100];
                    snprintf(info_topico, sizeof(info_topico), "TOPICO[%d]: Nome = %s, Subscritores = %d, Mensagens = %d\n",
                             i, dados->topicos[i].nome, dados->topicos[i].n_subs, dados->topicos[i].contador);
                    strcat(lista_topicos, info_topico);
                }
            }
            pthread_mutex_unlock(&dados->mutex_topicos);


            if (strlen(lista_topicos) == 0) {
                strcpy(r.str, "Nenhum tópico disponível no momento.");
            } else {
                strcpy(r.str, lista_topicos);
            }


            sprintf(fifo_cli, FIFO_CLI, p.pid);
            fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
        } else if (strcmp(p.comando, "unsubscribe") == 0) {
            pthread_mutex_lock(&dados->mutex_topicos);

            if (indice_usuario != -1) {
                int indice_topico = encontrar_topico(dados, p.topico);
                if (indice_topico == -1) {
                    strcpy(r.str, "[ERRO] Tópico não encontrado.");
                } else if (remover_subscritor(dados, indice_topico, indice_usuario)) {
                    strcpy(r.str, "Foi feita a inscrição no tópico '");
                    strcat(r.str, p.topico);
                    strcat(r.str, "'\n");
                } else {
                    strcpy(r.str, "[ERRO] Não estás subscrito neste tópico.");
                }
            } else {
                strcpy(r.str, "[ERRO] Usuário não encontrado.");
            }

            pthread_mutex_unlock(&dados->mutex_topicos);


            sprintf(fifo_cli, FIFO_CLI, p.pid);
            fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
        } else if (strcmp(p.comando, "login") == 0) {
            pthread_mutex_lock(&dados->mutex_usuarios);
            if (indice_usuario == -1) { // Usuário não está conectado
                int novo_indice = adicionar_usuario(dados, p.pid, p.mensagem);
                if (novo_indice >= 0) {
                    strcpy(r.str, "OK");
                } else if (novo_indice == -9) {
                    strcpy(r.str, "Erro: Nome já está em uso.");
                } else { // novo_indice == -1
                    strcpy(r.str, "Limite de usuários atingido.");
                }
            } else {
                strcpy(r.str, "Usuário já conectado.");
            }
            pthread_mutex_unlock(&dados->mutex_usuarios);


            sprintf(fifo_cli, FIFO_CLI, p.pid);
            fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
        }

        else if (strcmp(p.comando, "subscribe") == 0) {
            pthread_mutex_lock(&dados->mutex_topicos);
            if (indice_usuario != -1) {
                int indice_topico = encontrar_topico(dados, p.topico);
                if (indice_topico == -1) {
                    strcpy(r.str, "[ERRO] Tópico não encontrado.");
                } else if (inscrever_usuario(dados, indice_topico, indice_usuario)) {
                    strcpy(r.str, "Foi feita a inscrição no tópico '");
                    strcat(r.str, p.topico);
                    strcat(r.str, "'\n");



                    enviar_mensagens_persistentes(dados, indice_topico, p.pid);
                } else {
                    strcpy(r.str, "[ERRO] Falha ao se inscrever no tópico.");
                }
            } else {
                strcpy(r.str, "[ERRO] Usuário não encontrado.");
            }
            pthread_mutex_unlock(&dados->mutex_topicos);


            sprintf(fifo_cli, FIFO_CLI, p.pid);
            fd_cli = open(fifo_cli, O_WRONLY);
            if (fd_cli != -1) {
                write(fd_cli, &r, sizeof(RESPOSTA));
                close(fd_cli);
            }
        } else if (strcmp(p.comando, "msg") == 0) {
            pthread_mutex_lock(&dados->mutex_topicos);
            processar_mensagem(dados, &p, indice_usuario);
            pthread_mutex_unlock(&dados->mutex_topicos);
        } else if (strcmp(p.comando, "exit") == 0) {
            pthread_mutex_lock(&dados->mutex_topicos);
            pthread_mutex_lock(&dados->mutex_usuarios);
            remover_usuario_por_pid(dados, p.pid);
            pthread_mutex_unlock(&dados->mutex_topicos);
            pthread_mutex_unlock(&dados->mutex_usuarios);
        }
    }

    printf("Thread do servidor encerrada.\n");
    close(fd);
    pthread_exit(NULL);
}


void* thread_temporizador(void* arg) {
    DADOS* dados = (DADOS*)arg;

    while (dados->executa) {
        pthread_mutex_lock(&dados->mutex_topicos);
        for (int i = 0; i < MAX_TOPICOS; i++) {
            TOPICO* topico = &dados->topicos[i];
            if (topico->nome[0] != '\0') {
                for (int j = 0; j < MAX_MENSAGENS; j++) {
                    MENSAGEM* mensagem = &topico->mensagens[j];
                    if (mensagem->texto[0] != '\0' && mensagem->duracao > 0) {
                        mensagem->duracao--;
                        if (mensagem->duracao == 0) {

                            memset(mensagem, 0, sizeof(MENSAGEM));
                            if (topico->contador > 0) {
                                topico->contador--;
                            }
                            printf("Foi removida uma mensagem persistente do tópico '%s'.\n", topico->nome);
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock(&dados->mutex_topicos);
        sleep(1);
    }
    pthread_exit(NULL);
}





int main() {
    pthread_t t_admin, t_servidor, t_temporizador;


    DADOS dados;
    memset(&dados, 0, sizeof(DADOS));
    dados.executa = 1;


    pthread_mutex_init(&dados.mutex_usuarios, NULL);
    pthread_mutex_init(&dados.mutex_topicos, NULL);

    char* nome_ficheiro = getenv("MSG_FICH");

    // Criação do FIFO do servidor
    if (access(FIFO_SRV, F_OK) == 0) {
        printf("[ERRO] Já existe um manager aberto.\n");
        exit(3);
    }
    mkfifo(FIFO_SRV, 0600);
    int fd_srv = open(FIFO_SRV, O_RDWR);
    if (fd_srv == -1) {
        perror("[ERRO] Falha ao abrir FIFO do servidor");
        exit(2);
    }

    // Carrega mensagens persistentes
    carregar_mensagens_persistentes(&dados, nome_ficheiro);

    // Criação das threads
    if (pthread_create(&t_admin, NULL, thread_administrador, &dados) != 0) {
        perror("[ERRO] Não foi possivel criar a thread do administrador");
        exit(1);
    }
    if (pthread_create(&t_servidor, NULL, thread_servidor, &dados) != 0) {
        perror("[ERRO] Não foi possivel criar a thread do servidor");
        exit(1);
    }
    if (pthread_create(&t_temporizador, NULL, thread_temporizador, &dados) != 0) {
        perror("[ERRO] Não foi possivel criar a thread do temporizador");
        exit(1);
    }

    // Aguarda as threads finalizarem
    pthread_join(t_admin, NULL);
    pthread_join(t_servidor, NULL);
    dados.executa = 0;
    pthread_join(t_temporizador, NULL);

    
    salvar_mensagens_persistentes(&dados, nome_ficheiro);


    close(fd_srv);
    unlink(FIFO_SRV);
    pthread_mutex_destroy(&dados.mutex_usuarios);
    pthread_mutex_destroy(&dados.mutex_topicos);

    printf("Servidor encerrado.\n");
    pthread_exit(NULL);
}

