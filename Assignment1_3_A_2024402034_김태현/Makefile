CLI_SRC = cli.c
SVR_SRC = srv.c
CC = gcc
CLI_EXEC = cli
SVR_EXEC = srv

all: $(CLI_EXEC) $(SVR_EXEC)

$(CLI_EXEC): $(CLI_SRC)
	$(CC) -o $@ $^

$(SVR_EXEC): $(SVR_SRC)
	$(CC) -o $@ $^
clean:
	rm -rf $(CLI_EXEC) $(SVR_EXEC)