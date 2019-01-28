
CC = g++
CCFLAGS = -Wall -Wextra -std=c++17 -g3

LIBS = -lpthread

PATH_SRC = ./src
PATH_INC = ./include
PATH_BIN = ./bin
PATH_TEST = ./test

.PHONY: all
all:
	mkdir -p $(PATH_BIN)
	@echo
	@echo "*** Compiling object files ***"
	@echo "***"
	make $(OBJS)
	@echo "***"

.PHONY: clean
clean:
	@echo
	@echo "*** Purging binaries ***"
	@echo "***"
	rm -rvf $(PATH_BIN)
	@echo "***"


THREAD_POOL_DEP = $(addprefix $(PATH_INC)/, thread_pool.hpp) $(PATH_SRC)/thread_pool.cpp


$(PATH_BIN)/thread_pool.o: $(THREAD_POOL_DEP)
	$(CC) -I $(PATH_INC) $(DEFINED) $(CCFLAGS) $(PATH_SRC)/thread_pool.cpp -c -o $(PATH_BIN)/thread_pool.o


OBJS = $(addprefix $(PATH_BIN)/,  thread_pool.o)

$(PATH_BIN)/%.exe: $(PATH_TEST)/%.cpp $(OBJS)
	$(CC) -I $(PATH_INC) $(DEFINED) $(CCFLAGS) $< $(OBJS) $(LIBS) -o $@
