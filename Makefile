CC=gcc

EXTRA_WARNINGS = -Wall 

LIBS = -lcurl -lxml2 -lsqlite3 -lpthread

CFLAGS=-ggdb $(EXTRA_WARNINGS)

BINS=tacmsgrouter

all: tacmsgrouter

tacmsgrouter: tacmsgrouter.c log.c ini.c
	 $(CC) $+ $(CFLAGS) $(LIBS) -o $@ -I.

	 
clean:
	rm -rf $(BINS)
	rm *.o
	
