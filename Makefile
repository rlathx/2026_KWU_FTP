OBJS = kw2024402034_opt.c
CC = gcc
EXEC = kw2024402034_opt

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^

clean:
	rm -rf $(EXEC)