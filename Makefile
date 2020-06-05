IDIR=./include
ODIR=./obj
SRCDIR=./src
BINDIR=./bin

CC=g++
CFLAGS= -std=c++0x -lpthread -I $(IDIR)

DEPS = $(shell find $(IDIR) -name *.h -o -name *.hpp)

_OBJ = myDate.o patient.o ageGroup.o myLowLvlIO.o sendReceive.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

OBJ1 = $(ODIR)/master.o

OBJ2 = $(ODIR)/worker.o

OBJ3 = $(ODIR)/whoClient.o

OBJ4 = $(ODIR)/whoServer.o



all: $(BINDIR)/master $(BINDIR)/worker $(BINDIR)/whoClient $(BINDIR)/whoServer 

$(BINDIR)/master: $(OBJ1) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(BINDIR)/worker: $(OBJ2) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(BINDIR)/whoClient: $(OBJ3) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(BINDIR)/whoServer: $(OBJ4) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean clean_logs

clean:
	rm -f $(ODIR)/*.o $(BINDIR)/*
	
clean_logs:
	rm ./logs/*
