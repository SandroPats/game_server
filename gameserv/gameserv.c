#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

enum commands {
	chelp,
	cmarket,
	cplayer,
	cprod,
	cbuy,
	csell,
	cbuild,
	cturn,
	cstart,
	call,
	cto,
	cno
};

enum states {
	start,
	receiving,
	received,
	endturn,
	bankrupt,
	finish,
	error
};

typedef struct tag_marketlvl {
	int raw_am;
	int raw_min_price;
	int prod_am;
	int prod_max_price;
} marketlvl;

typedef struct tag_player {
	int fd;
	int id;
	int ready;
	int raw;
	int products;
	int factories;
	int cash;
	int prod_order;
	int factory_buf[4];
	//char buf[200];
	char *buf;
	enum states state;
	struct tag_player *next;
} player;

typedef struct tag_auction {
	int amount;
	int price;
	player *pl;
	struct tag_auction *next;
} auction;

typedef struct tag_banker {
	int turn;
	int game;
	int ready_players;
	int players;
	int active_players;
	int level;
	marketlvl lvlinf;
	auction *raw_auction;
	auction *prod_auction;
} banker;

const int level_change[5][5] = {
	{4, 4, 2, 1, 1},
	{3, 4, 3, 1, 1},
	{1, 3, 4, 3, 1},
	{1, 1, 3, 4, 3},
	{1, 1, 2, 4, 4}
};

const double lvl_odds_and_prices[5][4] = {
	{1, 800, 3, 6500},
	{1.5, 650, 2.5, 6000},
	{2, 500, 2, 5500},
	{2.5, 400, 1.5, 5000},
	{3, 300, 1, 4500}
};

const char cmd[][7] = {
	"help",
	"market",
	"player",
	"prod",
	"buy",
	"sell",
	"build",
	"turn",
	"start",
	"all",
	"to"
};


const int levels = 5;
const int bmonths = 4;
const int cmd_num = 11;
const int buflen = 200;

int is_int(char *str)
{
	if (*str)
		return *str >= '0' && *str <= '9' && is_int(str+1);
	return 1;
}

int str_to_int(char *str)
{
	int s = 0;
	while (*str && (*str <= '9' && *str >= '0')) {
		s = s*10;
		s += *str - '0';
		str++;
	}
	return s;
}

void add_to_str(char *str, char *buf)
{
	while (*str)
		str++;
	while (*buf) {
		*str = *buf;
		str++;
		buf++;
	}
	*str = 0;
}

int is_divider(int r)
{
	return r == '\r' || r =='\n' || r == '\t' || r == ' ';
}

void write_to_everyone(player *pl_ptr, char *str)
{
	player *pl;
	for (pl = pl_ptr; pl; pl = pl->next)
		if (pl->fd) {
			write(pl->fd, str, strlen(str));
		}
}

void exec_help(int fd)
{
	char s1[] = "\r\'market\': see current market condition\n\r";
	char s2[] = "\'player [id]\': see player's info\n\r";
	char s3[] = "\'prod\': turns 1 raw into 1 product ";
	char st[] = "by the end of the month\n\r";
	char s4[] = "\'buy [amount] [price]\': ";
	char s5[] = "place raw purchase request\n\r";
	char s6[] = "\'sell [amount] [price]\': ";
	char s7[] = "place production sale request\n\r";
	char s8[] = "\'build\': start building a factory\n\r";
	char s9[] = "\'turn\': end all actions for this month\n\r";
	char s10[] = "\'all [text]\': send message to everyone\n\r";
	char s11[] = "\'to [id] [text]\': send a private message\n\r#\n\r";
	char str[512];
	sprintf(str, "%s%s%s%s%s%s%s%s%s%s%s%s",
		s1, s2, s3, st, s4, s5, s6, s7, s8, s9, s10, s11);
	write(fd, str, strlen(str));
}

void exec_market(int fd, player *pl_ptr, banker *bank)
{
	player *pl;
	char s1[] = "\rplayers still active:\n\r%\t\t", str[512];
	char s2[] = "\n\rBank sells:   items\tmin. price\n\r%\t\t";
	char s3[] = "\n\rBank buys:    items\tmax. price\n\r%\t\t";
	int count = 0;
	for (pl = pl_ptr; pl; pl = pl->next)
		if (pl->fd)
			count++;
	sprintf(str, "%s%d%s%d\t%d%s%d\t%d\n\r#\n\r",
		s1, count, s2, bank->lvlinf.raw_am, bank->lvlinf.raw_min_price,
		s3, bank->lvlinf.prod_am, bank->lvlinf.prod_max_price);
	write(fd, str, strlen(str));
}

