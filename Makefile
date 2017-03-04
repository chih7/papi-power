CC = g++
CFLAGS = -O2 -Wall
LFLAGS = -lm -lpthread -lrt
PAPI_INCLUDE = /home/chih/PMU/papi-5.5.1/src
PAPI_LIBRARY = /home/chih/PMU/papi-5.5.1/src/libpapi.a
FILE = rp_t2

all:    clean $(FILE)

$(FILE):	$(FILE).o load.o
	$(CC) $(LFLAGS) -o $(FILE) $(FILE).o $(PAPI_LIBRARY)

$(FILE).o:	$(FILE).cpp
	$(CC) $(CFLAGS) -I$(PAPI_INCLUDE) -c $(FILE).cpp

load.o:		load.cpp
	$(CC) $(CFLAGS) -c load.cpp
	
clean:
	rm -f *.o *~ $(FILE)
