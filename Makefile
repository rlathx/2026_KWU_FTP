OBJS = 2024402034_opt.c
CC = gcc
EXEC = testopt

testopt: $(OBJS)
	$(CC) -o $@ $^

clean: 
	rm -rf $(EXEC)