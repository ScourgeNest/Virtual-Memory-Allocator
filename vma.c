//Copyright 2023 Niculici Mihai-Daniel (mihai.viper90@gmail.com)
#include "vma.h"

//Functia aceasta creeaza arena si o initializeaza apoi o returneaza.
arena_t *alloc_arena(const uint64_t size)
{
	//Mai intai se intampla alocarea dinamica a arenei.
	arena_t *arena;
	arena = (arena_t *)malloc(sizeof(arena_t));
	DIE(!arena, "arena malloc() failed\n");

	//Apoi fiecare 'camp' al arenei este initializat.
	arena->arena_size = size;
	arena->memory_used = 0;
	arena->alloc_list = dll_create(sizeof(block_t));

	//Se returneaza pointer-ul arenei pentru a putea folosi arena
	//pe tot parcursul executiei programului.
	return arena;
}

//Funtia elibereaza memoria ocupata de arena, dar inainte de a face asta
//trebuie eliberata memoria ocupata de listele dublu inlantuite de
//miniblock-uri, apoi, lista dublu inlantuita de block-uri.
void dealloc_arena(arena_t *arena, char *command)
{
	//Verific daca exista arena.
	if (!arena) {
		printf("Alloc the Arena First!!\n");
		free(command);
		return;
	}

	//Verific daca arena este goala.
	if (!arena->alloc_list->head) {
		free(command);
		free(arena->alloc_list);
		return;
	}
	dll_node_t *curr = (dll_node_t *)arena->alloc_list->head;
	//Pentru fiecare bloc in parte, trebuie sa eliberez
	//memoria ocupata de lista dublu inlantuita a lui dar si de el insusi.
	while (curr) {
		//Folosesc doua noduri pentru a ma plimba prin lista si pentru a da
		//free la block-uri.
		dll_node_t *next = curr->next;
		dll_node_t *mini_node = ((doubly_linked_list_t *)(((block_t *)
		(curr->data))->miniblock_list))->head;
		while (mini_node) {
			//Aici eliberez memoria alocata de fiecare mini-nod din lista
			//block-ului.
			dll_node_t *mini_next = mini_node->next;
			free(((miniblock_t *)(mini_node->data))->perm);
			free(((miniblock_t *)(mini_node->data))->rw_buffer);
			free(mini_node->data);
			free(mini_node);
			mini_node = NULL;
			mini_node = mini_next;
		}
		//Dupa ce am eliberat lista de miniblock-uri, eliberez si block-ul dar
		//si nodul care continea block-ul respectiv.
		free(((doubly_linked_list_t *)(((block_t *)(curr->data))
		->miniblock_list)));
		free(curr->data);
		free(curr);
		curr = next;
	}
	//Eliberez memoria alocata de "campurile" arenei si de string-ul
	//cu ajutorul caruia citeam comenzile.
	arena->alloc_list->head = NULL;
	free(arena->alloc_list);
	free(command);
}

//Functia aloca un block in arena
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	//Verific daca block-ul care urmeaza sa fie alocat, va provoca erori
	if (check_invalid_alloc_block_(arena, address, size) == 0)
		return;
	//Verific daca lista de block-uri este goala
	if (if_list_of_blocks_is_empty(arena, address, size) == 1)
		return;
	//Verifica daca exista block-uri adiacente cu block-ul pe care vreau
	//sa il aloc.
	int caz = 0;
	verify_if_exists_adjacent_blocks(arena, address, size, &caz);
	dll_add_node(arena, address, size, caz);
}

//Functia aceasta este apelata atunci cand folosim comanda "FREE_BLOCK".
void free_block(arena_t *arena, const uint64_t address)
{
	//Mai intai verific daca adresa pe care vreau sa o eliberez exista
	//si nu vom provoca erori.
	if (check_invalid_free_block(arena, address) == 0) {
		printf("Invalid address for free.\n");
		return;
	}
	//Apoi ma folosesc de o variabila pe care o numesc caz pentru a
	//vedea in ce ma aflu.
	/*CAZURI:
		caz = 0 (block-ul pe care trebuie sa il eliberez, are doar un
		miniblock)

		caz = 1 (Eliberez un miniblock care se afla la sfarsitul unei liste de
		miniblock-uri)

		caz = 2 (Eliberez un miniblock care se afla la inceputul unei liste de
		miniblock-uri)

		caz = 3 (Eliberez un miniblock care se afla in mijlocul unei liste de
		miniblock-uri)
	*/
	int caz = 0;

	//Cu functia "check_case", vad in ce caz ma aflu
	check_case(arena, address, &caz);

	//Cu functia "free_node", eliberez adresa care trebuie in functie de caz
	free_node(arena, address, caz);
}

//Functia aceasta este apelata atunci cand folosim comanda "READ".
void read(arena_t *arena, uint64_t address, uint64_t size)
{
	//Verific daca adresa unde trebuie sa citesc este valida
	//si daca am permisiune pentru a face asta.
	if (check_invalid_read(arena, address) == 0) {
		printf("Invalid address for read.\n");
		return;
	} else if (check_invalid_read(arena, address) == 2) {
		printf("Invalid permissions for read.\n");
		return;
	}
	//Parcurg lista de block-uri pentru a vedea din care block trebuie sa
	//citesc.
	dll_node_t *node = arena->alloc_list->head;
	while (node) {
		if (((block_t *)node->data)->start_address <= address &&
			((block_t *)node->data)->start_address + ((block_t *)
			node->data)->size > address) {
			break;
		}
		node = node->next;
	}
	//Parcurg lista de miniblock-uri a block-ului unde trebuie sa citesc
	//pentru a gasi miniblock-ul de unde trebuie sa incep sa citesc.
	dll_node_t *mini_node = (((doubly_linked_list_t *)
	((block_t *)(node->data))->miniblock_list))->head;
	while (mini_node) {
		if (((miniblock_t *)mini_node->data)->start_address <= address &&
			((miniblock_t *)mini_node->data)->start_address +
			((miniblock_t *)mini_node->data)->size > address)
			break;
		mini_node = mini_node->next;
	}
	//Calculez marimea maxima a block-ului si adresa de start a block-ului.
	uint32_t max_length = ((block_t *)(node->data))->start_address +
	((block_t *)(node->data))->size;
	uint32_t start_address = ((miniblock_t *)(mini_node->data))->start_address;
	//Afisez eroare in cazul in care comanda vrea sa citesc mai mult decat
	//marimea block-ului.
	if (address + size > max_length) {
		printf("Warning: size was bigger than the block size. ");
		printf("Reading %ld characters.\n", max_length - address);
	}
	int check_size = size;
	//Numar cate caractere citesc cu ajutorul lui j in timp ce afisez caracter
	//cu caracter din miniblock-uri.
	for (int i = address, j = 0; i < max_length && j < check_size; i++, j++) {
		//Daca ajung la sfarsitul unui miniblock trec la urmatorul.
		if (((miniblock_t *)mini_node->data)->size == i - start_address) {
			mini_node = mini_node->next;
			start_address = ((miniblock_t *)(mini_node->data))->start_address;
		}
		//Daca citesc dintr-un loc de unde nu am scris, ma opresc.
		if (!((char *)((miniblock_t *)mini_node->data)
			->rw_buffer)[i - start_address])
			break;
		//Afisez caracterul.
		printf("%c", ((char *)((miniblock_t *)(mini_node->data))
			   ->rw_buffer)[i - start_address]);
	}
	printf("\n");
}