void exec_player(player *pl_ptr, player *cur_pl)
{
	player *p;
	int id;
	char str2[] = "\rno player with such number\n\r#\n\r";
	char str[512], *r = cur_pl->buf;
	while (*r && (*r > '9' || *r < '0'))
		r++;
	if (*r)
		id = str_to_int(r);
	else
		id = cur_pl->id;
	for (p = pl_ptr; p; p = p->next) {
		if (p->id == id)
			break;
	}
	if (!p) {
		write(cur_pl->fd, str2, sizeof(str2) - 1);
	} else {
		char s[] = "\rinfo:\tcash\traw\tprod\tfactories\n\r%";
		sprintf(str,"%s\t%d\t%d\t%d\t%d\n\r#\n\r",
			s, p->cash, p->raw,
			p->products, p->factories);
		write(cur_pl->fd, str, strlen(str));
	}
}

void tell_bankrupt(player *pl_ptr, player *p)
{
	player *q;
	char s1[] = "player %", s2[] = " went bankrupt!\n\r#\n\r", str[512];
	char b[] = "you went bankrupt!\n\r#\n\r";
	sprintf(str, "%s%d%s", s1 , p->id, s2);
//	printf("%s\n", str);
	for (q = pl_ptr; q; q = q->next)
		if (q != p && q->fd)
			write(q->fd, str, strlen(str));
		else if (q->fd)
			write(q->fd, b, sizeof(b) - 1);
}

void end_turn(player *p)
{
	int i;
	char str[] = "factory has been built!\n\r#\n\r";
	p->state = start;
	p->raw -= p->prod_order;
	p->products += p->prod_order;
	p->cash -= p->raw*300 + p->products*500 +p->factories*1000;
	p->cash -= p->prod_order*2000;
	p->factories += p->factory_buf[bmonths - 1];
	p->cash -= p->factory_buf[bmonths - 1]*2500;
	p->prod_order = 0;
	for (i = 0; i < p->factory_buf[bmonths - 1]; i++)
		write(p->fd, str, sizeof(str) - 1);
	for (i = bmonths - 1; i > 0; i--)
		p->factory_buf[i] = p->factory_buf[i - 1];
	p->factory_buf[0] = 0;
}

void reset_bank(banker *b, player *pl)
{
	b->turn = 1;
	b->game = 1;
	b->level = 3;
	b->lvlinf.raw_am = 2*b->ready_players;
	b->lvlinf.raw_min_price = 500;
	b->lvlinf.prod_am = 2*b->ready_players;
	b->lvlinf.prod_max_price = 5500;
	b->raw_auction = NULL;
	b->prod_auction = NULL;
}

void set_player_list(player *ptr)
{
	int i;
	if (!ptr)
		return;
	for (i = 0; i < bmonths; i++)
		ptr->factory_buf[i] = 0;
	ptr->ready = 0;
	ptr->raw = 4;
	ptr->products = 2;
	ptr->factories = 2;
	ptr->cash = 10000;
	ptr->prod_order = 0;
	if (ptr->fd)
		ptr->state = start;
	else
		ptr->state = bankrupt;
	//ptr->buf = malloc(200);
	*ptr->buf = '\0';
	set_player_list(ptr->next);
}

void endgame_check(player *pl, banker *bank)
{
	player *p;
	int count = 0, id = 0;
	for (p = pl; p; p = p->next)
		if (p->state != bankrupt && p->state != finish) {
			count++;
			id = p->id;
		}
	if (count == 1 || !count) {
		char s1[] = "YOU WIN!!!\n\r#\n\r";
		char s2[] = "Player %", s3[] = " wins the game\n\r#\n\r";
		char s4[] = "IT'S A DRAW!\n\r#\n\r", str[512];
		sprintf(str, "%s%d%s", s2, id, s3);
		bank->active_players = 0;
		for (p = pl; p; p = p->next) {
			if (p->fd) {
				bank->active_players++;
			}
			if (p->id == id && p->fd && count) {
				write(p->fd, s1, sizeof(s1) - 1);
			} else if (p->fd && count) {
				write(p->fd, str, strlen(str) + 1);
			} else if (p->fd) {
				write(p->fd, s4, sizeof(s4) - 1);
			}
		}
		bank->game = 0;
		bank->ready_players = 0;
		set_player_list(pl);
	}
}

