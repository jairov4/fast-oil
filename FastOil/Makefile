CC=gcc
CFLAGS=-I../../boost -I./ -Wall -m64 -std=c++11 -D_NOT_USE_AVX256 -O3
LDFLAGS=-m64 -D_NOT_USE_AVX256

BUILDDIR=x64/gnu

ODIR=$(BUILDDIR)/obj
LDIR =../lib

LIBS=-lm -lstdc++

_DEPS=Nfa.h
DEPS=$(_DEPS)

SOURCES=main.cpp Nfa.cpp NfaDotExporter.cpp OilTrainer.cpp SamplesReader.cpp Testing.cpp
EXECUTABLE=$(BUILDDIR)/fastoil.exe

_OBJ=$(SOURCES:.cpp=.o)
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXECUTABLE): $(OBJ)
	gcc -o $@ $^ $(LDFLAGS) $(LIBS)

all: $(EXECUTABLE)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
	rm -f $(EXECUTABLE)