//Functia aceasta este apelata atunci cand folosim comanda "WRITE".
void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   int8_t *data)
{
	//Verific daca adresa trebuie sa scriu este valida si daca am permisiune.
	if (check_invalid_write(arena, address) == 0) {
		printf("Invalid address for write.\n");
		return;
	} else if (check_invalid_write(arena, address) == 2) {
		printf("Invalid permissions for write.\n");
		return;
	}
	//Parcurg lista pentru a afla in ce block trebuie sa scriu.
	dll_node_t *node = arena->alloc_list->head;
	while (node) {
		if (((block_t *)node->data)->start_address <= address &&
			((block_t *)node->data)->start_address + ((block_t *)
			node->data)->size > address) {
			break;
		}
		node = node->next;
	}
	//Parcurg lista de miniblock-uri a block-ului pentru a vedea in ce
	//miniblock trebuie sa incep sa scriu.
	dll_node_t *mini_node = (((doubly_linked_list_t *)((block_t *)
	(node->data))->miniblock_list))->head;
	while (mini_node) {
		if (((miniblock_t *)mini_node->data)->start_address <= address &&
			((miniblock_t *)mini_node->data)->start_address
			+ ((miniblock_t *)mini_node->data)->size > address)
			break;
		mini_node = mini_node->next;
	}
	//Calculez dimensiunea maxima a block-ului si adresa de start.
	uint32_t max_length = ((block_t *)(node->data))->start_address +
	((block_t *)(node->data))->size;
	uint32_t start_address = ((block_t *)(node->data))->start_address;
	//Daca trebuie sa scriu mai multe caractere decat are block-ul afisez
	//un warning.
	if (address + size > max_length) {
		printf("Warning: size was bigger than the block size.");
		printf(" Writing %ld characters.\n", max_length - address);
	}
	//Ma folosesc de variabila 'j' pentru a numara cate caractere trebuie
	//sa scriu in block.
	int i, j;
	for (i = address, j = 0; i < max_length && j < size; i++, j++) {
		//Daca ajung la capatul unui miniblock, trec la urmatorul.
		if (((miniblock_t *)mini_node->data)->size == i - start_address) {
			mini_node = mini_node->next;
			start_address = ((miniblock_t *)mini_node->data)->start_address;
		}
		//Scriu in 'rw_buffer'-ul miniblock-ului.
		((char *)((miniblock_t *)mini_node->data)
		->rw_buffer)[i - start_address] = data[j];
	}
}

//Functia aceasta afiseaza informatii despre arena, block-uri si miniblock-uri
void pmap(const arena_t *arena)
{
	if (arena) {
		if (arena->alloc_list) {
			//Afisez informatii despre arena.
			printf("Total memory: 0x%lX bytes\n", arena->arena_size);
			printf("Free memory: 0x%lX bytes\n", arena->arena_size
				   - arena->memory_used);
			printf("Number of allocated blocks: %d\n",
				   arena->alloc_list->size);

			//Parcurg lista pentru a calcula cate miniblock-uri au fost alocate
			dll_node_t *node = arena->alloc_list->head;
			int mini_blocks_allocated = 0;
			while (node) {
				mini_blocks_allocated += (((doubly_linked_list_t *)
				(((block_t *)(node->data))->miniblock_list))->size);
				node = node->next;
			}
			printf("Number of allocated miniblocks: %d\n",
				   mini_blocks_allocated);

			//Parcurg lista pentru a afisa informatii despre fiecare Block
			//si miniblock alocat.
			node = arena->alloc_list->head;
			int i = 1;
			while (node) {
				printf("\n");
				printf("Block %d begin\n", i);
				printf("Zone: 0x%lX - 0x%lX\n", ((block_t *)(node->data))
				->start_address,
				((block_t *)(node->data))->start_address + ((block_t *)
				(node->data))->size);
				dll_node_t *mini_node = ((dll_node_t *)
				(((doubly_linked_list_t *)(((block_t *)
				(node->data))->miniblock_list))->head));
				int j = 1;
				while (mini_node) {
					printf("Miniblock %d:\t\t0x%lX\t\t-\t\t0x%lX\t\t| %s\n",
						   j, ((miniblock_t *)(mini_node->data))->start_address,
						   ((miniblock_t *)(mini_node->data))->start_address +
						   ((miniblock_t *)(mini_node->data))->size,
						   ((miniblock_t *)(mini_node->data))->perm);
					mini_node = mini_node->next;
					j++;
				}
				printf("Block %d end\n", i);
				node = node->next;
				i++;
			}
		}
	}
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	//Verific daca adresa este valida.
	if (check_mprotect(arena, address) == 0) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	//Parcurg lista de block-uri pentru a vedea in ce block trebuie sa
	//modific permisiunile.
	dll_node_t *node = arena->alloc_list->head;
	while (node) {
		if (address >= ((block_t *)node->data)->start_address &&
			address < ((block_t *)node->data)->start_address +
			((block_t *)node->data)->size)
			break;
		node = node->next;
	}
	//Parcurg lista de miniblock-uri a block-ului pentru a vedea carui
	//miniblock trebuie sa ii modific permisiunile.
	dll_node_t *mini_node = ((doubly_linked_list_t *)
	(((block_t *)(node->data))->miniblock_list))->head;
	while (mini_node) {
		if (address >= ((miniblock_t *)mini_node->data)->start_address &&
			address < ((miniblock_t *)mini_node->data)->start_address +
			((miniblock_t *)mini_node->data)->size)
			break;
		mini_node = mini_node->next;
	}
	//Ma folosesc de o variabila perm pentru a interpreta permisiunile.
	int perm;
	//Verific ce permisiune trebuie sa atribui.
	check_perm(permission, &perm);
	//Setez permisiunea data.
	set_perm(perm, mini_node);
}

//Functia aceasta creeaza o lista dublu inlantuita si o initializeaza,
//dupa care o returneaza.
doubly_linked_list_t*
dll_create(unsigned int data_size)
{
	doubly_linked_list_t *list = malloc(sizeof(doubly_linked_list_t));
	DIE(!list, "list malloc() Failed!\n");

	list->data_size = data_size;
	list->size = 0;
	list->head = NULL;

	return list;
}

//Compar comanda pe care o primesc de la stdin cu fiecare din
//cele valide, daca nu corespunde, comanda este invalida.
unsigned int my_hash(char *string)
{
	if (strcmp(string, "ALLOC_ARENA") == 0)
		return ALLOC_ARENA;
	if (strcmp(string, "DEALLOC_ARENA") == 0)
		return DEALLOC_ARENA;
	if (strcmp(string, "ALLOC_BLOCK") == 0)
		return ALLOC_BLOCK;
	if (strcmp(string, "FREE_BLOCK") == 0)
		return FREE_BLOCK;
	if (strcmp(string, "READ") == 0)
		return READ;
	if (strcmp(string, "WRITE") == 0)
		return WRITE;
	if (strcmp(string, "PMAP") == 0)
		return PMAP;
	if (strcmp(string, "MPROTECT") == 0)
		return MPROTECT;
	return INVALID_COMMAND;
}

