#ifndef FTP_H
#define FTP_H

/** marcador de ínicio do cabeçalho (01111110 em binário) */
#define FTP_HEADER_START 0x7E
/** tamanho máximo do dado enviado */
#define FTP_DATA_MAX 64

/**
 * possíveis tipos de mensagens a serem enviados
 * @note valores malucos escolhidos em aula pela classe
 */
enum ftp_message_types {
    FTP_TYPES_OK = 0x1,
    FTP_TYPES_NACK = 0x2,
    FTP_TYPES_ACK = 0x3,
    FTP_TYPES_ERROR = 0x11,
    FTP_TYPES_END = 0x2E, /** finaliza transferência */
    FTP_TYPES_LS = 0x3F, /**< envia `ls` */
    FTP_TYPES_CD = 0x6, /**< envia `cd` */
    FTP_TYPES_GET = 0x9, /**< envia `get` */
    FTP_TYPES_PUT = 0xA, /**< envia `put` */
    FTP_TYPES_MKDIR = 0x10, /**< envia `mkdir` */
    FTP_TYPES_FDESC = 0x18, /**< descritor de arquivos */
    FTP_TYPES_DATA = 0x20,
};

/**
 * @brief Estrutura para mensagem a ser enviada entre cliente e servidor
 * @note atributo `__packed__` é uma extensão que garante que não haja
 *      "lacunas" de bytes entre os campos
 */
struct __attribute__((__packed__)) ftp_message {
    /** marcador de início (assume o valor de @ref FTP_HEADER_START) */
    unsigned header : 8;
    /** quantidade de bytes do campo data */
    unsigned size : 6;
    /** contador sem sinal de 4 bits */
    unsigned sequence : 4;
    /** comando a ser enviado */
    enum ftp_message_types type : 6;
    /** data (último byte reservado para paridade) */
    char data[FTP_DATA_MAX + 1];
};

/**
 * @brief Inicializa @ref ftp_message
 *
 * @return retorna os campos inicializados
 */
struct ftp_message ftp_message_init(void);

/**
 * @brief Imprime conteúdo de @ref ftp_message na tela
 * @note ligue a saída ao comando `hexdump` para observar conteúdo bitwise
 *
 * @param msg mensagem a ter seu conteúdo impresso
 * @param out saída a receber conteúdo
 * @return quantidade de bytes escritos
 */
size_t ftp_message_print(const struct ftp_message *msg, FILE *out);

/**
 * @brief Atualiza conteúdo de @ref ftp_message
 *
 * @param msg mensagem a ter seu conteúdo atualizado
 * @param type comando shell que os argumentos em `data` correspondem
 * @param data argumentos do comando shell
 * @param size tamanho de `data` em bytes
 * @return quantidade de bytes escritos
 */
void ftp_message_update(struct ftp_message *msg,
                        enum ftp_message_types type,
                        const char data[],
                        size_t size);

struct ftp_server {
    struct sockaddr_in6 *addr;
    int sockfd;
    int timeout_ms;
    struct pollfd *fds;
    int nfds;
    char buf[1024];
};

void ftp_command_dispatch(struct ftp_message *msg);

#endif /* FTP_H */