# ft_irc

This project is our final C++ project, where we implemented an IRC server. The server is fully compliant with RFC standards and has been thoroughly tested with the Irssi client.

## Features

- Fully RFC compliant
- Tested with the Irssi client
- Supports multiple simultaneous client connections
- Routes messages between connected clients
- Includes a chatbot that can:
  - Connect to the server
  - Join channels
  - Chat with users
  - Kick users who violate rules

## Description

The server can handle connections from multiple clients at the same time, ensuring smooth communication and message routing between them. The integrated chatbot enhances the server's functionality by managing interactions within channels.

## Commands
- NICK
- USER
- JOIN
- PRIVMSG
- QUIT
- PART
- PASS
- PING
- WHOIS
- WHOWAS
- MODE
- KICK
- TOPIC
- INVITE
- AWAY
- WHO
- OPER
- KILL

## Usage
```
make
./ircserv [port] [password]

make bot
./bot_v1 [server] [port] [password] [nickname] [password]
```

We have used the following RFCs:
- [RFC 1459](https://tools.ietf.org/html/rfc1459)
- [RFC 2810](https://tools.ietf.org/html/rfc2812)
- [Modern IRC](https://modern.ircdocs.horse/)