//Functia aceasta elibereaza memoria ocupata de un nod care contine un block.
void free_memory_block(dll_node_t **node)
{
	//Verific daca lista de miniblock-uri exista si daca are cel putin un
	//element.
	if (!((doubly_linked_list_t *)((((block_t *)((*node)->data)))
		->miniblock_list)) || !((doubly_linked_list_t *)((((block_t *)
		((*node)->data)))->miniblock_list))->head) {
		free(*node);
		return;
	}

	//Creez un pointer catre primul miniblock din lista.
	dll_node_t *curr = ((doubly_linked_list_t *)((((block_t *)
	((*node)->data)))->miniblock_list))->head;

	//Parcurg toata lista eliberand memoria fiecarui miniblock la fiecare pas.
	for (int i = 0; i < ((doubly_linked_list_t *)((((block_t *)
		 ((*node)->data)))->miniblock_list))->size; i++) {
		dll_node_t *next = curr->next;
		free(((miniblock_t *)(curr->data))->rw_buffer);
		((miniblock_t *)(curr->data))->rw_buffer = NULL;
		if (curr->next) {
			free(curr);
			break;
		}
		curr = next;
		curr->prev = NULL;
	}
	//Eliberez memoria ocupata de lista, apoi eliberez memoria ocupata de block
	free(((doubly_linked_list_t *)((block_t *)(*node)->data)->miniblock_list));
	free((doubly_linked_list_t *)(((block_t *)(*node)->data)));
}

unsigned int if_list_of_blocks_is_empty(arena_t *arena,
										const uint64_t address,
										const uint64_t size)
{
	if (!arena->alloc_list->head) {
		//Creez un nod pe care il aloc dinamic pentru a stoca informatia
		//si dupa ce ies din functia alloc_block.
		dll_node_t *node;
		node = (dll_node_t *)malloc(sizeof(dll_node_t));
		DIE(!node, "node malloc() failed\n");
		//Creez un block, care reprezinta un 'camp' din nod, pe care
		//il aloc dinamic.
		node->data = (block_t *)malloc(sizeof(block_t));
		DIE(!((block_t *)(node->data)), "block malloc() failed\n");
		//Aici atribui fiecare camp din nod, dar si din block.
		node->next = NULL;
		node->prev = NULL;
		((block_t *)(node->data))->start_address = address;
		((block_t *)(node->data))->size = size;
		//Aici creez lista de miniblock-uri.
		((block_t *)(node->data))->miniblock_list =
		dll_create(sizeof(miniblock_t));

		//Verific daca lista de miniblock-uri este goala. (chiar
		//daca ar trebui sa fie 100% goala)
		if (!((doubly_linked_list_t *)(((block_t *)node->data)
			->miniblock_list))->head) {
			//Creez un nod pe care il aloc dinamic si in care voi stoca
			//miniblock-ul pe care il voi adauga in lista.
			dll_node_t *mini_node;
			mini_node = (dll_node_t *)malloc(sizeof(dll_node_t));
			DIE(!mini_node, "mini_node malloc() failed\n");

			mini_node->data = malloc(sizeof(miniblock_t));
			DIE(!mini_node->data,
				"mini_node_data_field malloc() failed\n");

			mini_node->next = NULL;
			mini_node->prev = NULL;
			//Aloc si Initializez 'campul' perm al miniblock-ului.
			(((miniblock_t *)(mini_node->data))->perm) =
			(uint8_t *)malloc(PERM_SIZE);
			DIE(!((miniblock_t *)mini_node->data)->perm,
				"miniblock_permision_field malloc() failed\n");
			strcpy(((char *)(((miniblock_t *)mini_node->data)->perm)),
				   "RW-");
			//Aloc si Initializez 'campul' rw_buffer al miniblock-ului.
			((miniblock_t *)(mini_node->data))->rw_buffer = malloc(size);
			DIE(!((miniblock_t *)(mini_node->data))->rw_buffer,
				"miniblock_buffer_field malloc() failed\n");
			for (int i = 0; i < size; i++)
				((char *)(((miniblock_t *)(mini_node->data))
				->rw_buffer))[i] = 0;

			//Atribui Adresa de start si marimea miniblock-ului.
			((miniblock_t *)(mini_node->data))->size = size;
			((miniblock_t *)(mini_node->data))->start_address = address;
			//Setez head-ul listei de miniblock-uri si cresc numarul de
			//miniblock-uri cu 1.
			((doubly_linked_list_t *)(((block_t *)(node->data))
			->miniblock_list))->head = mini_node;
			((doubly_linked_list_t *)(((block_t *)(node->data))
			->miniblock_list))->size++;
		}
		arena->alloc_list->head = node;
		arena->alloc_list->size++;
		arena->memory_used += size;

		return 1;
	}
	return 0;
}

//Functia verifica daca alocarea block-ului poate avea loc fara sa apara erori.
unsigned int check_invalid_alloc_block_(arena_t *arena, const uint64_t address,
										const uint64_t size)
{
	//Verific daca adresa de start a block-ului este in afara arenei.
	if (address >= arena->arena_size) {
		printf("The allocated address is outside the size of arena\n");
		return 0;
	}
	//Verific daca adresa de sfarsit a block-ului este in afara arenei.
	if (address + size > arena->arena_size) {
		printf("The end address is past the size of the arena\n");
		return 0;
	}
	//Folosesc un nod pentru a ma plimba prin lista de block-uri pentru a vedea
	//daca locul unde vreau sa aloc memorie este ocupat sau nu.
	dll_node_t *node;
	node = arena->alloc_list->head;
	while (node) {
		//Verific daca adresa de start se afla in intervalul unui block deja
		//alocat pentru a afisa o eroare in cazul in care acest lucur se
		//intampla.
		if (address < ((block_t *)node->data)->start_address + ((block_t *)
			node->data)->size && address >= ((block_t *)node->data)
			->start_address) {
			printf("This zone was already allocated.\n");
			return 0;
		}
		//Verific daca adresa de final se afla in intervalul unui block deja
		//alocat pentru a afisa o eroare in cazul in care acest lucru se
		//intampla.
		if (address + size < ((block_t *)node->data)->start_address +
			((block_t *)node->data)->size &&
			address + size > ((block_t *)node->data)->start_address) {
			printf("This zone was already allocated.\n");
			return 0;
		}
		//Verific daca block-ul pe care urmeaza sa il aloc o sa "inghita" un
		//block deja alocat.
		if (address < ((block_t *)node->data)->start_address && address +
			size > ((block_t *)node->data)->start_address +
			((block_t *)node->data)->size) {
			printf("This zone was already allocated.\n");
			return 0;
		}
		node = node->next;
	}
	return 1;
}

