
all:   ftelnet 

# sudo apt install libssh-dev


CCFLAGS = -c -g -O0 -D_GNU_SOURCE -Wall -pthread 
LDFLAGS = -Wl,-v -Wl,-Map=a.map -Wl,--cref -Wl,-t -lpthread -pthread -lssh
#LDFLAGS = -lpthread -pthread
ARFLAGS = -rcs

# buildroot passes these in
CC = gcc
LD = gcc
AR = ar

ftelnet:  ftelnet.o ftty.o frbuff.o ftelnet_cmd.o flist.o fprintbuff.o mkaddr.o
	$(LD) $(LDFLAGS1)  -o ftelnet ftelnet.o ftelnet_cmd.o flist.o fprintbuff.o mkaddr.o frbuff.o ftty.o -lpthread



ftimeout: ftimeout.o
	$(LD) $(LDFLAGS) -o ftimeout ftimeout.o 

frbuff.o: frbuff.c frbuff.h
	$(CC)  $(CCFLAGS) -o frbuff.o  frbuff.c
ftty.o: ftty.c ftty.h
	$(CC)  $(CCFLAGS) -o ftty.o  ftty.c
ftimeout.o: ftimeout.c 
	$(CC)  $(CCFLAGS) -o ftimeout.o  ftimeout.c



fprintbuff.o: fprintbuff.c fprintbuff.h
	$(CC)  $(CCFLAGS) -o fprintbuff.o  fprintbuff.c

ftelnet.o: ftelnet.c 
	$(CC)  $(CCFLAGS) -o ftelnet.o  ftelnet.c

ftelnet_cmd.o: ftelnet_cmd.c 
	$(CC)  $(CCFLAGS) -o ftelnet_cmd.o  ftelnet_cmd.c

flist.o: flist.c 
	$(CC)  $(CCFLAGS) -o flist.o  flist.c

mkaddr.o : mkaddr.c mkaddr.h   
	$(CC)  $(CCFLAGS) -o mkaddr.o   mkaddr.c  








clean:
	rm -rf *.o
	rm -rf *.map
	rm -rf fs_expect
	rm -rf fs_ssh
	rm -rf ftimeout

.phony x:
x:
	./fs_expect


