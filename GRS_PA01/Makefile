CC = gcc
CFLAGS = -Wall -pthread 

PROG_A_SRC = MT25024_Part_A_Program_A.c
PROG_B_SRC = MT25024_Part_A_Program_B.c
WORKER_SRC = MT25024_Part_B_Workers.c

EXEC_A = program_a
EXEC_B = program_b

all: $(EXEC_A) $(EXEC_B)

$(EXEC_A): $(PROG_A_SRC) $(WORKER_SRC)
	$(CC) $(CFLAGS) -o $(EXEC_A) $(PROG_A_SRC) $(WORKER_SRC) -lm

$(EXEC_B): $(PROG_B_SRC) $(WORKER_SRC)
	$(CC) $(CFLAGS) -o $(EXEC_B) $(PROG_B_SRC) $(WORKER_SRC) -lm

clean:
	# Removes binaries and object files (Required to avoid 0 marks)
	rm -f $(EXEC_A) $(EXEC_B) *.o
	# Removes temporary logs created by top/iostat/io-workers
	rm -f top_*.txt iostat_*.txt io_test_temp.txt
