# Documentazione Progetto Reti Informatiche 23/24

<center>Antonio Ciociola</center>

## Cartelle

- **src, obj, dep** contengono i sorgenti, i file oggetto e i file di dipendenze del client e del server.
- **data** contiene 3 cartelle con file di testo per il server
  - **bacheca** contiene il file *bacheca.txt* che contiene i messaggi della bacheca
  - **rooms** contiene i file *id.txt* che descrivono le stanze, *available.txt* che contiene le stanze attualmente disponibili, e *room_keywords.md* che ha al suo interno tutte le keyword utilizzabili per una room.
  - **login** contiene i file *username.txt* che contengono gli hash delle password degli utenti registrati.

## Protocollo e formato dei messaggi

Il protocollo utilizzato è TCP dato che la comunicazione tra client e server è loss intolerant.

I messaggi inviati dal client al server sono formati da 1 a 3 stringhe, che sono il
comando con 2 parametri opzionali.

I messaggi inviati dal server al client sono formati da una stringa di response che
verrà stampata a video e dallo stato del gioco, che viene inviato come intero. Per la trasmissione delle stringhe viene prima inviata la lunghezza della stringa e poi la stringa stessa.

## Threads e Processi

Sia il client che il server sono multi-thread, per poter gestire contemporaneamente la lettura dei comandi da tastiera e la ricezione di messaggi dal socket.

### Server

È stato implementato un server concorrente, dato che le partite sono isolate tra loro non è un problema avere più processi che non condividono memoria a gestire le partite.

- Thread principale: gestisce i comandi da tastiera e l’avvio o lo spegnimento del thread server.
- Thread server: gestisce le richieste dei client che si connettono al server e per ogni client crea un processo figlio che gestisce il gioco per quel client.

Avere un processo per client è dispendioso dato che la maggior parte del tempo esso aspetterà che il client gli mandi un comando, inoltre la room viene instanziata per intero per ogni client, questo fa sprecare memoria in quanto per esempio la descrizione degli oggetti rimane costante tra tutti i client. Ma fino a quando non si ha un numero di client molto elevato questo non è un problema.

### Client

- Thread principale: crea due thread, uno per la lettura dei comandi da tastiera e uno per la ricezione dei messaggi dal socket.
- Thread lettore: gestisce la lettura dei comandi da tastiera e il loro invio al server.
- Thread ricevitore: gestisce la ricezione dei messaggi dal server e la loro stampa a video.

## Tempistiche della connessione

Per quasi tutta la durata della connessione la conversazione tra client e server si ha il client che invia un comando e il server che risponde con un messaggio di risposta e lo stato del gioco. La necessità di avere 2 thread sul client quindi si ha solo per poter gestire la fine della partita a causa della fine del tempo.

## Implementazione del gioco

Il gioco è stato implementato con la possibilità di avere oggetti invisibili e/o bloccati, che possono essere sbloccati o resi visibili dall’attivazione di altri oggetti. Per attivazione si intende nel caso di un oggetto utilizzabile, l’utilizzo di quell’oggetto. In caso di oggetto non utilizzabile è lo sblocco di esso, tramite un altra attivazione o la risoluzione dell’enigma che lo blocca.

Non tutti gli oggetti possono essere raccolti e il comando take in caso di oggetto bloccato funge da comando di interazione con l’enigma. Per ottenere i token e/o il tempo bonus di un oggetto è necessario raccogliere l’oggetto se è possibile farlo, in caso contrario basta osservarlo.

La partita termina automaticamente quando il tempo finisce, quando un enigma viene bloccato, quando vengono raccolti tutti i token o quando il client invia il comando end

## Bacheca

Per la funzionalità a scelta è stata implementata una bacheca. La bacheca è un file di testo che contiene messaggi che gli utenti possono lasciare per gli altri utenti.

Essa è accessibile dopo il login con il comando bacheca, utilizzandola in questo modo verrà stampata, con bacheca write si potrà scrivere un messaggio sulla bacheca.