//Copyright 2023 Niculici Mihai-Daniel (mihai.viper90@gmail.com)
#include "vma.h"
	/*
	   Folosim un string alocat dinamic pe care il vom folosi pe tot
	   parcursul programului pentru a stoca comenzile care intra de la stdin,
	   pentru ca mai apoi sa indentificam ce comanda este si sa o executam.
	*/
int main(void)
{
	arena_t *arena;
	uint64_t size, address;
	uint8_t *perm, *cont_perm, *data, space, endl;
	char *command;
	command = (char *)malloc(MAX_STRING_SIZE);
	DIE(!command, "command malloc() failed!\n");
	while (1) {
		scanf("%s", command);
		switch (my_hash(command)) {
		case ALLOC_ARENA:
			scanf("%ld", &size);
			arena = alloc_arena(size);
			break;
		case DEALLOC_ARENA:
			dealloc_arena(arena, command);
			if (arena)
				free(arena);
			exit(0);
			break;
		case ALLOC_BLOCK:
			scanf("%ld %ld", &address, &size);
			alloc_block(arena, address, size);
			break;
		case FREE_BLOCK:
			scanf("%ld", &address);
			free_block(arena, address);
			break;
		case READ:
			scanf("%ld %ld", &address, &size);
			read(arena, address, size);
			break;
		case WRITE:
			scanf("%ld %ld", &address, &size);
			data = malloc(size);
			DIE(!data, "data malloc() failed!\n");
			scanf("%c", &space);
			for (int i = 0; i < size; i++)
				scanf("%c", &data[i]);
			write(arena, address, size, data);
			free(data);
			break;
		case PMAP:
			pmap(arena);
			break;
		case MPROTECT:
			endl = '0';
			perm = malloc(MAX_PERM_LENGTH);
			DIE(!perm, "perm malloc() failed!\n");
			scanf("%ld %s", &address, perm);
			scanf("%c", &endl);
			while (endl != '\n') {
				if (endl != '\n') {
					strcat(perm, " | ");
					scanf("%c", &endl);
					scanf("%c", &endl);
				}
				cont_perm = malloc(MAX_PERM_LENGTH);
				DIE(!cont_perm, "perm malloc() failed!\n");
				scanf("%s", cont_perm);
				strcat(perm, cont_perm);
				free(cont_perm);
				scanf("%c", &endl);
			}
			mprotect(arena, address, perm);
			free(perm);
			break;
		default:
			printf("Invalid command. Please try again.\n");
			break;
		}
	}
	return 0;
}
