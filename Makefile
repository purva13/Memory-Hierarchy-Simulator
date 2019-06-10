CC = g++
OPT = -O3
OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

SIM_SRC = main.cpp

SIM_OBJ = main.o 

all: sim
	@echo "----"

sim: $(SIM_OBJ)
	$(CC) -o sim_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "----"

.cc.o:
	$(CC) $(CFLAGS) -c $*.cc

clean:
	rm -f *.o sim_cache

clobber:
	rm -f *.o
