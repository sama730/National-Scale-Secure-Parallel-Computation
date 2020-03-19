
CC = g++

CPPFLAGS := -std=c++11

SRC = FourParty.cpp

.phony: all clean

EXES = FourParty
all: $(EXES)
$(EXES): 
	$(CC) $(CPPFLAGS) $(SRC) -o FourParty

clean:
	rm -rf $(EXES)

