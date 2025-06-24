# ft_irc

Our project aims to build an IRC server that is irssi client compliant. In this server we allow multiple commands and users can message themselves. It is also possible to join the server by using `netcat`/`nc`. You can check this quick [video](https://www.youtube.com/watch?v=ZtNOcTy7Kqg) of our server in action.

## What is IRC?

IRC stands for Internet Relay Channel. It is a form of communication between users that is a bit demanding on UX, but it is very efficient due to small size of bytes transferred between users. For more information, you can check our very informative [wiki](https://github.com/ArminKuburas/ft_irc/wiki) or open an issue.

## Usage

### Server side
To start running a server, all you have to do is type:
```bash
./ircserv <port> <passoword>
```
Our standard port for testing has been `6667`. However, we greatly advise you to research on [ports](https://github.com/ArminKuburas/ft_irc/wiki/port) to understand and select the appropriate port.

#### User side
If you are a user, you can either get into the server (if it is running) using `irssi` or `netcat`:


- Using `irssi`

In terminal, type the following:
```bash
irssi
```
This will lead you to `irssi` interface. Over there you will *not* be following bash syntax. Therefore, you must follow the next steps precisely to join the server:
```
/connect <server_ip> <port> <password>
```
There will be an authentication process running by itself as `irssi` and the server communicate. After said authentication, one should see a message that should look like:

```
17:21 -!-  ___           _
17:21 -!- |_ _|_ _ _____(_)
17:21 -!-  | || '_(_-<_-< |
17:21 -!- |___|_| /__/__/_|
17:21 -!- Irssi v1.2.3 - https://irssi.org
17:21 -!- Irssi: Looking up 127.0.0.1
17:21 -!- Irssi: Connecting to 127.0.0.1 [127.0.0.1] port 6667
17:21 -!- Irssi: Connection to 127.0.0.1 established
17:21 -!- Capabilities supported: 
17:21 -!- You're now known as fdessoy
17:21 -!- Welcome to the server
17:21 !Zorg Available commands:
17:21 !Zorg   NICK     - Change your nickname
17:21 !Zorg   USER     - Set your username and real name
17:21 !Zorg   JOIN     - Join a channel
17:21 !Zorg   PART     - Leave a channel
17:21 !Zorg   PRIVMSG  - Send a message to a user or channel
17:21 !Zorg   MODE     - Set user or channel modes
17:21 !Zorg   TOPIC    - View or change channel topic
17:21 !Zorg   WHOIS    - Get information about a user
17:21 !Zorg   QUIT     - Disconnect from the server
17:21 !Zorg   STATS    - Show server statistics
17:21 !Zorg   WHO      - List users in a channel
17:21 !Zorg   INVITE   - Invite a user to a channel
17:21 !Zorg   KICK     - Remove a user from a channel
17:21 !Zorg   HELP     - Show this help message
17:21 !Zorg Type HELP <command> for more information about a specific command
17:21 !Zorg Note: In irssi, commands must be prepended with /quote (e.g., /quote NICK)
17:21 -!- Your user mode is [+i]
```
From this point, you can create channels using `JOIN` and use `PRIVMSG` to message your friends inside the server!

- Using `netcat`

In terminal, type:
```bash
$> netcat 127.0.0.1 6667
```
This will connect you to the server. However, you have to finish the authentication by yourself, since there is no client to do the work for you. You can do so by following these steps:
```bash
$> netcat 127.0.0.1 6667    
PASS 123
NICK flap
:!@127.0.0.1 NICK :flap
USER flap fdessoy zorg felipe      
:Zorg 001 flap :Welcome to the server
:Zorg NOTICE flap :Available commands:
:Zorg NOTICE flap :  NICK     - Change your nickname
:Zorg NOTICE flap :  USER     - Set your username and real name
:Zorg NOTICE flap :  JOIN     - Join a channel
:Zorg NOTICE flap :  PART     - Leave a channel
:Zorg NOTICE flap :  PRIVMSG  - Send a message to a user or channel
:Zorg NOTICE flap :  MODE     - Set user or channel modes
:Zorg NOTICE flap :  TOPIC    - View or change channel topic
:Zorg NOTICE flap :  WHOIS    - Get information about a user
:Zorg NOTICE flap :  QUIT     - Disconnect from the server
:Zorg NOTICE flap :  STATS    - Show server statistics
:Zorg NOTICE flap :  WHO      - List users in a channel
:Zorg NOTICE flap :  INVITE   - Invite a user to a channel
:Zorg NOTICE flap :  KICK     - Remove a user from a channel
:Zorg NOTICE flap :  HELP     - Show this help message
:Zorg NOTICE flap :Type HELP <command> for more information about a specific command
:Zorg NOTICE flap :Note: In irssi, commands must be prepended with /quote (e.g., /quote NICK)
JOIN #BBQ
:flap JOIN #BBQ
:Zorg 353 flap = #BBQ :@flap 
:Zorg 366 flap #BBQ :End of /NAMES list.
```
There is no specific order in which the authentication should occur, but it might be best to provide `PASS`, `NICK`, and `USER`. 

`PASS` and `NICK` will only ask for one argument, which is the password and your nickname respectively. 

`USER` requires you to provide a `username`, `hostname`, `server_name`, and `realname`.

After authentication is done, as you can see, you can join channels with your friends!

## Contributors

- Armin Kuburas - [GitHub](https://github.com/ArminKuburas/) - [Linkedin](https://www.linkedin.com/in/armin-kuburas-kub/) 
- Felipe Dessoy Caraballo - [GitHub](https://github.com/fjjdessoycaraballo) - [Linkedin](https://www.linkedin.com/in/fdessoy/) - [Linktree](https://linktr.ee/fdessoy)
- Pablo Markaide - [GitHub](https://github.com/pmarkaide) - [Linkedin](https://www.linkedin.com/in/pmarkaide/)
