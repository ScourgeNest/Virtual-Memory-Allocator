//Copyright 2023 Niculici Mihai-Daniel (mihai.viper90@gmail.com)
#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Folosesc DIE pentru o implementare mai usoara a programarii defensive.
#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		   \
			perror(call_description);			\
			exit(errno);				         \
		}							\
	} while (0)
//Folosesc Acest MACRO pentru lungimea string-urilor.
#define MAX_STRING_SIZE 64

//Am creat cate un MACRO pentru fiecare dintre comenzi
//pentru a folosi switch.
#define INVALID_COMMAND 0
#define ALLOC_ARENA 1
#define DEALLOC_ARENA 2
#define ALLOC_BLOCK 3
#define FREE_BLOCK 4
#define READ 5
#define WRITE 6
#define PMAP 7
#define MPROTECT 8
#define MAX_PERM_LENGTH 36

//Am creat un MACRO pentru lungimea campului de permisiuni
//pentru ca permisiunea contine 3 caractere iar eu am pus ca
//PERM_SIZE sa fie 4 pentru a avea loc si de '\0'.
#define PERM_SIZE 4

//Structura unui nod dintr-o lista dublu inlantuita contine cei doi pointeri
//next si prev dar si informatia care poate fi absolut orice, prin urmare
//este o structura de nod generica.
typedef struct dll_node_t dll_node_t;
struct dll_node_t {
	void *data;
	dll_node_t *prev, *next;
};

/*
  Structura unei liste dublu inlantuite contine:
  1. Dimensiunea datelor pe care le vor avea nodurile listei
  2. Dimensiunea listei
  3. Un pointer care contine adresa primului nod din lista.
*/
typedef struct doubly_linked_list_t doubly_linked_list_t;
struct doubly_linked_list_t {
	void *head;
	unsigned int data_size;
	unsigned int size;
};

/*
  Structura unui block contine:
  1. Adresa de start a block-ului
  2. Dimensiunea block-ului
  3. O lista dublu inlantuita unde nodurile ei vor avea cate un miniblock
*/
typedef struct {
	uint64_t start_address;
	size_t size;
	void *miniblock_list;
} block_t;

/*
  Structura unui miniblock contine:
  1. Adresa de start a miniblock-ului
  2. Dimensiunea miniblock-ului
  3. Permisiunile miniblock-ului
  (1 - Permisiune de Executie
	2 - Permisiune de Scriere
	3 - Permisiune de Executie si Scriere
	4 - Permisiune de Citire
	5 - Permisiune de Citire si Executie
	6 - Permisiune de Citire si Scriere
	7 - Permisiune de Citire , Scriere si Executie)
  4. Un buffer in care vom scrie/citi.
*/
typedef struct {
	uint64_t start_address;
	size_t size;
	uint8_t  *perm;
	void *rw_buffer;
} miniblock_t;

/*
	Structura unei arene contine:
	1. Dimensiunea arenei
	2. Memoria folosita din totalul arenei
	3. O lista dublu inlantuita unde nodurile ei vor stoca cate un block
*/
typedef struct {
	uint64_t arena_size;
	uint64_t memory_used;
	doubly_linked_list_t *alloc_list;
} arena_t;

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena, char *command);

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void free_block(arena_t *arena, const uint64_t address);

void read(arena_t *arena, uint64_t address, uint64_t size);
void write(arena_t *arena, const uint64_t address,  const uint64_t size,
		   int8_t *data);

void pmap(const arena_t *arena);

void mprotect(arena_t *arena, uint64_t address, int8_t *permission);

unsigned int my_hash(char *command);

doubly_linked_list_t *dll_create(unsigned int data_size);

void free_memory_block(dll_node_t **node);

//alloc_block mini-functions
unsigned int if_list_of_blocks_is_empty(arena_t *arena, const uint64_t address,
										const uint64_t size);
unsigned int check_invalid_alloc_block_(arena_t *arena, const uint64_t address,
										const uint64_t size);
void verify_if_exists_adjacent_blocks(arena_t *arena, const uint64_t address,
									  const uint64_t size, int *caz);

void dll_add_node(arena_t *arena, const uint64_t address,
				  const uint64_t size, int caz);
void add_block_case_0(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz);
void add_block_case_1(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz);
void add_block_case_2(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz);
void add_block_case_3(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz);

dll_node_t *alloc_node(const uint64_t address, const uint64_t size);
dll_node_t *alloc_new_block(const uint64_t address, const uint64_t size);
dll_node_t *alloc_mini_node(const uint64_t address, const uint64_t size);

//Debug function
void debug_node(dll_node_t *node);

//free_block mini-functions
unsigned int check_invalid_free_block(arena_t *arena, const uint64_t address);
void check_case(arena_t *arena, const uint64_t address, int *caz);

void free_node(arena_t *arena, const uint64_t address, int caz);
void free_node_case_0(arena_t *arena, const uint64_t address, int caz);
void free_node_case_1(arena_t *arena, const uint64_t address, int caz);
void free_node_case_2(arena_t *arena, const uint64_t address, int caz);
void free_node_case_3(arena_t *arena, const uint64_t address, int caz);

//read-write mini-functions
unsigned int check_invalid_read(arena_t *arena, const uint64_t address);
unsigned int check_invalid_write(arena_t *arena, const uint64_t address);

//mprotect mini-functions
void check_perm(int8_t *permission, int *perm);
void set_perm(int perm, dll_node_t *mini_node);
unsigned int check_read(int8_t *perm);
unsigned int check_write(int8_t *perm);
unsigned int check_mprotect(arena_t *arena, const uint64_t address);