int bankrupt_or_endturn(int i)
{
	return i == bankrupt || i == endturn;
}

void change_market_lvl(banker *bank)
{
	int r = 2, i, count = 0, a = bank->active_players;
//	printf("%d\n", a);
	r = 1 + (int)(12.0*rand()/(RAND_MAX+1.0));
	for (i = 0; i < levels && count < r; i++)
		count += level_change[bank->level - 1][i];
	bank->lvlinf.raw_am = (int)(a * lvl_odds_and_prices[i - 1][0]);
	bank->lvlinf.raw_min_price = lvl_odds_and_prices[i - 1][1];
	bank->lvlinf.prod_am = (int)(a * lvl_odds_and_prices[i- 1][2]);
	bank->lvlinf.prod_max_price = lvl_odds_and_prices[i - 1][3];
	bank->level = i;
}

void fulfill_max(auction *ptr, banker *bank, char *str, int max)
{
	player *pl;
	char s[] = "player amount price FULFILLED\n\r%";
	while (ptr && ptr->price == max) {
		pl = ptr->pl;
		pl->cash -= ptr->price*ptr->amount;
		pl->raw += ptr->amount;
		bank->lvlinf.raw_am -= ptr->amount;
		if (ptr->amount)
			sprintf(str + strlen(str),"%s%d\t%d\t%d\n\r",
			s, pl->id, ptr->amount, ptr->price);
		ptr = ptr->next;
	}
}

int raw_lot(auction *ptr, banker *bank, char *str, int max)
{
	auction *p = ptr, *sptr = NULL;
	player *pl = ptr->pl;
	char s[] = "player amount price FULFILLED\n\r%";
	int r, k;
	while (!(r = (int)(2.0*rand()/(RAND_MAX+1.0)))) {
		sptr = p;
		p = p->next;
		if (!p || p->price < max) {
			sptr = NULL;
			p = ptr;
		}
	}
	pl = p->pl;
	if (p->amount <= bank->lvlinf.raw_am) {
		pl->cash -= p->price*p->amount;
		pl->raw += p->amount;
		bank->lvlinf.raw_am -= p->amount;
		k = p->amount;
	} else {
		pl->cash -= p->price*bank->lvlinf.raw_am;
		pl->raw += bank->lvlinf.raw_am;
		k = bank->lvlinf.raw_am;
		bank->lvlinf.raw_am = 0;
	}
	if (p->amount)
		sprintf(str + strlen(str), "%s%d\t%d\t%d\n\r",
			s, pl->id, k, p->price);
	if (sptr) {
		sptr->next = p->next;
		free(p);
		return 0;
	} else {
		return 1;
	}
}

auction *free_and_next(auction *ptr)
{
	auction *p;
	p = ptr;
	ptr = ptr->next;
	free(p);
	return ptr;
}

void raw_auction_hold(banker *bank, player *pl_ptr)
{
	auction *ptr = bank->raw_auction, *p;
	char str[512] = "\rraw auction results:\n\r";
	int max, count = 0, l;
	while (ptr && bank->lvlinf.raw_am) {
		max = ptr->price;
		for (p = ptr; p && p->price == max; p = p->next)
			count += p->amount;
		if (count <= bank->lvlinf.raw_am) {
			fulfill_max(ptr, bank, str, max);
			while (ptr && ptr->price == max)
				ptr = free_and_next(ptr);
			count = 0;
		} else {
			l = raw_lot(ptr, bank, str, max);
			if (l)
				ptr = free_and_next(ptr);
		}
	}
	while (ptr)
		ptr = free_and_next(ptr);
	bank->raw_auction = NULL;
	sprintf(str + strlen(str), "#\n\r");
	write_to_everyone(pl_ptr, str);
}

void fulfill_min(auction *ptr, banker *bank, char *str, int min)
{
	player *pl;
	char s[] = "player amount price FULFILLED\n\r%";
	while (ptr && ptr->price == min) {
		pl = ptr->pl;
		pl->cash += ptr->price*ptr->amount;
		pl->products -= ptr->amount;
		bank->lvlinf.prod_am -= ptr->amount;
		if (ptr->amount)
			sprintf(str + strlen(str),"%s%d\t%d\t%d\n\r",
				s, pl->id, ptr->amount, ptr->price);
		ptr = ptr->next;
	}
}

