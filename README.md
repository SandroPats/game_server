# multiplayer game server
The first part of the project contains multiplayer game server implemented in C. The game is called ``Management`` and was created by ``Avalon Hill Company`` to develop basic skills for company management.
The second part of the project is a С++ implemented programmable game bot imitating human behaviour. The bot recieves a scenario textfile as input. Scenarios are written int a specificly developed programming langauge.

## Installation

1. Clone this repository
2. Compile the server:
```zsh
g++ -Wall -g gameserv/gameserv.c -o gameserv
```
3. execute MakeFile in ``aibot/`` directory:
```zsh
cd aibot
make
```
4. Go back to the root of the repository and run the server. Server recieves the amount of players and TCP-port number as input. As an example:
```zsh
./gameserv/gameserv 10 4444
```
where ``10`` indicates the amount of players and ``4444`` corresponds to the TCP-port number.
you can run the server as a background process by adding ``&`` to the end of the command.
5. ``Telnet`` is used as a client-side programm. Followin command allows to join to the server:
```zsh
telnet <IP-adress> <TCP-port numper>
```
6. To add bot to the server run:
```zsh
./aibot/bot <IP-adress> <TCP-port numer> <number of players> <scenario filename>
```
To add multiple bots you can run ``aibot/boscript``:
```zsh
./aibot/botscript <number of bots> <scenario filename>
```
Make sure you have permission to run the script:
```zsh
chmod +x aibot/botscript
```
