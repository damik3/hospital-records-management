IDIR=./include
ODIR=./obj
SRCDIR=./src
BINDIR=./bin

CC=g++
CFLAGS= -std=c++0x -lpthread -I $(IDIR)

DEPS = $(shell find $(IDIR) -name *.h -o -name *.hpp)

_OBJ = myDate.o patient.o ageGroup.o myLowLvlIO.o sendReceive.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_OBJ1 = master.o
OBJ1 = $(patsubst %,$(ODIR)/%,$(_OBJ1))

_OBJ2 = worker.o
OBJ2 = $(patsubst %,$(ODIR)/%,$(_OBJ2))

_OBJ3 = whoClient.o
OBJ3 = $(patsubst %,$(ODIR)/%,$(_OBJ3))

_OBJ4 = whoServer.o
OBJ4 = $(patsubst %,$(ODIR)/%,$(_OBJ4))



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
