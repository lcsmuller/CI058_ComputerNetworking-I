CC = gcc

SRC_DIR     = src
INCLUDE_DIR = include

EXES = server client

OBJS = $(SRC_DIR)/ftp_message.o    \
       $(SRC_DIR)/ftp_raw_socket.o

CFLAGS += -Wall -Wextra -Werror -Wpedantic -I$(INCLUDE_DIR)

SERVER_PORT = 5010

all:  $(EXES)

debug:
	@ CFLAGS="-DFTP_DEBUG" $(MAKE)

test:
	@ echo "Hello World!" | nc 127.0.0.1 $(SERVER_PORT)

$(EXES): $(OBJS)

clean: 
	@ rm -f $(EXES) $(OBJS)

.PHONY: all test
