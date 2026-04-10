CLI_SRC = cli.c
SVR_SRC = svr.c
CC = gcc
CLI_EXEC = cli
SVR_EXEC = svr

all: $(CLI_OBJS) $(SVR_OBJS)

$(CLI_EXEC): $(CLI_SRC)
	$(CC) -o $@ $^

$(SVR_EXEC): $(SVR_SRC)
	$(CC) -o $@ $^
clean:
	rm -rf $(CLI_EXEC) $(SVR_EXEC)