void verify_if_exists_adjacent_blocks(arena_t *arena, const uint64_t address,
									  const uint64_t size, int *caz)
{
	/*CAZURI:
		caz = 0 (block-ul pe care trebuie sa il aloc, nu are niciun
		block adiacent)

		caz = 1 (block-ul pe care trebuie sa il aloc, continua un block
		care este deja alocat)

		caz = 2 (block-ul pe care trebuie sa il aloc, se termina unde
		incepe un block deja alocat)

		caz = 3 (cazul 1 si 2 combinat, adica block-ul pe care trebuie sa il
		aloc, se afla fix intre 2 block-uri deja alocate)
	*/

	if (arena->alloc_list->size >= 2) {
		//Creez 2 pointeri la noduri pentru a ma putea plimba prin lista.
		dll_node_t *curr, *next;
		curr = arena->alloc_list->head;
		next = curr->next;

		//Parcurg toata lista si verific daca block-urile deja alocate sunt
		//adiacente cu block-ul pe care vreau sa il aloc.
		while (curr->next) {
			//Verific daca block-ul pe care vreau sa il aloc se afla exact
			//intre 2 block-uri deja alocate.
			if (address == ((block_t *)curr->data)->start_address +
				((block_t *)curr->data)->size &&
			    ((block_t *)next->data)->start_address == address + size) {
				*caz = 3;
				return;
			}
			//Verific daca block-ul pe care vreau sa il aloc se afla in
			//continuarea unui block deja alocat.
			if (address == ((block_t *)curr->data)->start_address +
				((block_t *)curr->data)->size) {
				*caz = 1;
				return;
			}
			//Verific daca block-ul pe care vreau sa il aloc se afla se
			//termina la inceputul unui alt block deja alocat.
			if (((block_t *)curr->data)->start_address == address + size) {
				*caz = 2;
				return;
			}
			curr = next;
			next = curr->next;
		}
		if (address == ((block_t *)curr->data)->start_address +
			((block_t *)curr->data)->size) {
			*caz = 1;
			return;
		}
		//Verific daca block-ul pe care vreau sa il aloc se afla se termina
		//la inceputul unui alt block deja alocat.
		if (((block_t *)curr->data)->start_address == address + size) {
			*caz = 2;
			return;
		}
	} else if (arena->alloc_list->head) {
		//Daca am doar un singur element in lista atunci verific daca sa pun
		//block-ul inainte sau dupa block-ul deja alocat.
		dll_node_t *node = arena->alloc_list->head;
		if (address == ((block_t *)node->data)->start_address +
		   ((block_t *)node->data)->size) {
			*caz = 1;
			return;
		}
		if (((block_t *)node->data)->start_address == address + size) {
			*caz = 2;
			return;
		}
	}
	*caz = 0;
}

//Ma folosesc de aceasta functie pentru a adauga noul block unde trebuie,
//dupa caz
void dll_add_node(arena_t *arena, const uint64_t address,
				  const uint64_t size, int caz)
{
	add_block_case_0(arena, address, size, caz);
	add_block_case_1(arena, address, size, caz);
	add_block_case_2(arena, address, size, caz);
	add_block_case_3(arena, address, size, caz);
}

//Aceasta functie aloca block-ul pentru cazul 0
void add_block_case_0(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz)
{
	// caz = 0 (block-ul pe care trebuie sa il aloc, nu are niciun block
	//adiacent)
	if (caz == 0) {
		//Daca am un singur block alocat in lista de block-uri verific daca
		//trebuie sa-l pun inainte sau dupa block-ul deja alocat
		if (arena->alloc_list->size == 1) {
			dll_node_t *node = alloc_node(address, size);
			if (((block_t *)(((dll_node_t *)arena->alloc_list->head)->data))
			    ->start_address > address + size) {
				node->next = arena->alloc_list->head;
				arena->alloc_list->head = node;
				if (node->next)
					node->next->prev = node;
			} else {
				node->prev = arena->alloc_list->head;
				node->next = ((dll_node_t *)arena->alloc_list->head)->next;
				((dll_node_t *)arena->alloc_list->head)->next = node;
				if (node->next)
					node->next->prev = node;
			}
		} else if (arena->alloc_list->size >= 2) {
			//Daca avem mai mult de 2 block-uri alocate in lista de block-uri,
			//Caut nodul care ar trebui sa in fata block-ului pe care urmeaza
			//sa-l aloc.
			dll_node_t *aux = arena->alloc_list->head;
			while (aux->next) {
				if (((block_t *)aux->data)->start_address > address + size)
					break;
				aux = aux->next;
			}
			//Daca acest nod exista, fac legaturile intre noduri.
			if (((block_t *)aux->data)->start_address > address + size) {
				dll_node_t *node = alloc_node(address, size);
				node->next = aux;
				node->prev = aux->prev;
				aux->prev = node;
				if (node->prev)
					node->prev->next = node;
				if (arena->alloc_list->head == aux)
					arena->alloc_list->head = node;
			} else {
				//Daca nu exista, inseamna ca m-am oprit la nod-ul care va fi
				//in spatele block-ului pe care urmeaza sa-l aloc si fac
				//legaturile.
				dll_node_t *node = alloc_node(address, size);
				node->next = aux->next;
				node->prev = aux;
				aux->next = node;
				if (node->next)
					node->next->prev = node;
			}
		}
		//Cresc memoria folosita de arena, dar si numarul de noduri din lista.
		arena->memory_used += size;
		arena->alloc_list->size++;
	}
}

