#$Id: Makefile,v 1.1 2008/03/17 06:02:36 zhouzr Exp $
COMPILER?=g++
CC:=$(COMPILER)

WORKROOT=../../../

ifeq ($(MAC),64)
  LIBPATH=$(WORKROOT)lib2-64
else
  LIBPATH=$(WORKROOT)lib2
endif
#added by fatcat at 20180721 for railway project
#DEVPATH=$(WORKROOT)libsource4
DEVPATH=../lib/libsource4/

#ULLIB=$(DEVPATH)/ullib/
#NSHEAD=$(WORKROOT)/public/nshead/
#UBLIB=$(WORKROOT)/public/ub/
ULLIB=$(DEVPATH)/ullib/output/
NSHEAD=$(DEVPATH)/nshead/output/
UBLIB=$(DEVPATH)/ub/output/
MCPACK=$(DEVPATH)/mcpack/output
#MCPACK1LIB=$(DEVPATH)/mcpack1/output
INCLUDE_PATH=-I$(ULLIB)/include/ -I$(NSHEAD)/include/ -I$(UBLIB)/include -I$(MCPACK)/include    -I.  -I../dev/include/nopoll -I../dev/include/fvad
#LIB_PATH=-L$(ULLIB)/lib -L$(UBLIB)/lib  -L$(NSHEAD)/lib -L$(MCPACK1LIB)/lib
LIB_PATH=-L$(ULLIB)/lib -L$(UBLIB)/lib  -L$(NSHEAD)/lib  -L$(MCPACK)/lib -L../dev/lib

OBJ:=main

#CPPFLAGS = -g -Wall -W -Winline -Wshadow -Wreturn-type -Wtrigraphs -Wformat -Wparentheses -Wpointer-arith -Werror -D_GNU_SOURCE
CPPFLAGS = -g -D_GNU_SOURCE

LIB = -lnshead -lullib -lmcpack -lev -lnopoll  -lfvad -lpthread   -Wl,-rpath=../dev/lib/   -Wl,-rpath=./ -Wl,-rpath=./lib

.PHONY: all clean doc wc install

all: $(OBJ)

main : main.c  ws_server.c asr_client.c  tcp_server.c
	$(CC) -o $@ $^ $(CPPFLAGS) $(INCLUDE_PATH) $(LIB_PATH) $(LIB)
test_pack : test_pack.cpp
	$(CC) -o $@ $^ $(CPPFLAGS) $(INCLUDE_PATH) $(LIB_PATH) $(LIB)

test_pack2 : test_pack2.c
	$(CC) -o $@ $^ $(CPPFLAGS) $(INCLUDE_PATH) $(LIB_PATH) $(LIB)

clean:
	rm -f *.o $(OBJ)

doc:
	doxygen
wc:
	wc *.cpp *.c *.h
