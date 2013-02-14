This is how I will be testing your client and server

In one window start your server. 
In another window do the following at a the shell prompt.

Set the following variable:

server="server:port"

Where server the ip address of the computer on which your
server is running and port is the port it is listening on.

Now you may begin the tests:

For the following you will need to inspect the output to be sure
that things are correct 

0) simple test

./csimple $server | ./client

1) more complex tests

./cfile ./client $server
./cfile ./client $server /etc/services

2) even more rigors

for ((i=0;i<100;i++))
do 
   ./cfile ./client $server /etc/services & 
   ./cfile ./client $server & 
   ./cfile ./client $server /etc/shells  &  
done

