#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <errno.h>
#include "bot.hpp"
#include "syntan.hpp"
#include "rpn.hpp"

Bot::Bot(int plrs)
{
	neededplrs = plrs;
	plid = 1;
	player = 0;
	id = 0;
	winnerid = 0;
	raw_auction = 0;
	prod_auction = 0;
	bank = new banker;
	bank->turn = 0;
	bank->players = 0;
	bank->market.raw_am = 0;
	bank->market.raw_min_price = 0;
	bank->market.prod_am = 0;
	bank->market.prod_max_price = 0;
	game = true;
}

Bot::~Bot()
{
	delete bank;
	if (player)
		DeletePlayersList();
	if (raw_auction || prod_auction)
		DeleteAuctionLists();
}

bool IsPrefix(const char *s1, const char *s2)
{
	if (!*s1 || !*s2)
		return false;
	while (*s1 && *s2) {
		if (*s1 != *s2)
			return false;
		s1++;
		s2++;
	}
	return true;
}

void Bot::DeletePlayersList()
{
	while (player) {
		players *p = player;
		player = player->next;
		delete p;
	}
}

void Bot::DeleteAuctionLists()
{
	while (prod_auction) {
		auction *p = prod_auction;
		prod_auction = prod_auction->next;
		delete p;
	}
	while (raw_auction) {
		auction *p = raw_auction;
		raw_auction = raw_auction->next;
		delete p;
	}
}

