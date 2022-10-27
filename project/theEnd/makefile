all: server1-LeCrochet server2-LeCrochet server3-LeCrochet
server1-LeCrochet: server1-LeCrochet.c
gcc server1-LeCrochet.c -pthread -o server1-LeCrochet

server2-LeCrochet: server2-LeCrochet.c
gcc server2-LeCrochet.c -pthread -o server2-LeCrochet

server3-LeCrochet: server3-LeCrochet.c
gcc server3-LeCrochet.c -pthread -o server3-LeCrochet

clean:
rm -f server1-LeCrochet
rm -f server2-LeCrochet
rm -f server3-LeCrochet