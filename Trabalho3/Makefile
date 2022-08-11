CXX ?= g++

CXXFLAGS = -Wall -Wextra -Wpedantic -I./

TEST = test
OBJS = player.o

all: $(TEST)

$(TEST): $(OBJS)

clean:
	@ rm -f $(TEST) $(OBJS)

.PHONY: all clean

