CC = g++
CFLAGS = -O2 -Wall
LFLAGS = -lm -lpthread
PAPI_INCLUDE = /home/chih/PMU/papi-5.5.1/src
PAPI_LIBRARY = /home/chih/PMU/papi-5.5.1/src/libpapi.a
FILE = rp_t

all:	$(FILE)

$(FILE):	$(FILE).o
	$(CC) $(LFLAGS) -o $(FILE) $(FILE).o $(PAPI_LIBRARY)

$(FILE).o:	$(FILE).cpp
	$(CC) $(CFLAGS) -I$(PAPI_INCLUDE) -c $(FILE).cpp

clean:
	rm -f *.o *~ $(FILE)
