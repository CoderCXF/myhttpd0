CC = gcc
CFLAGS = -Wall -g 
target = myhttpd_v0
all : $(target)
$(target) : myhttpd0.c
	$(CC) $< -o $@ $(CFLAGS)
.PHONY : all clean
clean :
	-rm -f $(target) *.o
