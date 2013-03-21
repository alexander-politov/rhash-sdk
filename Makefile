OBJS = $(patsubst %.c,%.o,$(wildcard *.c)) 
LIB_DIR = librhash
LIB_MK_NAME = librhash.mk
SAMPLE_DIR = rhash
SAMPLE_MK_NAME = rhash.mk
.PHONY = all clean install

all: lib sample 
	 
lib:	 
	+make -C $(LIB_DIR) all 
sample: lib 
	+make -C $(SAMPLE_DIR) all 
 
 	
clean: rhash_clean lib_clean
lib_clean:
	+make -C $(LIB_DIR) clean
rhash_clean:
	+make -C $(SAMPLE_DIR) clean