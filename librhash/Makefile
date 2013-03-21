OBJS = $(patsubst %.c,%.o,$(wildcard *.c)) 
INC = ../inc
LIB_NAME = librhash.a
LIB_DIR = .
CC = gcc
.PHONY = all clean install

all: $(OBJS)
	ar -cqs $(LIB_DIR)/$(LIB_NAME) $(OBJS) 
	ar -cr  $(LIB_DIR)/$(LIB_NAME) $(OBJS) 
	ranlib $(LIB_DIR)/$(LIB_NAME)	
%.o: %.c
	$(CC) -I$(INC) -c $<	
	
clean :
	rm -rf *.o
	rm -rf *.a