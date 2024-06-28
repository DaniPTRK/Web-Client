# Tema 4 PCom - Ion Daniel 325CC

### Surse externe utilizate pentru rezolvarea temei:
* laboratorul 9, de unde am preluat scheletul pentru a implementa client.c,
buffer.c, helpers.c, requests.c, buffer.h, helpers.h, requests.h si Makefile-ul;
* biblioteca de parsare a fisierelor JSON parson.c, parson.h:
https://github.com/kgabis/parson.

### Implementare Client - client.c
Pentru a implementa clientul, am realizat o bucla while() infinita in care se
asteapta si se executa comenzile date de catre utilizator, pana cand acesta da
exit. Comenzile implementate sunt urmatoarele:
1. register/login -> In cazul aplicarii acestor comenzi, verific daca clientul
este deja conectat sau nu prin verificarea cookie-ului inregistrat. Astfel,
daca clientul are un cookie, inseamna ca acesta este deja inregistrat cu un
cont, deci nu poate sa inregistreze un alt cont sau sa dea login la alt cont.
In acest caz, utilizatorul primeste un mesaj ce indica acest lucru. In cazul
in care clientul nu este conectat, acesta trebuie sa puna un username si
o parola, date care sunt verificate pentru a nu avea spatii sau a fi goale.
Trimit un POST request cu un JSON Object construit din username-ul si parola
primita si verific daca username-ul exista deja sau nu in cazul register-ului
sau daca parola este corecta pentru username-ul dat in cazul login-ului.
In cazul login-ului, daca totul este OK, pastrez cookie-ul primit in
mesajul de response.
2. enter_library -> Se verifica mai intai daca utilizatorul este logat prin
verificarea cookie-urilor. Dupa aceea, pentru a accesa biblioteca, se trimite
un GET request catre server, integrand in mesajul transmis cookie-ul
utilizatorului. In cazul in care raspunsul este OK, se extrage din mesajul
trimis de catre server token-ul primit in JSON si se pastreaza pentru
utilizatorul curent.
3. get_books -> Se verifica daca clientul este conectat si are un token de
acces in biblioteca. Dupa aceea, se trimite un GET request catre server,
integrand cookie-ul si token-ul utilizatorului, iar in cazul in care
raspunsul server-ului este OK, se extrage din continutul mesajului bucata
de final ce contine JSON Array-ul si se afiseaza.
4. get_book/delete_book -> Se verifica daca clientul este conectat si are un
token de acces in biblioteca. Daca conditiile sunt indeplinite, atunci
clientul va trebui sa trimita un ID valid (de tip unsigned int) pentru a
putea fi realizata cautarea. Dupa ce se citeste intr-un buffer ID-ul,
se verifica mai intai daca acesta are o forma valida ce permite transformarea
sa din sir de caractere in int, iar in caz contrar se afiseaza un mesaj de
eroare. Daca ID-ul dat are o forma valida, atunci se trimite un GET request
in cazul lui get_book sau un DELETE requeste in cazul lui delete_book catre
server cu URL-ul ce contine id-ul cartii. Daca aceasta carte exista, este
afisata/stearsa in functie de comanda data.
5. add_book -> Se verifica daca clientul este conectat si are un token de
acces in biblioteca. Daca aceste conditii sunt indeplinite, clientul va
trebui sa ofere informatiile necesare pentru a adauga cartea in
biblioteca, dupa care se vor verifica datele trimise de catre acesta, anume
daca exista vreun camp lasat gol sau daca page_count-ul nu are o forma
numerica. Daca toate datele sunt valide, atunci se construieste un JSON
Object cu datele primite, iar apoi se trimite un POST request catre
server pentru a inregistra cartea. Clientul primeste apoi feedback in
functie de raspunsul dat de server.
6. logout -> Se verifica daca clientul este conectat. Daca este, atunci
se trimite un GET request pe URL-ul de logout ce contine cookie-ul si
token-ul, iar apoi sunt sterse cookie-ul si token-ul inregistrate.
7. exit -> Se elibereaza memoria, se inchide socket-ul prin care se comunica
cu server-ul si se iese din bucla while().

### Fisiere sursa auxiliare
* requests.c -> se ocupa cu procesul de construire al unui mesaj ce va fi
trimis ca request catre server. Tipurile de request-uri implementate sunt
GET, POST si DELETE.
* helpers.c -> se ocupa cu trimiterea datelor catre server si receptionarea
acestora.
* buffer.c -> ajuta la construirea buffer-ului de date ce va fi trimis catre
server sau primit de la server.
* parson.c -> sursa folosita pentru parsarea datelor de tip JSON.
