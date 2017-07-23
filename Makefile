CXX= g++
CXXFLAGS= -c -Wall -std=c++11 -DNDEBUG
CODEFILES= ex2.tar uthreads.cpp Thread.h Thread.cpp Makefile README
LIBOBJECTS= uthreads.o Thread.o


# Default
default: uthreads


# Executables
uthreads: uthreads.o Thread.o
	ar rcs libuthreads.a $(LIBOBJECTS)
	-rm -vf *.o


# Object Files
Thread.o: Thread.h Thread.cpp
	$(CXX) $(CXXFLAGS) Thread.cpp -o Thread.o
	

uthreads.o: uthreads.cpp uthreads.h Thread.h
	$(CXX) $(CXXFLAGS) uthreads.cpp -o uthreads.o


# tar
tar:
	tar -cvf $(CODEFILES)


# Other Targets
clean:
	-rm -vf *.o *.a *.tar
	
	