int prod_lot(auction *ptr, banker *bank, char *str, int min)
{
	auction *p = ptr, *sptr = NULL;
	player *pl = ptr->pl;
	char s[] = "player amount price FULFILLED\n\r%";
	int r, k;
	while (!(r = (int)(2.0*rand()/(RAND_MAX+1.0)))) {
		sptr = p;
		p = p->next;
		if (!p || p->price > min) {
			sptr = NULL;
			p = ptr;
		}
	}
	pl = p->pl;
	if (p->amount <= bank->lvlinf.prod_am) {
		pl->cash += p->price*p->amount;
		pl->products -= p->amount;
		bank->lvlinf.prod_am -= p->amount;
		k = p->amount;
	} else {
		pl->cash += p->price*bank->lvlinf.prod_am;
		pl->products -= bank->lvlinf.prod_am;
		k = bank->lvlinf.prod_am;
		bank->lvlinf.prod_am = 0;
	}
	if (p->amount)
		sprintf(str + strlen(str), "%s%d\t%d\t%d\n\r",
			s, pl->id, k, p->price);
	if (sptr) {
		sptr->next = p->next;
		free(p);
		return 0;
	} else {
		return 1;
	}
}

void prod_auction_hold(banker *bank, player *pl_ptr)
{
	auction *ptr = bank->prod_auction, *p;
	char str[512] = "\rproduct auction results:\n\r";
	int min, count = 0, l;
	while (ptr && bank->lvlinf.prod_am) {
		min = ptr->price;
		for (p = ptr; p && p->price == min; p = p->next)
			count += p->amount;
		if (count <= bank->lvlinf.prod_am) {
			fulfill_min(ptr, bank, str, min);
			while (ptr && ptr->price == min)
				ptr = free_and_next(ptr);
			count = 0;
		} else {
			l = prod_lot(ptr, bank, str, min);
			if (l)
				ptr = free_and_next(ptr);
		}
	}
	while (ptr)
		ptr = free_and_next(ptr);
	bank->prod_auction = NULL;
	sprintf(str + strlen(str), "#\n\r");
	write_to_everyone(pl_ptr, str);
}

void exec_turn(player *pl_ptr, player *cur_pl, banker *bank)
{
	char s[] = "\rturn ended\n\r#\n\r";
	int count = 0;
	player *p;
	cur_pl->state = endturn;
	write(cur_pl->fd, s, sizeof(s) - 1);
	for (p = pl_ptr; p; p = p->next)
		if (!bankrupt_or_endturn(p->state) && p->state != finish)
			count++;
	if (!count) {
		char s[] = "\rcurrent month is %", *n = "th\n\r#\n\r", str[512];
		bank->turn++;
		raw_auction_hold(bank, pl_ptr);
		prod_auction_hold(bank, pl_ptr);
		sprintf(str, "%s%d%s", s , bank->turn, n);
		for (p = pl_ptr; p; p = p->next) {
			if (p->fd)
				write(p->fd, str, strlen(str));
			if (p->state != bankrupt)
				end_turn(p);
			if (p->cash <= 0 && p->state != bankrupt) {
				p->state = bankrupt;
				bank->active_players--;
				tell_bankrupt(pl_ptr, p);
			}
		}
		change_market_lvl(bank);
	}
}

void exec_produce(player *p)
{
	char *b = p->buf;
	int am;
	while ((*b > '9' || *b < '0') && *b)
		b++;
	if (!*b) {
		char str[] = "\rplease specify the amount\n\r#\n\r";
		write(p->fd, str, sizeof(str) - 1);
		return;
	} else if (*b == '0') {
		char str[] = "\rthe amount must be more then zero\n\r#\n\r";
		write(p->fd, str, sizeof(str) - 1);
		return;
	} else {
		am = str_to_int(b);
	}
	if (p->factories  < am + p->prod_order) {
		char str[] = "\rnot enough factories\n\r#\n\r";
		write(p->fd, str, sizeof(str) - 1);
	} else if (p->raw < am + p->prod_order) {
		char str[] = "\rnot enough raw\n\r#\n\r";
		write(p->fd, str, sizeof(str) - 1);
	} else if (p->cash < 2000*(am + p->prod_order)) {
		char str[] = "\rnot enough cash\n\r#\n\r";
		write(p->fd, str, sizeof(str) - 1);
	} else {
		char s[] = "\rproducts ordered:\n\r%\t", str[512];
		p->prod_order += am;
		sprintf(str, "%s%d\n\r#\n\r", s, p->prod_order);
		write(p->fd, str, strlen(str));
	}
}