bool Server::Connect()
{
	struct sockaddr_in addr;
	char *endptr;
	errno = 0;
	port = strtoll(portstr, &endptr, 10);
	if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN)) ||
		(errno != 0 && port == 0)) {
		perror("strtol");
		return 0;
	}
	if (endptr == portstr) {
		fprintf(stderr, "Second argument has no digits in it\n");
		return 0;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if ( -1 == (sfd = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket");
		return 0;
	}
	if (!inet_aton(ip, &(addr.sin_addr))) {
		fprintf(stderr, "invalid IP-adress\n");
		return 0;
	}
	if (0 != connect(sfd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror("connect");
		return 0;
	}
	return 1;
}

Server::Server(char *ipstr, char *pstr)
{
	portstr = new char[strlen(pstr) + 1];
	ip = new char[strlen(ipstr) + 1];
	strcpy(ip, ipstr);
	strcpy(portstr, pstr);
	sfd = 0;
	curindex = 0;
	buf[0] = '\0';
}

Server::~Server()
{
	delete[] ip;
	delete[] portstr;
}

int Server::Read()
{
	int count = 0, rc = 0;
	buf[0] = '\0';
	while ((strlen(buf) < 3 || buf[strlen(buf) - 3] != '#') &&
		sizeof(buf) - 1 - strlen(buf) > 0) {
		rc = read(sfd, buf + strlen(buf),
			sizeof(buf) - 1 - strlen(buf));
		if (rc == -1) {
			perror("read");
		} else if (rc == 0) {
		//	fprintf(stderr, "Connection lost\n");
		//	return rc;
			throw "Connection lost";
		}
		count += rc;
		buf[count] = '\0';
	}
	//printf("############\n%s###############\n", buf);
	return count;
}	

char *Server::ReceiveMessage()
{
	int i;
	char *mes;
	if (!buf[curindex]) {
		curindex = 0;
		if (0 == Read())
			return 0;
	}
	for (i = curindex; buf[i] != '#' && buf[i]; i++);
	if (!buf[i]) {
		curindex = 0;
		if (0 == Read())
			return 0;
		for (i = curindex; buf[i] != '#' && buf[i]; i++);
	}
	mes = new char[i - curindex + 2];
	for (int j = 0; j <= i - curindex; j++)
		mes[j] = buf[j + curindex];
	mes[i - curindex + 1] = '\0';
	curindex = i + 3;
	return mes;
}

void Server::Start() const
{
	char s[] = "start\n";
	write(sfd, s, sizeof(s));
}

void Server::Player(int id) const
{
	char s[128];
	sprintf(s, "player %d\n", id);
	write(sfd, s, strlen(s) + 1);
}

void Server::Buy(int amount, int price) const
{
	char s[128];
	sprintf(s, "buy %d %d\n", amount, price);
	write(sfd, s, strlen(s) + 1);
}

void Server::Sell(int amount, int price) const
{
	char s[128];
	sprintf(s, "sell %d %d\n", amount, price);
	write(sfd, s, strlen(s) + 1);
}

void Server::Market() const
{
	char s[] = "market\n";
	write(sfd, s, sizeof(s));
}

void Server::Produce(int amount) const
{
	char s[128];
	sprintf(s, "prod %d\n", amount);
	write(sfd, s, strlen(s) + 1);
}

void Server::Build() const
{
	char s[] = "build\n";
	write(sfd, s, strlen(s) + 1);
}

void Server::Turn() const
{
	char s[] = "turn\n";
	write(sfd, s, sizeof(s));
}

static void SscanfErrorCheck(int n)
{
	if (errno != 0)
		perror("scanf");
	else if (n == EOF)
		fprintf(stderr, "No matching characters");
}

void Bot::GetPlayerInfo(char *mes)
{
	int n;
	players *pl = new players;
	while (*mes != '%') {
		mes++;
	}
	errno = 0;
	n = sscanf(mes, "%%\t%d\t%d\t%d\t%d",
		&pl->cash, &pl->raw, &pl->products, &pl->factories);
	SscanfErrorCheck(n);
	pl->id = plid;
	pl->next = player;
	player = pl;
	plid++;
}

void Bot::GetBotInfo()
{
	players *p = player;
	while (p->id != id)
		p = p->next;
	bot.id = id;
	bot.cash = p->cash;
	bot.products = p->products;
	bot.factories = p->factories;
	bot.raw = p->raw;
	plid = 1;
}

void Bot::GetReadyPlayers(char *mes)
{
	int n;
	while (*mes != '%')
		mes++;
	errno = 0;
	n = sscanf(mes, "%%\t%d", &bank->players);
	SscanfErrorCheck(n);
}

void Bot::GetMarketInfo(char *mes)
{
	int n;
	while (*mes != '%')
		mes++;
	errno = 0;
	n = sscanf(mes, "%%\t\t%d", &bank->players);
	SscanfErrorCheck(n);
	mes++;
	while (*mes != '%')
		mes++;
	errno = 0;
	n = sscanf(mes, "%%\t\t%d\t%d",
		&bank->market.raw_am, &bank->market.raw_min_price);
	SscanfErrorCheck(n);
	mes++;
	while (*mes != '%')
		mes++;
	errno = 0;
	n = sscanf(mes, "%%\t\t%d\t%d",
		&bank->market.prod_am, &bank->market.prod_max_price);
	SscanfErrorCheck(n);
}

void Bot::GetProductAuctionResults(char *mes)
{
	auction *prod;
	while (*mes) {
		int n;
		while (*mes != '%' && *mes)
			mes++;
		if (!*mes)
			continue;
		prod  = new auction;
		prod->next = prod_auction;
		errno = 0;
		n = sscanf(mes, "%%%d\t%d\t%d",
		&prod->id, &prod->amount, &prod->price);
		SscanfErrorCheck(n);
		prod_auction = prod;
		mes++;
	}
}

void Bot::GetRawAuctionResults(char *mes)
{
	auction *raw;
	while (*mes) {
		int n;
		while (*mes != '%' && *mes)
			mes++;
		if (!*mes)
			continue;
		raw  = new auction;
		raw->next = raw_auction;
		raw_auction = raw;
		errno = 0;
		n = sscanf(mes, "%%%d\t%d\t%d",
		&raw->id, &raw->amount, &raw->price);
		SscanfErrorCheck(n);
		mes++;
	}
}

void Bot::GetWinnerId(char *mes)
{
	if (IsPrefix(mes, "YOU WIN")) {
		winnerid= id;
		printf("This bot wins the game\n");
	} else if (IsPrefix(mes, "Player")) {
		sscanf(mes, "Player %%%d", &winnerid);
		printf("The winner is player %d\n", winnerid);
	} else {
		printf("No winner\n");
	}
	game = false;
}

int Bot::GetConnectedPlayers(char *mes)
{
	int c;
	while (*mes != '%' && *mes)
		mes++;
	sscanf(mes, "%%\t%d", &c);
	return c;
}

int Bot::HandleMessage(char *mes)
{
	if (IsPrefix(mes, "your id is %")) {
		sscanf(mes, "your id is %%%d", &id);
	} else if (IsPrefix(mes, "\rplayers ready")) {
		GetReadyPlayers(mes);
	} else if (IsPrefix(mes, "players connected") &&
		GetConnectedPlayers(mes) >= neededplrs) {
		return cstart;
	} else if (IsPrefix(mes, "\rThe game has begun") ||
		IsPrefix(mes, "\rThe game has already")) {
		return cplayer;
	} else if (IsPrefix(mes, "\rplayers still active")) {
		GetMarketInfo(mes);
		return crpn;
	} else if (IsPrefix(mes, "\rnot enough factories") ||
		IsPrefix(mes, "\rnot enough raw\n") ||
		IsPrefix(mes, "\rproducts ordered") ||
		IsPrefix(mes, "\rthe amount must be more then zero")) {
	} else if (IsPrefix(mes, "\rfactories under constr")) {
	} else if (IsPrefix(mes, "\rbank can't buy so many")) {
	} else if (IsPrefix(mes, "\rPurchase request") ||
		IsPrefix(mes, "\rnot enough cash")) {
	} else if (IsPrefix(mes, "\rSale request")) {
	} else if (IsPrefix(mes, "\rraw auction results")) {
		GetRawAuctionResults(mes);
	} else if (IsPrefix(mes, "\rproduct auction results")) {
		GetProductAuctionResults(mes);
		//return crpn;
	} else if (IsPrefix(mes, "\rinfo:")) {
		GetPlayerInfo(mes);
		return cplayer;
	} else if (IsPrefix(mes, "\rcurrent month")) {
		sscanf(mes, "\rcurrent month is %%%d", &bank->turn);
		return cplayer;
	} else if (IsPrefix(mes, "YOU WIN") || IsPrefix(mes, "IT'S A DRAW") ||
		IsPrefix(mes, "Player %")) {
		GetWinnerId(mes);
	} else if (IsPrefix(mes, "\rno player with")) {
		GetBotInfo();
		return cmarket;
	}
	return no;
}

void Bot::Send(const Server &serv, int cmd)
{
	switch (cmd) {
		case cstart:
			bank->turn = 1;
			serv.Start();
			break;
		case cplayer:
			serv.Player(plid);
			break;
		case cmarket:
			serv.Market();
			break;
		case crpn:
			break;
		case no:
			break;
	}
}

bool  Bot::Handle(char *mes, const Server &serv)
{
	int cmd;
	cmd = HandleMessage(mes);
	Send(serv, cmd);
	if (cmd == crpn)
		return true;
	return false;
}

void Bot::PrintInfo()
{
	players *p = player;
	while(p) {
		printf("player id=%d\n"
			"cash=%d\n"
			"raw=%d\n"
			"products=%d\n"
			"factories=%d\n#\n",
			p->id, p->cash,p->raw,p->products,p->factories);
		p = p->next;
	}
	printf("currnet month is %dth\n"
		"bank sells %d raw for min price %d\n"
		"bank buys %d products for max price %d\n#\n",
		bank->turn, bank->market.raw_am, bank->market.raw_min_price,
		bank->market.prod_am, bank->market.prod_max_price);
	if (raw_auction) {
		auction *raw = raw_auction;
		while (raw) {
			printf("player#%d bought %d raw for %d each\n#\n",
			raw->id, raw->amount, raw->price);
			raw = raw->next;
		}
	}
	if (prod_auction) {
		auction *prod = prod_auction;
		while (prod) {
			printf("player#%d sold %d products for %d each\n#\n",
			prod->id, prod->amount, prod->price);
			prod = prod->next;
		}
	}

}

players *Bot::GetPlPtr(int plid) const
{
	players *p = player;
	while (p && p->id != plid) {
		p = p->next;
	}
	if (p)
		return p;
	return 0;
}

int Bot::GetMoney(int plid) const
{
	players *p = GetPlPtr(plid);
	if (p)
		return p->cash;
	return 0;
}

int Bot::GetRaw(int plid) const
{
	players *p = GetPlPtr(plid);
	if (p)
		return p->raw;
	return 0;
}

int Bot::GetProduction(int plid) const
{
	players *p = GetPlPtr(plid);
	if (p)
		return p->products;
	return 0;
}

int Bot::GetFactories(int plid) const
{
	players *p = GetPlPtr(plid);
	if (p)
		return p->factories;
	return 0;
}

int Bot::GetManufactured(int plid) const
{
	players *p = GetPlPtr(plid);
	if (p) {
		auction *q = prod_auction;
		while (q && q->id != plid) {
			printf("plid=%d, id=%d\n", plid, p->id);
			q = q->next;
		}
		if (q)
			return p->products - q->amount;
	}
	return 0;
}

int Bot::GetResRawSold(int plid) const
{
	auction *q = raw_auction;
	while (q && q->id != plid)
		q = q->next;
	if (q)
		return q->amount;
	return 0;
}

int Bot::GetResRawPrice(int plid) const
{
	auction *q = raw_auction;
	while (q && q->id != plid)
		q = q->next;
	if (q)
		return q->price;
	return 0;
}

int Bot::GetResProdBought(int plid) const
{
	auction *q = prod_auction;
	while (q && q->id != plid)
		q = q->next;
	if (q)
		return q->amount;
	return 0;
}

int Bot::GetResProdPrice(int plid) const
{
	auction *q = prod_auction;
	while (q && q->id != plid)
		q = q->next;
	if (q)
		return q->price;
	return 0;
}
