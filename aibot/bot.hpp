#ifndef BOT_HPP_SENTRY
#define BOT_HPP_SENTRY
#include "rpn.hpp"

class RPNItem;

class Server {
	int port, sfd, curindex;
	char *ip, *portstr, buf[1024];
	int Read();
public:
	Server(char *ipstr, char *pstr);
	bool Connect();
	char *ReceiveMessage();
	void Start() const;
	void Buy(int amount, int price) const;
	void Sell(int amount, int price) const;
	void Turn() const;
	void Player(int id) const;
	void Market() const;
	void Produce(int amount) const;
	void Build() const;
	~Server();
};

struct players {
	int id, raw, products, factories, cash;
	players *next;
};

class Bot {
	enum Commands {
		no,
		cbuy,
		csell,
		cprod,
		cbuild,
		cstart,
		cturn,
		cmarket,
		cplayer,
		crpn
	};
	struct  marketinfo {
		int raw_am, raw_min_price, prod_am, prod_max_price;
	};
	struct banker {
		int turn, players;
		marketinfo market;
	};
	struct auction {
		int amount;
		int price;
		int id;
		auction *next;
	};
	auction *raw_auction, *prod_auction;
	players *player, bot;
	banker *bank;
	int id, plid, winnerid, game, neededplrs;
	int HandleMessage(char *mes);
	int GetConnectedPlayers(char *mes);
	void Send(const Server &serv, int cmd);
	void GetPlayerInfo(char *mes);
	void GetReadyPlayers(char *mes);
	void GetBotInfo();
	void GetMarketInfo(char *mes);
	void GetProductAuctionResults(char *mes);
	void GetRawAuctionResults(char *mes);
	void GetWinnerId(char *mes);
	players *GetPlPtr(int plid) const;
public:
	Bot(int plrs);
	~Bot();
	bool Game() { return game; }
	void DeletePlayersList();
	void DeleteAuctionLists();
	bool Handle(char *mes, const Server &serv);
	int GetMoney(int plid) const;
	int GetRaw(int plid) const;
	int GetProduction(int plid) const;
	int GetFactories(int plid) const;
	int GetDemand() const { return bank->market.prod_am;}
	int GetSupply() const { return bank->market.raw_am;}
	int GetProdPrice() const { return bank->market.prod_max_price;}
	int GetRawPrice() const { return bank->market.raw_min_price;}
	int GetActive() const { return bank->players;}
	int GetTurn() const { return bank->turn;}
	int GetBotId() const { return bot.id;}
	int GetManufactured(int plid) const;
	int GetResRawSold(int plid) const;
	int GetResRawPrice(int plid) const;
	int GetResProdBought(int plid) const;
	int GetResProdPrice(int plid) const;
	void PrintInfo();
};

#endif