void exec_build(player *p)
{
	int count = 0, i, *buf = p->factory_buf;
	char s[] = "\rfactories under construction:\n\r%\t", str[512];
	for (i = 0; i < bmonths; i++)
		count += buf[i];
	if (p->cash < 2500*count) {
		char str[] = "\rnot enough cash\n\r#\n\r";
		write(p->fd, str, sizeof(str) - 1);
		return;
	}
	buf[0]++;
	p->cash -= 2500;
	count++;
	sprintf(str,"%s%d\n\r#\n\r", s, count);
	write(p->fd, str, strlen(str));
}

int *get_param(player *pl)
{
	int *apl = malloc(2*sizeof(int)), i = 0, intf = 0;
	char *r = pl->buf;
	while (*r) {
		if (*r <= '9' && *r >= '0' && !intf && i < 2) {
			apl[i] = str_to_int(r);
			i++;
			intf = !intf;
		} else if (*r <= '9' && *r >= '0' && !intf){
			char str[] = "\rtoo many arguments\n\r#\n\r";
			write(pl->fd, str, sizeof(str) - 1);
			free(apl);
			return NULL;
		} else  if (*r > '9' || *r < '0'){
			intf = 0;
		}
		r++;
	}
	if (i < 2) {
		char str[] = "\rtoo few arguments\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		free(apl);
		return NULL;
	}
	return apl;
}

auction *create_auction_node(int *apl, player *pl)
{
	auction *ptr;
	ptr = malloc(sizeof(auction));
	ptr->amount = apl[0];
	ptr->price = apl[1];
	ptr->pl = pl;
	ptr->next = NULL;
	return ptr;
}

int aplied(player *pl, auction *auc)
{
	auction *ptr;
	for (ptr = auc; ptr; ptr = ptr->next)
		if (ptr->pl == pl) {
			char str[] = "\ryou've already applied\n\r#\n\r";
			write(pl->fd, str, sizeof(str) - 1);
			return 1;
		}
	return 0;
}

int sell_param_check(player *pl, banker *bank, int *apl)
{	
	if (apl[0] > pl->products) {
		char str[] = "\rnot enough products\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		free(apl);
		return 0;
	} else if (apl[0] > bank->lvlinf.prod_am) {
		char str[] = "\rbank can't buy so many products\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		free(apl);
		return 0;
	} else if (apl[1] > bank->lvlinf.prod_max_price || apl[1] < 0) {
		char str[] = "\rprice is too high\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		free(apl);
		return 0;
	}
	return 1;
}

void exec_sell(player *pl, banker *bank)
{
	int *apl;
	auction *ptr, *prod = bank->prod_auction, *prev = NULL;
	if (aplied(pl, bank->prod_auction))
		return;
	apl = get_param(pl);
	if (!apl || !sell_param_check(pl, bank, apl))
		return;
	ptr = create_auction_node(apl, pl);
	if (!prod)
		bank->prod_auction = ptr;
	while (prod) {
		if (ptr->price < prod->price) {
			ptr->next = prod;
			if (prev)
				prev->next = ptr;
			else
				bank->prod_auction = ptr;
			break;
		} else if (!prod->next) {
			prod->next = ptr;
			break;
		}
		prev = prod;
		prod = prod->next;
	}
	char s[] = "\rSale request accepted\n\r#\n\r";
	write(pl->fd, s, sizeof(s) - 1);
	free(apl);
}

int buy_param_check(player *pl, banker *bank, int *apl)
{	
	if (apl[0] > bank->lvlinf.raw_am) {
		char str[] = "\rnot enough raw in bank\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		return 0;
	} else if (apl[1] < bank->lvlinf.raw_min_price) {
		char str[] = "\rprice is too low\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		return 0;
	} else if (apl[1]*apl[0] > pl->cash) {
		char str[] = "\rnot enough cash\n\r#\n\r";
		write(pl->fd, str, sizeof(str) - 1);
		return 0;
	}
	return 1;
}

void exec_buy(player *pl, banker *bank)
{	
	int *apl;
	auction *ptr, *raw = bank->raw_auction, *prev = NULL;
	if (aplied(pl, bank->raw_auction))
		return;
	apl = get_param(pl);
	if (!apl || !buy_param_check(pl, bank, apl))
		return;
	ptr = create_auction_node(apl, pl);
	if (!raw)
		bank->raw_auction = ptr;
	while (raw) {
		if (ptr->price > raw->price) {
			ptr->next = raw;
			if (prev)
				prev->next = ptr;
			else
				bank->raw_auction = ptr;
			break;
		} else if (!raw->next) {
			raw->next = ptr;
			break;
		}
		prev = raw;
		raw = raw->next;
	}
	char s[] = "\rPurchase request accepted\n\r#\n\r";
	write(pl->fd, s, sizeof(s) - 1);
	free(apl);
}

