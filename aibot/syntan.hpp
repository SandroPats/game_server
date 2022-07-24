#ifndef SYNTAN_HPP_SENTRY
#define SYNTAN_HPP_SENTRY
#include "lexan.hpp"
#include "rpn.hpp"
#include <stdio.h>

class SyntAnalyser {
	LexAnalyser lexan;
	Lexeme *curlex;
	VarTable v;
	LabelTable label_table;
	RPNItem *rpn_last, *rpn_first;
	int strnum;
	FILE *file;
	bool CurrentType(LexType type);
	bool CurrentIs(const char *str);
	int StrNum() const { return curlex ? curlex->strnum : strnum; }
	void Next();
	void P();
	void S();
	void V();
	void Wh();
	void If();
	void Ex();
	void L();
	void O();
	void Buy();
	void Sell();
	void Prod();
	void O1();
	void Print();
	void X();
	void X1();
	void X2();
	void A();
	void B();
	void D();
	void E();
	void K();
	void E1();
	void K1();
	void E2();
	void K2();
	void E3();
	void K3();
	void E4();
	void K4();
	void E5();
	void E6();
	void Var();
	void Func();
	void F();
	void AddToRPN(RPNElem *e);
public:
	SyntAnalyser(FILE *f);
	~SyntAnalyser();
	RPNItem *Analyse();
};

#endif
