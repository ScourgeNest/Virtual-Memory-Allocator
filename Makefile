##### Copyright 2023 Niculici Mihai-Daniel (mihai.viper90@gmail.com)
CC=gcc
CFLAGS=-I -Wall -Wextra -std=c99

build:
	gcc vma.c -c
	gcc vma.o main.c -o vma

run_vma:
	./vma -Wall -Wextra -std=c99

clean:
	rm -f vma.o vma