void exec_no(player *cur_pl)
{
	char str1[] = "\renter 'help' for the list of available commands\n\r#\n\r";
	char str2[] = "\ravailable commands:", str[256];
	char str3[] = "'help', 'market', 'player', 'all' , 'to'\n\r#\n\r";
	if (bankrupt_or_endturn(cur_pl->state)) {
		sprintf(str, "%s%s", str2, str3);
		write(cur_pl->fd, str, strlen(str));
	} else {
		write(cur_pl->fd, str1, sizeof(str1) - 1);
	}
}

void exec_start(player *pl_ptr, player *cur_pl, banker *bank)
{
	char str[256] = "\rplayers ready:\n\r%\t";
	if (bank->game) {
		char s[] = "\rThe game has already begun\n\r#\n\r";
		write(cur_pl->fd, s, sizeof(s) - 1);
		return;
	}
	if (!cur_pl->ready) {
		cur_pl->ready = 1;
		bank->ready_players++;
		sprintf(str + strlen(str), "%d\n\r#\n\r", bank->ready_players);
	write_to_everyone(pl_ptr, str);
	} else {
		char s[] = "you've already entered \'start\'\n\r#\n\r";
		write(cur_pl->fd, s, sizeof(s) - 1);
	}
}

void exec_all(player *cur_pl, player *pl_ptr)
{
	player *p;
	char str[256];
	char *s = cur_pl->buf;
	s = s + 3;
	sprintf(str, "from player %d:%s\n\r#\n\r", cur_pl->id, s);
	for (p = pl_ptr; p; p= p->next)
		if (p->fd && p != cur_pl)
			write(p->fd, str, strlen(str));
}

void exec_to(player *cur_pl, player *pl_ptr)
{
	player *p;
	char str[256];
	char *s = cur_pl->buf + 2;
	int id;
	while (*s && *s == ' ')
		s++;
	if (*s > '9' || *s < '0') {
		char str[] = "\rinvalid parameters\n\r";
		write(cur_pl->fd, str, sizeof(str) - 1);
		return;
	}
	id = str_to_int(s);
	for (p = pl_ptr; p; p = p->next)
		if (p->id && p->id == id)
			break;
	if (!p) {
		char str[] = "\rno player with such id\n\r";
		write(cur_pl->fd, str, sizeof(str) - 1);
		return;
	}
	while (*s >= '0' && *s <= '9')
		s++;
	sprintf(str, "(private)from player %d: %s\n\r#\n\r", cur_pl->id, s);
	write(p->fd, str, strlen(str));
}

void exec_request(player *pl_ptr, player *cur_pl, int i, banker *bank)
{
	switch(i) {
	case chelp:
		exec_help(cur_pl->fd);
		break;
	case cmarket:
		exec_market(cur_pl->fd, pl_ptr, bank);
		break;
	case cplayer:
		exec_player(pl_ptr, cur_pl);
		break;
	case cprod:
		exec_produce(cur_pl);
		break;
	case cbuy:
		exec_buy(cur_pl, bank);
		break;
	case csell:
		exec_sell(cur_pl, bank);
		break;
	case cbuild:
		exec_build(cur_pl);
		break;
	case cturn:
		exec_turn(pl_ptr, cur_pl, bank);
		break;
	case cstart:
		exec_start(pl_ptr, cur_pl, bank);
		break;
	case call:
		exec_all(cur_pl, pl_ptr);
		break;
	case cto:
		exec_to(cur_pl, pl_ptr);
		break;
	case cno:
		exec_no(cur_pl);
		break;
	}
	if (cur_pl->state != receiving)
		*cur_pl->buf = 0;
}

int market_help_player_all_to(int i)
{
	return i == cmarket || i == chelp ||
		i == cplayer || i == call || i == cto;
}

