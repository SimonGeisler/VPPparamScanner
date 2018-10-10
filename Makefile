CC=g++
LINK=g++
EXPRTK=./exprtk/exprtk/
CCFLAGS=-I./include/ -I$(EXPRTK) -std=c++11
.PHONY : all
all : VPPParamScanner
 
VPPParamScanner: Runner.o
	$(LINK) -o VPPParamScanner.exe Runner.o -lstdc++
 
Runner.o : Runner.cpp
	$(CC) -c Runner.cpp -o Runner.o $(CCFLAGS)
 
.PHONY : clean
clean :
	rm Runner.o runner