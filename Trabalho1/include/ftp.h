#ifndef FTP_H
#define FTP_H

/** marcador de ínicio do cabeçalho (01111110 em binário) */
#define FTP_MESSAGE_HEADER_VALUE 0x7E
/** tamanho máximo do campo dados (mais paridade) em bytes */
#define FTP_MESSAGE_DATA_SIZE (64 + 1)

/**
 * possíveis tipos de mensagens a serem enviados
 * @note valores malucos escolhidos em aula pela classe
 */
enum ftp_message_types {
    FTP_TYPES_OK = 0x1,
    FTP_TYPES_NACK = 0x2,
    FTP_TYPES_ACK = 0x3,
    FTP_TYPES_ERROR = 0x11,
    FTP_TYPES_END = 0x2E, /**< finaliza transferência */
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
 *
 * - [0]:
 *      - bits 0 ~ 7: marcador de início
 *          (assume o valor de @ref FTP_MESSAGE_HEADER_VALUE)
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
struct ftp_message {
    /** marcador de início */
    unsigned header : 8;
    /** tamanho do campo data */
    unsigned size : 6;
    /** contador de sequencia da mensagem */
    unsigned seq : 4;
    /** tipo da mensagem */
    enum ftp_message_types type : 6;
    /** dados, com o último byte reservado para paridade */
    unsigned char data[FTP_MESSAGE_DATA_SIZE];
};

/**
 * @brief Inicializa @ref ftp_message
 *
 * @param msg mensagem pré-alocada a ser inicializada
 */
void ftp_message_init(struct ftp_message *msg);

/**
 * @brief Imprime conteúdo de @ref ftp_message na tela
 * @note inclua a flag `-DFTP_DEBUG` na compilação para incluir o hexdump das
 *      mensagens
 *
 * @param msg mensagem a ter seu conteúdo impresso
 * @param out saída a receber conteúdo
 */
void ftp_message_print(const struct ftp_message *msg, FILE *out);

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

/**
 * @brief Envia mensagem FTP
 * @note Wrapper básico de send()
 *
 * @param sockfd socket do cliente ou servidor
 * @param msg mensagem FTP a ser enviada
 * @return valor de retorno de send()
 */
int ftp_message_send(int sockfd, struct ftp_message *msg);

/**
 * @brief Recebe mensagem FTP
 * @note Wrapper básico de recv()
 *
 * @param sockfd socket do cliente ou servidor
 * @param msg mensagem FTP recebida
 * @return valor de retorno de recv()
 */
int ftp_message_recv(int sockfd, struct ftp_message *msg);

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
 * @brief Inicia um raw socket
 *
 * @param device o dispositivo para se conectar (ex: "lo" para loopback)
 * @return valor negativo em caso de falha, ou um file-descriptor para socket
 *      se bem sucedido
 */
int ftp_raw_socket(const char device[]);

#endif /* FTP_H */
