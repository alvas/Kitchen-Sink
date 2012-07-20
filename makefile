mac_dump : mac_dump.c
	gcc -Wall -o mac_dump mac_dump.c
clean:
	rm mac_dump mem.dump
