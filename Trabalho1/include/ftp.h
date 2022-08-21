#ifndef FTP_H
#define FTP_H

/** marcador de ínicio do cabeçalho (01111110 em binário) */
#define FTP_HEADER_START 0x7E

#ifndef FTP_SERVER_PORT
/** porta padrão do servidor */
#define FTP_SERVER_PORT 5010
#endif

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

/** tamanho máximo do campo header em bits */
#define FTP_MESSAGE_HEADER_BMAX 8
/** tamanho máximo do campo tamanho em bits */
#define FTP_MESSAGE_SIZE_BMAX 6
/** tamanho máximo do campo contador em bits */
#define FTP_MESSAGE_SEQ_BMAX 4
/** tamanho máximo do campo tipo em bits */
#define FTP_MESSAGE_TYPE_BMAX 6
/** tamanho máximo do campo dados em bits */
#define FTP_MESSAGE_DATA_BMAX (64 * 8)
/** tamanho máximo do campo paridade em bits */
#define FTP_MESSAGE_PARITY_BMAX 8

/** tamanho máximo da mensagem em bytes */
#define FTP_MESSAGE_MAX                                                       \
    ((FTP_MESSAGE_HEADER_BMAX + FTP_MESSAGE_SIZE_BMAX + FTP_MESSAGE_SEQ_BMAX  \
      + FTP_MESSAGE_TYPE_BMAX + FTP_MESSAGE_DATA_BMAX                         \
      + FTP_MESSAGE_PARITY_BMAX)                                              \
     / 8)

/** @brief Estrutura para mensagem a ser enviada entre cliente e servidor */
struct ftp_message {
    /**
     * - [0]:
     *      - bits 0 ~ 7: marcador de início
     *          (assume o valor de @ref FTP_HEADER_START)
     * - [1]:
     *      - bits 8 ~ 13: tamanho do campo data (n)
     *      - bits 14 ~ 17: contador sem sinal (sequencia da mensagem)
     * - [2]:
     *      - bits 14 ~ 17: contador sem sinal (sequencia da mensagem)
     *      - bits 18 ~ 23: tipo do comando a ser enviado / recebido
     * - [3 ~ (n - 1)]: (n pode estar entre 3 e 67, vide campo tamanho)
     *      - bits 24 ~ (n * 8) - 1: data
     * - [n]:
     *      - bits (n * 8) ~ (n * 8) + 7: byte de paridade
     */
    unsigned char payload[FTP_MESSAGE_MAX];
};

/**
 * @brief Inicializa @ref ftp_message
 *
 * @param msg mensagem pré-alocada a ser inicializada
 */
void ftp_message_init(struct ftp_message *msg);

/**
 * @brief Obtêm tamanho do campo `data` da mensagem
 *
 * @param msg mensagem ftp
 * @return tamanho do campo `data` em bytes
 */
unsigned ftp_message_get_data_size(const struct ftp_message *msg);

/**
 * @brief Obtêm o campo `tipo` da mensagem
 *
 * @param msg mensagem ftp
 * @return o tipo da mensagem
 */
enum ftp_message_types ftp_message_get_type(const struct ftp_message *msg);

/**
 * @brief Obtêm o campo `data` da mensagem
 *
 * @param msg mensagem ftp
 * @return os dados da mensagem
 */
unsigned char *ftp_message_get_data(const struct ftp_message *msg);

/**
 * @brief Imprime conteúdo de @ref ftp_message na tela
 * @note ligue a saída ao comando `hexdump` para observar conteúdo bitwise
 *
 * @param msg mensagem a ter seu conteúdo impresso
 * @param out saída a receber conteúdo
 * @return quantidade de bytes escritos na descrição
 */
int ftp_message_print(const struct ftp_message *msg, FILE *out);

/**
 * @brief Atualiza conteúdo de @ref ftp_message
 *
 * @param msg mensagem a ter seu conteúdo atualizado
 * @param type comando shell que os argumentos em `data` correspondem
 * @param data argumentos do comando shell
 * @param size tamanho de `data` em bytes
 * @return `true` se houve truncamento de `data`
 */
_Bool ftp_message_update(struct ftp_message *msg,
                         enum ftp_message_types type,
                         const char data[],
                         unsigned size);

/**
 * @brief Decodifica e executa instruções contidas na mensagem FTP
 * @note will return `NULL` in case there's no output to be read
 * @see ftp_file_close()
 *
 * @param msg mensagem a ser decodificada e executada
 * @return a wrapped `FILE*` pointer, should be free'd with ftp_file_close()
 */
struct ftp_file *ftp_message_unpack(struct ftp_message *msg);

/** @brief Wrapped `FILE` pointer */
struct ftp_file {
    /** output stream */
    FILE *stream;
};

/**
 * @brief Closes an open @ref ftp_file
 *
 * @param file file received at ftp_message_unpack()
 */
void ftp_file_close(struct ftp_file *file);

/**
 * @brief Inicializa servidor
 *
 * @return estrutura opaca para servidor
 */
struct ftp_server *ftp_server_init(void);

/**
 * @brief Libera recursos alocados ao servidor
 *
 * @param server servidor a ser liberado
 */
void ftp_server_cleanup(struct ftp_server *server);

/**
 * @brief Obtêm endereço IPV6 do servidor
 *
 * @param endereço em que o servidor reside
 */
struct sockaddr_in6 ftp_server_get_addr(void);

/**
 * @brief Inicializa cliente
 *
 * @return estrutura opaca para cliente
 */
struct ftp_client *ftp_client_init(void);

/**
 * @brief Libera recursos alocados ao cliente
 *
 * @param server cliente a ser liberado
 */
void ftp_client_cleanup(struct ftp_client *client);

/**
 * @brief Envia mensagem FTP ao servidor
 * @note Wrapper básico de sendto()
 *
 * @param client cliente inicializado por ftp_client_init()
 * @param msg mensagem FTP a ser enviada
 * @return valor de retorno de sendto()
 */
int ftp_client_send(struct ftp_client *client, struct ftp_message *msg);

/**
 * @brief Envia mensagem FTP ao servidor
 * @note Wrapper básico de recvfrom()
 *
 * @param client cliente inicializado por ftp_client_init()
 * @param msg mensagem FTP recebida do servidor
 * @return valor de retorno de recvfrom()
 */
int ftp_client_recv(struct ftp_client *client, struct ftp_message *msg);

#endif /* FTP_H */
