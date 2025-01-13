CC = g++
CFLAGS = -W -O2 -std=c++11
TARGET = project1

$(TARGET): project1.o
	$(CC) $(CFLAGS) -o $(TARGET) project1.o

project1.o: project1.h project1.cc
	$(CC) $(CFLAGS) -c -o project1.o project1.cc

clean:
	rm -rf *.o $(TARGET)