OBJS = $(patsubst %.c,%.o,$(wildcard *.c))
BIN_NAME = rhash
BIN_DIR = ../bin
INC = ../inc
LIB_NAME = rhash
LIB_DIR = ../librhash 
CC = gcc 

all : $(OBJS)
	$(CC) $(OBJS) -I$(INC) -L$(LIB_DIR) -l$(LIB_NAME) -o $(BIN_DIR)/$(BIN_NAME) 
%.o: %.c 
	$(CC) -I$(INC) -c $<	
	                                       
clean :
	rm -rf *.o
	rm $(BIN_DIR)/*