//Aceasta functie aloca block-ul pentru cazul 1
void add_block_case_1(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz)
{
	//caz = 1 (block-ul pe care trebuie sa il aloc, continua un block care
	//este deja alocat)
	if (caz == 1) {
		//Aloc un mininod, dupa care caut in ce lista de miniblock-uri trebuie
		//alocat verificand fiecare block din lista.
		dll_node_t *mini_node = alloc_mini_node(address, size);
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			if (((block_t *)node->data)->start_address +
			    ((block_t *)node->data)->size == address) {
				break;
			}
			node = node->next;
		}
		//Cresc marimea block-ului.
		((block_t *)(node->data))->size += size;
		//Cresc numarul de miniblock-uri din lista.
		((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->size++;

		//Merg pana la sfarsitul listei de miniblock-uri si ma opresc la
		//ultimul miniblock.
		node = ((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head;
		while (node->next)
			node = node->next;

		//Aici fac legaturile intre nod-uri.
		mini_node->next = node->next;
		mini_node->prev = node;
		node->next = mini_node;
		//Cresc memoria folosita de block-uri in arena.
		arena->memory_used += size;
	}
}

//Aceasta functie aloca block-ul pentru cazul 2
void add_block_case_2(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz)
{
	//caz = 2 (block-ul pe care trebuie sa il aloc, se termina unde incepe
	//un block deja alocat)
	if (caz == 2) {
		//Creez un mininod, dupa care caut in ce lista de miniblock-uri
		//trebuie sa il adaug.
		dll_node_t *mini_node = alloc_mini_node(address, size);
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			if (address + size == ((block_t *)node->data)->start_address)
				break;
			node = node->next;
		}
		//Cresc marimea block-ului
		((block_t *)(node->data))->size += size;
		//Cresc numarul de miniblock-uri din lista.
		((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
		->size++;

		//Cum miniblock-ul pe care trebuie sa-l aloc se afla la inceputul
		//listei, doar fac legaturile cu celelalte noduri si modific
		//head-ul listei de miniblock-uri.
		dll_node_t *head;
		head = ((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head;
		mini_node->prev = head->prev;
		mini_node->next = head;
		head->prev = mini_node;
		//Cresc memoria folosita de block-uri in arena.
		arena->memory_used += size;
		((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
		->head = mini_node;

		//Schimb adresa de start a block-ului pentru ca am adaugat un miniblock
		//la inceputul listei de miniblock-uri
		((block_t *)(node->data))->start_address = address;
	}
}

//Aceasta functie aloca block-ul pentru cazul 3
void add_block_case_3(arena_t *arena, const uint64_t address,
					  const uint64_t size, int caz)
{
	//caz = 3 (cazul 1 si 2 combinat, adica block-ul pe care trebuie sa il
	// aloc, se afla fix intre 2 block-uri deja alocate)
	if (caz == 3) {
		//Creez mininod-ul pe care urmeaza sa-l alloc.
		dll_node_t *mini_node = alloc_mini_node(address, size);
		dll_node_t *node = arena->alloc_list->head;
		//Caut nod-ul care contine block-ul care se va face mai mare
		//pentru ca vom unii acest block cu vecinul din fata lui datorita
		//miniblock-ului pe care il vom aloca
		while (node) {
			if (((block_t *)node->data)->start_address + ((block_t *)
			    node->data)->size == address) {
				break;
			}
			node = node->next;
		}
		// Ma duc la sfarsitul listei de miniblock-uri ale primului block
		// pentru a adauga miniblock-ul la sfarsitul listei.
		dll_node_t *next = node->next;
		dll_node_t *aux_mininode = ((doubly_linked_list_t *)(((block_t *)
		(node->data))->miniblock_list))->head;
		while (aux_mininode->next)
			aux_mininode = aux_mininode->next;

		mini_node->next = aux_mininode->next;
		mini_node->prev = aux_mininode;
		aux_mininode->next = mini_node;
		//Cresc dimensiunea block-ului pentru ca am adaugat miniblock-ul.
		((block_t *)(node->data))->size += size;

		//Cresc numarul de miniblock-uri din lista.
		((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
		->size++;

		//Adun dimnesiunile celor doua block-uri care se vor unii.
		((block_t *)(node->data))->size += ((block_t *)(next->data))->size;

		//Adun numarul de miniblock-uri ale celor doua block-uri.
		((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
		->size += ((doubly_linked_list_t *)(((block_t *)(next->data))
		->miniblock_list))->size;

		//Apoi fac legaturile intre miniblock-urile primului block si
		//miniblock-urile celui de-al doilea block.
		aux_mininode = ((doubly_linked_list_t *)(((block_t *)
		(next->data))->miniblock_list))->head;
		mini_node->next = aux_mininode;
		aux_mininode->prev = mini_node;
		node->next = next->next;
		if (next->next)
			node->next->prev = node;

		//Apoi eliberez memoria ocupata de cel de-al doilea block.
		free(((doubly_linked_list_t *)(((block_t *)
		(next->data))->miniblock_list)));
		free(next->data);
		free(next);
		//Cresc memoria ocupata de block-uri in arena
		arena->memory_used += size;
		//Scad numarul de block-uri din arena pentru ca am unit 2 block-uri.
		arena->alloc_list->size--;
	}
}

//Functia aceasta aloca un nod si il returneaza.
dll_node_t *alloc_node(const uint64_t address, const uint64_t size)
{
	//Creez un nod pe care il aloc dinamic pentru a stoca informatia
	//si dupa ce ies din functia alloc_node.
	dll_node_t *node;
	node = (dll_node_t *)malloc(sizeof(dll_node_t));
	DIE(!node, "node malloc() failed\n");

	//Creez un block, care reprezinta un 'camp' din nod, pe care
	//il aloc dinamic.
	node->data = (block_t *)malloc(sizeof(block_t));
	DIE(((block_t *)(node->data)) == NULL, "block malloc() failed\n");

	//Aici atribui fiecare camp din nod, dar si din block.
	node->next = NULL;
	node->prev = NULL;
	((block_t *)(node->data))->start_address = address;
	((block_t *)(node->data))->size = size;

	//Aici creez lista de miniblock-uri.
	((block_t *)(node->data))->miniblock_list =
	dll_create(sizeof(miniblock_t));

	//Aici creez mini_node-ul
	dll_node_t *mini_node;
	mini_node = (dll_node_t *)malloc(sizeof(dll_node_t));
	DIE(!mini_node, "mini_node malloc() failed\n");

	mini_node->next = NULL;
	mini_node->prev = NULL;

	mini_node->data = malloc(sizeof(miniblock_t));
	DIE(!mini_node->data, "mini_node_data_field malloc() failed\n");

	//Aloc si Initializez 'campul' perm al miniblock-ului.
	(((miniblock_t *)(mini_node->data))->perm) = (uint8_t *)malloc(PERM_SIZE);
	DIE(!((miniblock_t *)(mini_node->data))->perm,
		"miniblock_permision_field malloc() failed\n");
	strcpy(((miniblock_t *)(mini_node->data))->perm, "RW-");

	//Aloc si Initializez 'campul' rw_buffer al miniblock-ului.
	((miniblock_t *)(mini_node->data))->rw_buffer = (char *)malloc(size);
	DIE(!((miniblock_t *)(mini_node->data))->rw_buffer,
		"miniblock_buffer_field malloc() failed\n");
	for (int i = 0; i < size; i++)
		((char *)(((miniblock_t *)(mini_node->data))->rw_buffer))[i] = 0;

	//Atribui Adresa de start si marimea miniblock-ului.
	((miniblock_t *)(mini_node->data))->size = size;
	((miniblock_t *)(mini_node->data))->start_address = address;

	((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
	->head = mini_node;
	((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
	->size++;

	return node;
}

//Functia aloca un nod care contine un block, fara a initializa lista de
//miniblock-uri sau fara a adauga miniblock-uri in lista.
dll_node_t *alloc_new_block(const uint64_t address, const uint64_t size)
{
	//Creez nodul.
	dll_node_t *new_block = malloc(sizeof(dll_node_t));
	DIE(!new_block, "new_block malloc() failed!\n");
	//Creez block-ul
	new_block->data = malloc(sizeof(block_t));
	DIE(!new_block->data, "new_block_data_field malloc() failed!\n");
	//Atribui 'campurile' block-ului.
	((block_t *)(new_block->data))->start_address = address;
	((block_t *)(new_block->data))->size = size;
	((block_t *)(new_block->data))->miniblock_list =
	dll_create(sizeof(miniblock_t));
	//Functia returneaza block-ul alocat.
	return new_block;
}

//Folosesc aceasta functie pentru a aloca un mininod.
dll_node_t *alloc_mini_node(const uint64_t address, const uint64_t size)
{
	dll_node_t *mini_node;
	mini_node = (dll_node_t *)malloc(sizeof(dll_node_t));
	DIE(!mini_node, "mini_node malloc() failed\n");

	mini_node->next = NULL;
	mini_node->prev = NULL;

	mini_node->data = malloc(sizeof(miniblock_t));
	DIE(!mini_node->data, "mini_node_data_field malloc() failed\n");

	//Aloc si Initializez 'campul' perm al miniblock-ului.
	(((miniblock_t *)(mini_node->data))->perm) = (uint8_t *)malloc(PERM_SIZE);
	DIE(!((miniblock_t *)(mini_node->data))->perm,
		"miniblock_permision_field malloc() failed\n");
	strcpy(((miniblock_t *)(mini_node->data))->perm, "RW-");

	//Aloc si Initializez 'campul' rw_buffer al miniblock-ului.
	((miniblock_t *)(mini_node->data))->rw_buffer = (char *)malloc(size);
	DIE(!((miniblock_t *)(mini_node->data))->rw_buffer,
		"miniblock_buffer_field malloc() failed\n");
	for (int i = 0; i < size; i++)
		((char *)(((miniblock_t *)(mini_node->data))->rw_buffer))[i] = 0;
	//Atribui Adresa de start si marimea miniblock-ului.
	((miniblock_t *)(mini_node->data))->size = size;
	((miniblock_t *)(mini_node->data))->start_address = address;

	return mini_node;
}

//Folosesc aceasta functie pentru a face debug.
//In aceasta functie afisez absolut toate adresele care ma pot interesa la
//un nod atunci cand fac debug.
void debug_node(dll_node_t *node)
{
	printf("Node_address: %p\n", node);
	printf("Node_next: %p\n", node->next);
	printf("Node_prev: %p\n", node->prev);
	printf("Node_Data: %p\n", node->data);

	printf("\nBlock Info Incomming:\n");

	printf("Node_Block: %p\n", ((block_t *)(node->data)));
	printf("Node_Block_miniblock_list: %p\n", ((block_t *)(node->data))
	->miniblock_list);
	printf("Node_Block_size: %ld\n", ((block_t *)(node->data))->size);
	printf("Node_Block_start_address: %ld\n", ((block_t *)(node->data))
	->start_address);
	printf("Node_Block_end_address: %ld\n", ((block_t *)(node->data))
	->start_address + ((block_t *)(node->data))->size);

	printf("\nMini_Block Info Incomming:\n");

	printf("Node_mini_block_list: %p\n", ((doubly_linked_list_t *)
	(((block_t *)(node->data))->miniblock_list)));
	printf("Node_mini_block_list_size: %d\n", ((doubly_linked_list_t *)
	(((block_t *)(node->data))->miniblock_list))->size);
	printf("Node_mini_block_data_size: %d\n", ((doubly_linked_list_t *)
	(((block_t *)(node->data))->miniblock_list))->data_size);

	printf("\n");
	dll_node_t *mini_node = ((dll_node_t *)(((doubly_linked_list_t *)
	(((block_t *)(node->data))->miniblock_list))->head));
	int i = 1;
	while (mini_node) {
		printf("Node_mini_block_node_%d: %p\n", i, mini_node);
		printf("Node_mini_block_node_%d_next: %p\n", i, mini_node->next);
		printf("Node_mini_block_node_%d_prev: %p\n", i, mini_node->prev);
		printf("Node_mini_block_node_%d_size: %ld\n", i, ((miniblock_t *)
			   (mini_node->data))->size);
		printf("Node_mini_block_node_%d_start_address: %ld\n", i,
			   ((miniblock_t *)(mini_node->data))->start_address);
		printf("Node_mini_block_node_%d_perm: %s\n", i, ((miniblock_t *)
			   (mini_node->data))->perm);
		printf("Node_mini_block_node_%d_rw_buffer: (%s)\n", i, ((char *)
			   (((miniblock_t *)(mini_node->data))->rw_buffer)));
		printf("\n");
		mini_node = mini_node->next;
		i++;
	}
	printf("\n\n\n");
}

//Funtia aceasta verifica daca adresa pe care vreau sa o eliberez
//exista sau daca va provoca erori.
unsigned int check_invalid_free_block(arena_t *arena, const uint64_t address)
{
	//In primul rand verific daca exista block-uri in arena,
	//pentru ca nu pot sa eliberez o adresa care nu a fost alocata.
	if (arena->alloc_list->size == 0)
		return 0;
	//Caut in lista de miniblock-uri a fiecarul block daca adresa a fost
	//alocata sau nu.
	dll_node_t *node = arena->alloc_list->head;
	while (node) {
		dll_node_t *mini_node;
		mini_node = ((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head;
		while (mini_node) {
			if (((miniblock_t *)mini_node->data)->start_address == address)
				return 1;
			mini_node = mini_node->next;
		}
		node = node->next;
	}
	//In cazul in care nu gasesc un miniblock adresa data, inseamna
	//ca adresa pe care trebuie sa o eliberez nu este o adresa de start a
	//unui miniblock, prin urmare comanda este invalida.
	return 0;
}

//Functia aceasta imi returneaza prin intermediul variabilei "caz",
//cazul in care ma aflu atunci cand vine vorba de eliberat memoria unui block.
void check_case(arena_t *arena, const uint64_t address, int *caz)
{
	//Ma plimb prin lista de miniblock-uri a fiecarui block si caut
	//miniblock-ul care are adresa de start egala cu adresa data.
	//Dupa care, verific sa vad in ce caz ma aflu.
	dll_node_t *node = arena->alloc_list->head;
	while (node) {
		dll_node_t *mini_node = ((doubly_linked_list_t *)(((block_t *)
		(node->data))->miniblock_list))->head;
		while (mini_node) {
			if (((miniblock_t *)mini_node->data)->start_address == address) {
				if (!mini_node->prev && !mini_node->next)
				//Daca miniblock-ul nu are vecini, in seamna ca exista doar
				//un miniblock in lista.
					*caz = 0;
				else if (!mini_node->next)
				//Daca miniblock-ul nu are pe nimeni in fata, inseamna ca se
				//afla la sfarsitul listei de miniblock-uri
					*caz = 1;
				else if (!mini_node->prev)
				//Daca miniblock-ul nu are pe nimeni in spate, inseamna ca se
				//afla la inceputul listei de miniblock-uri
					*caz = 2;
				else
				//Daca are vecini in ambele parti, inseamna ca acesta se afla
				//la mijlocul listei de miniblock-uri
					*caz = 3;
			}
			mini_node = mini_node->next;
		}
		node = node->next;
	}
}

//Functia aceasta elibereaza block-ul in functie de caz
void free_node(arena_t *arena, const uint64_t address, int caz)
{
	free_node_case_0(arena, address, caz);
	free_node_case_1(arena, address, caz);
	free_node_case_2(arena, address, caz);
	free_node_case_3(arena, address, caz);
}

//Functia aceasta elibereaza block-ul daca ne aflam in cazul 0.
void free_node_case_0(arena_t *arena, const uint64_t address, int caz)
{
	//caz = 0 (block-ul pe care trebuie sa il eliberez, are doar un miniblock)
	if (caz == 0) {
		//Caut block-ul pe care trebuie sa il eliberez.
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			if (((block_t *)node->data)->start_address == address)
				break;
			node = node->next;
		}
		//Verific daca block-ul pe care o sa-l eliberez este primul din lista,
		//caz in care schimb "head"-ul listei de block-uri.
		dll_node_t *next = node->next;
		if (node == arena->alloc_list->head)
			arena->alloc_list->head = next;

		//Scad memoria alocata de block-uri in arena.
		arena->memory_used -= ((miniblock_t *)(((dll_node_t *)
		(((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head))->data))->size;

		if (node->next)
			node->next->prev = node->prev;
		if (node->prev)
			node->prev->next = node->next;

		//Eliberez fiecare absolut toata memoria alocata de block.
		free(((miniblock_t *)(((dll_node_t *)(((doubly_linked_list_t *)
		(((block_t *)(node->data))->miniblock_list))->head))->data))->perm);

		((miniblock_t *)(((dll_node_t *)(((doubly_linked_list_t *)
		(((block_t *)(node->data))->miniblock_list))->head))->data))
		->perm = NULL;

		free(((miniblock_t *)(((dll_node_t *)(((doubly_linked_list_t *)
		(((block_t *)(node->data))->miniblock_list))->head))->data))
		->rw_buffer);

		((miniblock_t *)(((dll_node_t *)(((doubly_linked_list_t *)
		(((block_t *)(node->data))->miniblock_list))->head))->data))
		->rw_buffer = NULL;

		free(((miniblock_t *)(((dll_node_t *)(((doubly_linked_list_t *)
		(((block_t *)(node->data))->miniblock_list))->head))->data)));

		((dll_node_t *)(((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head))->data = NULL;

		free((dll_node_t *)(((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head));

		((doubly_linked_list_t *)(((block_t *)(node->data))->miniblock_list))
		->head = NULL;

		free(((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list)));

		((block_t *)(node->data))->miniblock_list = NULL;

		free((block_t *)(node->data));
		node->data = NULL;

		free(node);
		node = NULL;

		//Scad numarul block-uri din arena.
		arena->alloc_list->size--;
		//Daca in arena nu mai avem arena, setam "head"-ul listei la NULL,
		//pentru a nu avea erori atunci cand incercam sa afisam cu "PMAP".
		if (arena->alloc_list->size == 0)
			arena->alloc_list->head = NULL;
	}
}

//Functia aceasta elibereaza block-ul daca ne aflam in cazul 1.
void free_node_case_1(arena_t *arena, const uint64_t address, int caz)
{
	//caz = 1 (Eliberez un miniblock care se afla la sfarsitul unei liste de
	//miniblock-uri)

	//Caut la sfarsitul fiecarei liste de miniblock-uri al fiecarui block
	//adresa pe care vreau sa o eliberez.
	if (caz == 1) {
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			dll_node_t *mini_node = ((doubly_linked_list_t *)(((block_t *)
			(node->data))->miniblock_list))->head;

			while (mini_node->next)
				mini_node = mini_node->next;

			if (((miniblock_t *)mini_node->data)->start_address == address) {
				//Scad memoria ocupata de miniblock in arena.
				arena->memory_used -= ((miniblock_t *)mini_node->data)->size;

				mini_node->prev->next = NULL;

				//Scad marimea block-ului.
				((block_t *)node->data)->size -= ((miniblock_t *)
				mini_node->data)->size;

				//Scad numarul de miniblock-uri din lista.
				((doubly_linked_list_t *)(((block_t *)node->data)
				->miniblock_list))->size--;

				//Eliberez tot miniblock-ul.
				free(((miniblock_t *)mini_node->data)->perm);
				((miniblock_t *)mini_node->data)->perm = NULL;

				free(((miniblock_t *)mini_node->data)->rw_buffer);
				((miniblock_t *)mini_node->data)->rw_buffer = NULL;

				free(mini_node->data);
				mini_node->data = NULL;

				free(mini_node);
				mini_node = NULL;
			}
			node = node->next;
		}
	}
}

//Functia aceasta elibereaza block-ul daca ne aflam in cazul 2.
void free_node_case_2(arena_t *arena, const uint64_t address, int caz)
{
	//caz = 2 (Eliberez un miniblock care se afla la inceputul unei liste de
	//miniblock-uri)
	if (caz == 2) {
		dll_node_t *node = arena->alloc_list->head;
		//Caut miniblock-ul pe care trebuie sa-l eliberez in listele
		//de miniblock-uri ale block-urilor.
		while (node) {
			if (((miniblock_t *)(((dll_node_t *)(((doubly_linked_list_t *)
			    (((block_t *)node->data)->miniblock_list))->head))->data))->
			    start_address == address)
				break;
			node = node->next;
		}

		//Tin minte primele doua miniblock-uri din lista.
		dll_node_t *mini_node = ((dll_node_t *)(((doubly_linked_list_t *)
		(((block_t *)node->data)->miniblock_list))->head));
		dll_node_t *next = mini_node->next;

		//Scad memoria ocupata de miniblock in arena.
		arena->memory_used -= ((miniblock_t *)(((dll_node_t *)
		(((doubly_linked_list_t *)(((block_t *)node->data)->miniblock_list))
		->head))->data))->size;

		//Scad marimea block-ului.
		((block_t *)(node->data))->size -= ((miniblock_t *)(((dll_node_t *)
		(((doubly_linked_list_t *)(((block_t *)node->data)->miniblock_list))
		->head))->data))->size;

		//Scad numarul de miniblock-uri din lista.
		((doubly_linked_list_t *)(((block_t *)node->data)->miniblock_list))
		->size--;

		//Eliberez tot miniblock-ul.
		free(((miniblock_t *)(mini_node->data))->perm);
		((miniblock_t *)(mini_node->data))->perm = NULL;

		free(((miniblock_t *)mini_node->data)->rw_buffer);
		((miniblock_t *)mini_node->data)->rw_buffer = NULL;

		free(mini_node->data);
		mini_node->data = NULL;

		free(mini_node);
		mini_node = NULL;

		//Setez noul "head" al listei de miniblock-uri.
		((doubly_linked_list_t *)(((block_t *)node->data)
		->miniblock_list))->head = next;

		//Setez noua adresa de start a block-ului.
		((block_t *)node->data)->start_address =
		((miniblock_t *)next->data)->start_address;

		//Dupa ce am eliberat primul nod, pun NULL in spatele noului "head"
		//al listei de miniblock-uri.
		next->prev = NULL;
	}
}

//Functia aceasta elibereaza block-ul daca ne aflam in cazul 3.
void free_node_case_3(arena_t *arena, const uint64_t address, int caz)
{
	//caz = 3 (Eliberez un miniblock care se afla in mijlocul unei liste de
	//miniblock-uri)
	if (caz == 3) {
		//Caut miniblock-ul in lista de miniblock-uri din lista fiecarui block.
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			int pos = 0;
			dll_node_t *mini_node = ((doubly_linked_list_t *)(((block_t *)
			(node->data))->miniblock_list))->head;
			while (mini_node->next) {
				if (((miniblock_t *)mini_node->data)->start_address
				     == address)
					break;
				mini_node = mini_node->next;
				pos++;
			}
			if (((miniblock_t *)mini_node->data)->start_address == address) {
				//Tin minte vecinii miniblock-ului.
				dll_node_t *prev = mini_node->prev;
				dll_node_t *next = mini_node->next;
				//Calculez dimensiunea block-ului care se va forma atunci cand
				//eliberez memoria miniblock-ului.
				uint32_t block_size = 0;
				while (next) {
					block_size += ((miniblock_t *)next->data)->size;
					next = next->next;
				}
				next = mini_node->next;
				//Scad dimensiunea block-ului.
				((block_t *)node->data)->size -= block_size;
				((block_t *)node->data)->size -= ((miniblock_t *)
				mini_node->data)->size;
				//Scad memoria alocata de block-uri in arena.
				arena->memory_used -= ((miniblock_t *)mini_node->data)->size;
				//Cresc numarul de block-uri din arena.
				arena->alloc_list->size++;
				//Aloc noul block.
				dll_node_t *new_block = alloc_new_block(((miniblock_t *)
				next->data)->start_address, block_size);
				//Setez campurile listelor de miniblock-uri
				((doubly_linked_list_t *)(((block_t *)new_block->data)
				->miniblock_list))->head = next;
				((doubly_linked_list_t *)(((block_t *)new_block->data)
				->miniblock_list))->size =
				((doubly_linked_list_t *)(((block_t *)node->data)
				->miniblock_list))->size - pos - 1;
				((doubly_linked_list_t *)(((block_t *)node->data)
				->miniblock_list))->size = pos;
				//Rup legaturile miniblock-ului cu vecinii lui.
				next->prev = NULL;
				prev->next = NULL;
				mini_node->next = NULL;
				mini_node->prev = NULL;

				//Eliberez tot miniblock-ul.
				free(((miniblock_t *)mini_node->data)->perm);
				((miniblock_t *)mini_node->data)->perm = NULL;

				free(((miniblock_t *)mini_node->data)->rw_buffer);
				((miniblock_t *)mini_node->data)->rw_buffer = NULL;

				free(mini_node->data);
				mini_node->data = NULL;

				free(mini_node);
				mini_node = NULL;

				new_block->next = node->next;
				node->next = new_block;
				new_block->prev = node;
				if (new_block->next)
					new_block->next->prev = new_block;
			}
			node = node->next;
		}
	}
}

//Functia aceasta verifica daca adresa daca este valida si daca am permisiune
//pentru a citi.
unsigned int check_invalid_read(arena_t *arena, const uint64_t address)
{
	/*
		ok = 0 (Adresa este invalida)

		ok = 1 (Adresa este valida si am si permisiune)

		ok = 2 (Adresa este valida, dar nu am permisiune)
	*/

	//Verific daca exista arena si daca exista cel putin un block in ea.
	if (arena && arena->alloc_list->head) {
		//Parcurg lista de block-uri pentru a vedea daca in acesta vreau
		//sa citesc, daca da, verific permisiunile fiecarui miniblock.
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			if (((block_t *)node->data)->start_address <= address &&
			    ((block_t *)node->data)->start_address + ((block_t *)
			    node->data)->size > address) {
				int ok = 1;
				dll_node_t *mini_node = ((doubly_linked_list_t *)(((block_t *)
				node->data)->miniblock_list))->head;
				while (mini_node) {
					if (check_read(((miniblock_t *)mini_node->data)->perm) == 0)
						ok = 2;
					mini_node = mini_node->next;
				}
				if (ok == 1)
					return 1;
				if (ok == 2)
					return 2;
			}
			node = node->next;
		}
	}
	return 0;
}

//Functia aceasta verifica daca adresa daca este valida si daca am permisiune
//pentru a scrie.
unsigned int check_invalid_write(arena_t *arena, const uint64_t address)
{
	/*
		ok = 0 (Adresa este invalida)

		ok = 1 (Adresa este valida si am si permisiune)

		ok = 2 (Adresa este valida, dar nu am permisiune)
	*/

	//Verific daca exista arena si daca exista cel putin un block in ea.
	if (arena && arena->alloc_list->head) {
		//Parcurg lista de block-uri pentru a vedea daca in acesta vreau
		//sa scriu, daca da, verific permisiunile fiecarui miniblock.
		dll_node_t *node = arena->alloc_list->head;
		while (node) {
			if (((block_t *)node->data)->start_address <= address &&
			    ((block_t *)node->data)->start_address + ((block_t *)
			    node->data)->size > address) {
				int ok = 1;
				dll_node_t *mini_node = ((doubly_linked_list_t *)
				(((block_t *)node->data)->miniblock_list))->head;
				while (mini_node) {
					if (check_write(((miniblock_t *)mini_node->data)->
						perm) == 0)
						ok = 2;
					mini_node = mini_node->next;
				}
				if (ok == 1)
					return 1;
				if (ok == 2)
					return 2;
			}
			node = node->next;
		}
	}
	return 0;
}

//Functia aceasta verifica ce permisiune trebuie sa atribui miniblock-ului.
void check_perm(int8_t *permission, int *perm)
{
	//Permisiunea este setata la fel ca intr-un sistem de operare Linux.
	//Incerc toate combinatiile posibile pentru a obtine permisiunea corecta.
	if (strcmp(permission, "PROT_READ | PROT_WRITE | PROT_EXEC") == 0 ||
	    strcmp(permission, "PROT_READ | PROT_EXEC | PROT_WRITE") == 0 ||
	    strcmp(permission, "PROT_WRITE | PROT_READ | PROT_EXEC") == 0 ||
	    strcmp(permission, "PROT_WRITE | PROT_EXEC | PROT_READ") == 0 ||
	    strcmp(permission, "PROT_EXEC | PROT_WRITE | PROT_READ") == 0 ||
	    strcmp(permission, "PROT_EXEC | PROT_READ | PROT_WRITE") == 0)
		*perm = 7;
	if (strcmp(permission, "PROT_READ | PROT_WRITE") == 0 ||
	    strcmp(permission, "PROT_WRITE | PROT_READ") == 0)
		*perm = 6;
	if (strcmp(permission, "PROT_READ | PROT_EXEC") == 0 ||
	    strcmp(permission, "PROT_EXEC | PROT_READ") == 0)
		*perm = 5;
	if (strcmp(permission, "PROT_READ") == 0)
		*perm = 4;
	if (strcmp(permission, "PROT_WRITE | PROT_EXEC") == 0 ||
	    strcmp(permission, "PROT_EXEC | PROT_WRITE") == 0)
		*perm = 3;
	if (strcmp(permission, "PROT_WRITE") == 0)
		*perm = 2;
	if (strcmp(permission, "PROT_EXEC") == 0)
		*perm = 1;
	if (strcmp(permission, "PROT_NONE") == 0)
		*perm = 0;
}

//Functia seteaza permisiunea in miniblock.
void set_perm(int perm, dll_node_t *mini_node)
{
	if (perm == 7)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "RWX");
	if (perm == 6)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "RW-");
	if (perm == 5)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "R-X");
	if (perm == 4)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "R--");
	if (perm == 3)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "-WX");
	if (perm == 2)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "-W-");
	if (perm == 1)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "--X");
	if (perm == 0)
		strcpy(((miniblock_t *)(mini_node->data))->perm, "---");
}

//Functia verifica daca intr-un miniblock am dreptul de a citi.
unsigned int check_read(int8_t *perm)
{
	if (strcmp(perm, "R--") == 0)
		return 1;
	if (strcmp(perm, "RW-") == 0)
		return 1;
	if (strcmp(perm, "R-X") == 0)
		return 1;
	if (strcmp(perm, "RWX") == 0)
		return 1;
	return 0;
}

//Functia verifica daca intr-un miniblock am dreptul de a scrie.
unsigned int check_write(int8_t *perm)
{
	if (strcmp(perm, "-W-") == 0)
		return 1;
	if (strcmp(perm, "RW-") == 0)
		return 1;
	if (strcmp(perm, "RWX") == 0)
		return 1;
	if (strcmp(perm, "-WX") == 0)
		return 1;
	return 0;
}

//Functia verifica daca adresa la care trebuie sa fac modificari la permisiuni
// este valida.
unsigned int check_mprotect(arena_t *arena, const uint64_t address)
{
	//In primul rand verific daca exista block-uri in arena,
	//pentru ca nu pot sa modific permisiunile la o adresa care nu a fost
	//alocata.
	if (arena->alloc_list->size == 0)
		return 0;
	//Caut in lista de miniblock-uri a fiecarul block daca adresa a fost
	//alocata sau nu.
	dll_node_t *node = arena->alloc_list->head;
	while (node) {
		dll_node_t *mini_node;
		mini_node = ((doubly_linked_list_t *)(((block_t *)(node->data))
		->miniblock_list))->head;
		while (mini_node) {
			if (((miniblock_t *)mini_node->data)->start_address == address)
				return 1;
			mini_node = mini_node->next;
		}
		node = node->next;
	}
	//In cazul in care nu gasesc un miniblock adresa data, inseamna
	//ca adresa la care trebuie sa modific permisiunile nu este o adresa de
	//start a unui miniblock, prin urmare comanda este invalida.
	return 0;
}
