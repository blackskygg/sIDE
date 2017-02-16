OBJS = list.o dict.o
CC = gcc --std=c11 -Wall

sas.exe: $(OBJS) sas.c
	$(CC) $(OBJS) sas.c -o sas.exe

%.o : %.c
	$(CC) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) sas.exe
