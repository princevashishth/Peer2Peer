# Peer to Peer chat application
<li>The chat application is written in C++ and  uses TCP as the underlying transport layer protocol </li>

<li> Every instance of the chat application runs a TCP server where the chat application listens for incoming connections. The same process of the application also runs one or multiple TCP clients (based on the number of peers) to connect to the other users and transfer messages </li>

<li>There can be a maximum of 5 peers</li>
### Usage

<li> Make sure to compile the program by typing

g++ P2P_multiclient.c -o p2p
<li> Run the file by ./p2p </li>
<li> Provide the number of peers with their names and their respective IP address </li>
<li> send the message by 

    peername/message 



