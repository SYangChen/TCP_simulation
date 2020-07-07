CC = gcc
TARGET1 = client
TARGET2 = server

FLAG = -std=c11

all: $(TARGET1).c $(TARGET2).c
	$(CC) $(FLAG) -c $(TARGET1).c $(TARGET2).c header.h
	$(CC) $(FLAG) -o $(TARGET1) $(TARGET1).o 
	$(CC) $(FLAG) $(TARGET2).o -lm -o $(TARGET2) 

clean:
	rm -f $(TARGET1) $(TARGET2)
	rm -f $(TARGET1).o $(TARGET2).o
