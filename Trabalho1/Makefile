CC = gcc

MAIN  = main
TESTS = server client

OBJS = ftp_message.o ftp_command.o ftp_server.o ftp_client.o

CFLAGS = -Wall -Wextra -Werror -Wpedantic -I.

SERVER_PORT = 5010

all:  $(MAIN)
test:  $(TESTS)

test_port:
	@ echo "Hello World!" | nc 127.0.0.1 $(SERVER_PORT)

$(MAIN): $(OBJS)
$(TESTS): $(OBJS)

clean: 
	@ rm -f $(MAIN) $(TESTS) $(OBJS)

.PHONY: all test test_port
