CC = gcc
CFLAGS = -Wall -g 
target = myhttpd_v0 client
all : $(target)
myhttpd_v0 : myhttpd0.c
	$(CC) $< -o $@ $(CFLAGS)
client : client.c
	$(CC) $< -o $@ $(CFLAGS)
.PHONY : all clean
clean :
	-rm -f $(target) *.o
