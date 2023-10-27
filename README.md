##### Copyright 2023 Niculici Mihai-Daniel (mihai.viper90@gmail.com)

		 Virtual Memory Allocator
			-Tema 1-
	
		Pentru a rezolva aceasta tema, am folosit conceptul de lista in lista,
	ambele liste contin noduri in care pot stoca orice, prin urmare, consider ca
	am respectat regula privind genericitatea.
		Pentru absolut tot ce am alocat dinamic, am incercat sa folosesc
	programarea defensiva prezentata la laborator, adica folosind "DIE".

		La executia programului, se citesc la nesfarsit comenzi pana la cand
	este intalnita comanda "DEALLOC_ARENA", moment in care se elibereaza toata
	memoria alocata dinamic, dupa care executia programului este incheiata.

		1)Atunci cand este introdusa comanda "ALLOC_ARENA", se creeaza aceasta arena,
	'alocand-o'. Campurile structurii de arena sunt atribuite si alocate
	dinamic.
		
		2)La intalnirea comenzii "DEALLOC_ARENA", se elibereaza memoria listelor de
	miniblock-uri, apoi cea a listelor de block-uri, dupa care este eliberata si
	memoria ocupata de arena.

		3)Daca introducem comanda "ALLOC_BLOCK", mai intai se verifica daca adresa pe
	care vreau sa o aloc este valida, dupa care verific unde se va aloca acest
	block in lista. Pentru acest lucru eu am creat 4 cazuri:

		CAZUL 0 : Block-ul pe care trebuie sa il aloc, nu are niciun
		block adiacent.

		CAZUL 1 : Block-ul pe care trebuie sa il aloc, continua un block
			  care este deja alocat.

		CAZUL 2 : Block-ul pe care trebuie sa il aloc, se termina unde
			  incepe un block deja alocat.

		CAZUL 3 : Block-ul pe care trebuie sa il aloc, se afla fix intre 2 block-uri
			  deja alocate.
	Apoi block-ul este alocat.

		4)La introducerea comenzii "FREE_BLOCK", mai intai se verifica daca adresa la
	care trebuie sa eliberez memoria este valida, dupa care, se verifica
	in ce caz se incadreaza aceasta operatie, dupa care este executata.
	Din nou, Eu am creat 4 cazuri pentru aceasta operatie:
		
		CAZUL 0 : Block-ul pe care trebuie sa il eliberez, are doar un
		miniblock.

		CAZUL 1 : Eliberez un miniblock care se afla la sfarsitul unei liste de
		miniblock-uri.

		CAZUL 2 : Eliberez un miniblock care se afla la inceputul unei liste de
		miniblock-uri.

		CAZUL 3 : Eliberez un miniblock care se afla in mijlocul unei liste de
		miniblock-uri.
	
		5)Atunci cand introducem comanda "WRITE", mai intai este verificata adresa la
	care programul trebuie sa scrie pentru a vedea daca este valida si daca are
	permisiunea de a scrie, dupa care se cauta miniblock-ul unde trebuie sa
	inceapa sa scrie, apoi comanda este executata. Daca in momentul scrierii,
	miniblock-ul se termina, se trece la urmatorul din lista de miniblock-uri
	a block-ului pana cand se termina toate miniblock-urile sau pana cand se
	termina numarul de caractere pe care trebuie sa le scrie.

		6)Atunci cand introducem comanda "READ", mai intai este verificata adresa la
	care programul trebuie sa citeasca pentru a vedea daca este valida si daca are
	permisiunea de a citi, dupa care se cauta miniblock-ul unde trebuie sa
	inceapa sa citeasca, apoi comanda este executata. Daca in momentul citirii,
	miniblock-ul se termina, se trece la urmatorul din lista de miniblock-uri
	a block-ului pana cand se termina toate miniblock-urile sau pana cand se
	termina numarul de caractere pe care trebuie sa le citeasca.

		Tin sa mentionez ca am initializat campul "rw_buffer" al miniblock-urilor
	cu 0 pentru a nu putea sa se citeasca locul unde nu a fost scris nimic
	anterior.

		7)La introducerea comenzii "PMAP", se parcurce lista de block-uri, iar pentru
	fiecare block in parte, este parcursa lista de miniblock-uri a lui. In timp
	ce se desfasoara aceasta parcurgere sunt afisate toate informatiile dorite.

		8)BONUS: Atunci cand introducem comanda "MPROTECT", se verifica daca adresa la
	care trebuie sa schimbam permisiunile este valida, dupa care parcurg lista de
	block-uri pentru a vedea in ce block se afla miniblock-ul ale carui permisiuni
	trebuie sa le modific. Dupa ce am gasit block-ul, se cauta in lista de
	miniblock-uri a lui, miniblock-ul ale carui permisiuni trebuie modificate.
	Verific ce permisiune trebuie sa atribui miniblock-ului, dupa care modific
	permisiunile miniblock-ului de la adresa data.



	