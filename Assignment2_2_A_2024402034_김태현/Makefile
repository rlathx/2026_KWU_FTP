CLI_SRC = cli.c
SRV_SRC = srv.c
CC = gcc
CLI_EXEC = cli
SRV_EXEC = srv

all: $(CLI_EXEC) $(SRV_EXEC)

$(CLI_EXEC): $(CLI_SRC)
	$(CC) -o $@ $^

$(SRV_EXEC): $(SRV_SRC)
	$(CC) -o $@ $^
clean:
	rm -rf $(CLI_EXEC) $(SRV_EXEC)