int identify_request(player *cur_pl, banker *bank)
{
	int i;
	char *r = cur_pl->buf;
	const char **c;
	if (cur_pl->state == receiving || cur_pl->state == error||!*cur_pl->buf)
		return -1;
	else if (cur_pl->state == start)
		return cmd_num;
	c = malloc(cmd_num*sizeof(char*));
	for (i = 0; i < cmd_num; i++)
		c[i] = cmd[i];
	while (*r && *r == ' ')
		r++;
	while (*r) {
		for (i = 0; i < cmd_num; i++) {
			if (c[i] && *r != *c[i] && *c[i])
				c[i] = NULL;
			if (c[i] && *c[i])
				c[i]++;
		}
		r++;
	}
	for (i = 0; i < cmd_num; i++)
		if (c[i])
			break;
	free(c);
	if (!market_help_player_all_to(i) && bankrupt_or_endturn(cur_pl->state))
		return cmd_num;
	return i;
}

int error_bankrupt_or_endturn(int i)
{
	return i == error || i == bankrupt || i == endturn;
}

char *serv_stat(banker *bank)
{
	char strc[] = "players connected:\n\r%\t";
	char strr[] = "players ready:\n\r%\t";
	char stra[] = "player slots available:\n\r%\t", *str;
	int a;
	a = bank->players - bank->active_players;
	str = malloc(512);
	sprintf(str, "%s%d\n\r%s%d\n\r%s%d\n\r#\n\r",
		strc, bank->active_players , strr, bank->ready_players, stra,a);
	return str;
}

void pl_shutdown(player *cur_pl, player *pl, banker *bank)
{
	char s[256];
	shutdown(cur_pl->fd, 2);
	close(cur_pl->fd);
//	printf("disconnected %d\n", cur_pl->id);
	cur_pl->fd = 0;
	if (cur_pl->state != bankrupt) {
		bank->active_players--;
		cur_pl->state = bankrupt;
		if (bank->game)
			sprintf(s, "player %% %d went bankrupt!\n\r#\n\r", cur_pl->id);
		else {
			char *st;
			if (cur_pl->ready) {
				cur_pl->ready = 0;
				bank->ready_players--;
			}
			st = serv_stat(bank);
			sprintf(s, "player has disconnected\n\r%s",st);
			free(st);
		}
	//	write_to_everyone(pl, s);
	}
}

void change_status(player *pl, char *req, int rc)
{
	if (rc >= buflen - strlen(pl->buf) - 1 && pl->state != error)
		pl->state = error;
	else if ((req[rc - 1] == '\n' || req[rc -2] == '\n') &&
		!error_bankrupt_or_endturn(pl->state)) {
		pl->state = received;
	} else if (req[rc - 1] == '\n' && !bankrupt_or_endturn(pl->state))
		pl->state = start;
}

void read_request(player *pl, player *pl_ptr, banker *bank)
{
	int rc;
	char req[buflen];
	if (!error_bankrupt_or_endturn(pl->state))
		pl->state = receiving;
	rc = read(pl->fd, req, sizeof(req) - 1);
	if (!rc)
		pl_shutdown(pl, pl_ptr, bank);
	else
		change_status(pl, req, rc);
	req[rc] = '\0';
	/*if (pl->state != error)
		sprintf(pl->buf + strlen(pl->buf), "%s", req);
	if (pl->state != error && strlen(req) < rc - 1) {
		sprintf(pl->buf + strlen(pl->buf), "#%s", req+ strlen(req)+1);
	}*/
	if (pl->state != error) {
		sprintf(pl->buf + strlen(pl->buf), "%s", req);
		while (rc && strlen(req) < rc -1) {
			sprintf(pl->buf + strlen(pl->buf), "#%s",
				req + strlen(req)+1);
			*(req + strlen(req)) = '#';
		}
	}
}

void accept_request(int ls, player *pl_ptr, banker *bank)
{	
	player *p;
	int fd;
	char str[] = "server reached its max capacity\n\r", *s, st[256];
	for (p = pl_ptr; p; p = p->next) {
		if (!p->fd) {
			p->fd = accept(ls, NULL, NULL);
			p->id = p->fd - ls;
			if (!bank->game) {
				p->state = start;
				bank->active_players++;
			}
			sprintf(st, "your id is %%%d\n\r#\n\r", p->id);
			write(p->fd, st, strlen(st));
			break;
		}
	}
	if (!p) {
		fd = accept(ls, NULL, NULL);
		write(fd, str, sizeof(str) - 1);
		shutdown(fd, 2);
		close(fd);
	} else if (p->fd == -1) {
		perror("accept");
		shutdown(p->fd, 2);
		close(p->fd);
		p->fd = 0;
	} else if (!bank->game) {
		s = serv_stat(bank);
		write_to_everyone(pl_ptr, s);
		free(s);
	}
}

