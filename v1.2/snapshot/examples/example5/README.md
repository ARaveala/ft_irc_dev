# FT_IRC

## Project:
FT_IRC is an Internet Relay Chat server implemented in C++ that follows the IRC protocol and supports multiple clients simultaneously. It offers the basic IRC functionalities, such as user authentication, nickname setting, channel related commands and etc. 

## Contributers
This project was made in collaboration by:
[Thomas Parratt](https://github.com/ThomasParratt)
[Markus Korpela](https://github.com/Marsu4ever)
[Joseph Lu](https://github.com/quietmid)

## Core Features
- **User authentication:** Checks if the client connecting is providing the correct port and password information. Returns proper error message if the information isn't correct.
- **Welcoming message:** Upon successful client registration, client will recieve the welcoming message from our server, which will have the server's information, such as server name, version, server created time and channel modes.
- **Nickname Setting:** with /nick <yourNewNick> will change your nickname to your desired new one and alert everyone that you are associated with by channels.
- **Join and leave channels:** with /join you can join an existing channels or create a new channel. with /part or /leave you can leave your current channels, if you wish to leave multiple channels at once. you can /part #channelname1,#channelname2,...
- **Private message:** clients are allowed to message other clients privately.
- **Setting channel topic:** allows the user to set channel topic or to see what the channel topic is
- **Channel Mode:** with different channel mode flag, as a channel operator, you will be able to set channel information and rules. For example: /mode +i
  - *+i / -i:* this turns on invite only for this channel
  - *+t / -t:* -t turns off the restriction that only operator can set channel topic. +t turns the restriction back.
  - *+k <passkey> / -k:* adds a channel key, which is required for new comer to add to their command upon wishing to join the channel. -k removes the restriction
  - *+o <nick> / -o <nick>:* gives or takes the user's channel operator privilege.
  - *+l <number> / -l:* adds a limit to the number of users can be in the channel and the -l removes this restriction
- **Clients quitting:** When a client quits, our server alerts the rest of the users, they have any association by channels or priv msg.

## Examples
<h4>Server starting</h4>
<img src="https://i.imgur.com/GwBb3fR.gif" width="300">
      
<h4>Interaction between the Server and client</h4>
<img src="https://i.imgur.com/kNVbXsq.gif" width="300">

## Usage
Before testing the program, you might want to download irssi, which is our preferred client ([irssi download link](https://irssi.org/download/)).
You can test the program by git cloning the project through a terminal and head into the Makefile and make sure you have the correct cflag based on your operating system. Return to the main directory and
```
make
```
After make, you should have the executable called ircserv. You can test the program by entering
```
./ircserv <port> <password>
eg. ./ircserv 6667 4321
```
You should see the following text in the terminal.
```
Server started at Wed Jan 29 12:17:50 2025
Server loop started
```
Now open another terminal and enter the following command. -c stands for connect and w is password and p is port. You can also test it out with different numbers and password to see the error messages.
```
irssi -c localhost -w 4321 -p 6667
```
Upon connecting, you should see this from the server terminal
```
New client connected, socket 4
<< CAP LS
<< PASS 4321
<< NICK josephlu
<< USER josephlu josephlu localhost :Joseph Lu
>> :ircserver 001 josephlu :Welcome to the IRC network josephlu!josephlu@localhost
>> :ircserver 002 josephlu :Your host is ircserver, running version 1.0
>> :ircserver 003 josephlu :This server was created Wed Jan 29 12:21:45 2025
>> :ircserver 004 josephlu ircserver 1.0 ro itkol
>> :ircserver 005 josephlu CHANMODES=i,t,k,o,l :are supported by this server
<< MODE josephlu +i
<< PING ircserver
>> PONG ircserver
```
Now from the irssi/client terminal, you can test out our IRC server!

## PS
if you are experiencing compiling issue with non_block, you might have to head to serverInit.cpp and under Server::serverInit() and choose the one that matches your system
```
// linux
// _serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
//MacOS
_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
fcntl(_serverSocket, F_SETFL, O_NONBLOCK); // for mac user
```
