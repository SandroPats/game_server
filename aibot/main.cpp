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

void MainCycle(Bot &bot, Server &serv, RPNItem *rpn)
{
	VarTable var_table;
	RPNItem *stack = 0;
	RPNItem *r = rpn;
	char *mes;
	while (r) {
		RPNElem *o = r->elem;
		RPNOperator *i = dynamic_cast<RPNOperator*>(o);
		if (i)
			i->SetBotAndServ(&bot, &serv);
		r = r->next;
	}
	int res;
	do {
		do {
			if (0 == (mes = serv.ReceiveMessage()))
				return;
			res = bot.Handle(mes, serv);
			delete[] mes;
		} while (!res && bot.Game());
		r = rpn;
		while (r && bot.Game()) {
			r->elem->Evaluate(&stack, &r, var_table);
		}
	} while (bot.Game());
}

int main(int argc, char **argv)
{
	char *endptr;
	if (argc < 5) {
		fprintf(stderr, "too few arguments\n");
		return 1;
	}
	errno = 0;
	int p = strtoll(argv[3], &endptr, 10);
	if ((errno == ERANGE &&
		(p == LONG_MAX || p == LONG_MIN)) ||
		(errno != 0 && p == 0)) {
		perror("strtoll");
		return 0;
	}
	if (endptr == argv[3]) {
		fprintf(stderr, "Second argument has no digits in it\n");
	}
	FILE *f = fopen(argv[4], "r");
	if (!f) {
		perror(argv[4]);
		return 4;
	}
	SyntAnalyser syntan(f);
	RPNItem *rpn = syntan.Analyse();
	if (!rpn)
		return 5;
	Server serv(argv[1], argv[2]);
	Bot bot(p);
	if (!serv.Connect()) {
		return 2;
	}
	try {
		MainCycle(bot, serv, rpn);
	}
	catch(const RPNEx &ex) {
		fprintf(stderr, "%s:%s\n",ex.GetElem()->GetRPNElemStr() ,ex.GetStr());
	}
	catch(const char *er) {
		fprintf(stderr, "%s\n", er);
	}
	return 0;
}
