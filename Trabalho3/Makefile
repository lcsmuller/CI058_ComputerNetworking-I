CC  ?= gcc
CXX ?= g++

CFLAGS   = -Wall -Wextra -Wpedantic -I./
CXXFLAGS = -Wall -Wextra -Wpedantic -I./

MAIN = game
TEST = test_player

OBJS = player.o

all: $(MAIN)
test: $(TEST)

$(MAIN): $(OBJS)
$(TEST): $(OBJS)

clean:
	@ rm -f $(MAIN) $(TEST) $(OBJS)

.PHONY: all test clean