void all_players(player *pl_ptr, banker *bank)
{
	int a = bank->active_players, r = bank->ready_players;
	char str[] ="\rThe game has begun\n\r#\n\r" ;
	if (a && a == r) {
		write_to_everyone(pl_ptr, str);
		reset_bank(bank, pl_ptr);
	}
}

void reject_request(player *pl, player *pl_ptr)
{
	char str1[] = "\ravailable commands: 'help','start','all','to'\n\r#\n\r";
	*pl->buf = '\0';
	if (pl->fd)
		write(pl->fd, str1, sizeof(str1) -1);
}

player *create_player_list(int players)
{
	int i;
	player *ptr;
	if (players) {
		ptr = malloc(sizeof(player));
		for (i = 0; i < bmonths; i++)
			ptr->factory_buf[i] = 0;
		ptr->fd = 0;
		ptr->id = 0;
		ptr->ready = 0;
		ptr->raw = 4;
		ptr->products = 2;
		ptr->factories = 2;
		ptr->cash = 10000;
		ptr->prod_order = 0;
		ptr->state = bankrupt;
		ptr->buf = malloc(200);
		*ptr->buf = '\0';
		ptr->next = create_player_list(players - 1);
		return ptr;
	} else {
		return NULL;
	}
}

int help_all_to_or_start(int i)
{
	return i == call || i == cto || i == cstart || i == chelp;
}

void set_bank(banker *b, int players)
{
	b->game = 0;
	b->players = players;
	b->active_players = 0;
	b->ready_players = 0;
}

int error_or_receiving(int i)
{
	return i == receiving || i == error;
}

void execute_requests(player *p, player *pl_ptr, banker *bank)
{
	int res;
	int end = 0;
	char *b = p->buf, *q = p->buf;
	while (1) {
		while (*b && *b != '#')
			b++;
		if (!*b)
			end = 1;
		*b = '\0';
		res = identify_request(p, bank);
		if (bank->game || help_all_to_or_start(res)) {
			exec_request(pl_ptr, p, res, bank);
		} else if (!error_or_receiving(p->state)) {
			reject_request(p, pl_ptr);
		}
		if (end)
			break;
		p->buf = b + 1;
		b++;
	}
	p->buf = q;
}

void check_requests(int ls, int players)
{
	banker *bank;
	player *pl_ptr, *p;
	int res, max_d = ls;
	pl_ptr = create_player_list(players);
	bank = malloc(sizeof(banker));
	set_bank(bank, players);
	for (;;) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(ls, &readfds);
		max_d = ls;
		for (p = pl_ptr; p; p = p->next) {
			if (p->fd)
				FD_SET(p->fd, &readfds);
			if (p->fd > max_d)
				max_d = p->fd;
		}
		res = select(max_d + 1, &readfds, NULL, NULL, NULL);
		if (res < 1) {
			perror("select");
			exit(1);
		}
		if (FD_ISSET(ls, &readfds))
			accept_request(ls, pl_ptr, bank);
		for (p = pl_ptr; p; p = p->next)
			if (FD_ISSET(p->fd, &readfds)) {
				read_request(p, pl_ptr, bank);
				execute_requests(p, pl_ptr, bank);
	/*			res = identify_request(p, bank);
				if (bank->game || help_all_to_or_start(res)) {
					exec_request(pl_ptr, p, res, bank);
				} else if (!error_or_receiving(p->state)) {
					reject_request(p, pl_ptr);
				}*/
			}
		if (!bank->game)
			all_players(pl_ptr, bank);
		else
			endgame_check(pl_ptr, bank);
	}
}

int command_line_check(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "not enough arguments\n");
		return 1;
	} else if (!is_int(argv[1]) || !is_int(argv[2])) {
		fprintf(stderr, "invalid command line arguments\n");
		return 1;
	}
	return 0;
}

int main (int argc, char **argv)
{
	int ls, port, players, opt = 1;
	struct sockaddr_in addr;
	if (command_line_check(argc, argv))
		return 1;
	players = str_to_int(argv[1]);
	port = str_to_int(argv[2]);
	ls = socket(AF_INET, SOCK_STREAM, 0);
	if (ls == -1) {
		perror("socket");
		return 2;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (-1 == bind(ls, (struct sockaddr*) &addr, sizeof(addr))) {
		perror("bind");
		return 3;
	}
	if (-1 == listen(ls, 15)) {
		perror("listen");
		return 4;
	}
	check_requests(ls, players);
	return 0;
}
