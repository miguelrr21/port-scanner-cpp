# port-scanner-cpp
TCP port scanner built in C++ for learning networking and security fundamentals

Ive done this project in windows 11 home, not in linux then i had to use the libraries <winsock2.h> & #include <ws2tcpip.h>.

## V1 Single-port-scanner

First of all, we've to input as arguments the IP and PORT we want to scan (for example: scanner.exe x.x.x.x 80)
that port "80" is stored as a string, so we use STOI (String to integer) on the port argument since the arguments are always c-string.

Next step is create a socket for the communication, but before creating it we have to initialize it (this because its windows, linux is always ready) After checking winsock is all right, we create the socket:
socket(domain, type, protocol);

after checking if it's created, we configure a conection timeout, in case the port is filtered by a firewall (1000 ms)

now we prepare the target IP using sckaddr_in, which represents the IP and the PORT (sockets API requirements)

destino.sin_port = htons(port);

transforms the port number to the bytes format that the network is expecting (big-endian) this is MANDATORY.

then we use inet_pton to do the same thing with the IP (trasform it to binary)

now comes the three way handshake, that lets us know if that port is open or closed.
If there is an error, we check the wsa error buffer to check if its a timeout or the port is closed.

Finally, we have to close the socket and clean the WSA.

## V2 Port-range-scanner

Now, the program scans a range of ports (given by parameters with syntax "x-y") We get the first and last port to check using parsing, getting the position of "-" before using stoi on each string.

After that, we just add a for loop from portIni to portFin just before creating the socket, because sockets cannot be used for multiple ports.

I've implemented an chrono to check the time each port gives conection and deleted the timeout.

Last, we clean the WSA.

## V3 Thread_usage

We created a new function, void scanner(std::string ip, int port) , with all the socket creation and conection process. It also has the mutex for the cout, in case two different threads finish at the same time, output is not overlapped.

then in the main, first we created a vector of threads "hilos_puertos", then a for loop that creates a thread for each port and moves it to the vector (keep in mind, threads cannot be copied, so we have to move the process)

Last we check if every thread has finished, using a "range for" and the function join().