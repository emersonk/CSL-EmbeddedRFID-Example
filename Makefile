
CC := gcc


CFLAGS_RFID_OS_API = -I./include/rfid/linux

CFLAGS_RFID_API = -I./include/rfid $(CFLAGS_RFID_OS_API)


CFLAGS_RFID_OS_TX =  -I./include/rfid/linux -I./include/rfid/tx/linux

CFLAGS_RFID_TX = -I./include/rfid -I./include/rfid/tx $(CFLAGS_RFID_OS_TX)

#LDFLAGS_RFID_TX = -L./lib -lrfidtx -lcpl
LDFLAGS_RFID_TX = -L./lib -lrfidtx -lcpl -Wl,--no-as-needed -lrt


CFLAGS_RFID_INET =


CFLAGS_RFID_CSL = -I./include/rfid/csl


#CFLAGS_RFID = $(CFLAGS_RFID_API) $(CFLAGS_RFID_CSL)

CFLAGS_RFID = $(CFLAGS_RFID_TX) $(CFLAGS_RFID_CSL)
LDFLAGS_RFID = $(LDFLAGS_RFID_TX)

#CFLAGS_RFID = $(CFLAGS_RFID_INET) $(CFLAGS_RFID_CSL)


CFLAGS = -g $(CFLAGS_RFID)
CFLAGS = -g -DDRIVER_HAVE_BUG $(CFLAGS_RFID)
LDFLAGS = $(LDFLAGS_RFID)


all : example

example : example.c
	$(CC) $(CFLAGS) -o example example.c -lpthread $(LDFLAGS) -lm

test: example
	./example

test_lib: csl_rfid_tx.o

csl_rfid_tx.o: ./include/rfid/tx/code/csl_rfid_tx.inc.c
	$(CC) $(CFLAGS_RFID_TX) -o csl_rfid_tx.o -c ./include/rfid/tx/code/csl_rfid_tx.inc.c

clean:
	-rm -f *.o
	-rm example

