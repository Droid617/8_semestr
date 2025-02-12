.PHONY: all clean

all: otp


otp: main.cpp worker.h generator.h
	g++ -std=c++0x $^ -o $@ 
	
clean:
	del otp.exe

#mingw32-make