# 1. COMPILAZIONE

  make;
  read -p "Compilazione eseguita. Premi invio per eseguire..."

# 2.1 esecuzione del server sulla porta 4242
  gnome-terminal -- sh -c "./server 4242; exec bash"

  sleep 1;

# 2.2 esecuzione del primo client che si collega alla porta 4242 del server
	gnome-terminal -- sh -c "./client 4242; exec bash"

# 2.3 esecuzione del secondo client che si collega alla porta 4242 del server
	gnome-terminal -- sh -c "./client 4242; exec bash"