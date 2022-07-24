#include "lexan.hpp"
#include "syntan.hpp"
#include "rpn.hpp"
#include "bot.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SyntaxError::SyntaxError(const char *s, int snum)
{
	str = new char[strlen(s) + 1];
	strcpy(str, s);
	strnum = snum;
}

SyntaxError::SyntaxError(const SyntaxError &E)
{
	str = new char[strlen(E.str) + 1];
	strcpy(str, E.str);
	strnum = E.strnum;
}

SyntaxError::~SyntaxError()
{
	delete[] str;
}

SyntAnalyser::SyntAnalyser(FILE *f)
{
	file = f;
	curlex = 0;
	strnum = 0;
	rpn_last = new RPNItem;
	rpn_last->elem = new RPNNoOp;
	rpn_last->next = 0;
	rpn_first = 0;
}

SyntAnalyser::~SyntAnalyser()
{
	fclose(file);
}

void SyntAnalyser::Next()
{
	char *er;
	int c;
	while ((c = fgetc(file)) != EOF) {
		curlex = lexan.FeedChar(c);
		if (curlex) {
			strnum = curlex->strnum;
			break;
		}
	}
	if (c == EOF) {
		if ((er = lexan.ErrorCheck()))
			fprintf(stderr, "%s\n", er);
		curlex = 0;
	}
}

bool SyntAnalyser::CurrentType(LexType type)
{
	return curlex && curlex->type == type;
}

bool SyntAnalyser::CurrentIs(const char *str)
{
	if (!curlex)
		return false;
	char *s = curlex->str;
	while (*str && *s) {
		if (*s != *str)
			return false;
		str++;
		s++;
	}
	return !((*s && !*str) || (*str && !*s));
}

RPNItem *SyntAnalyser::Analyse()
{
	Next();
	try {
		P();
		RPNItem *r = rpn_first;
		while (r) {
			RPNElem *o = r->elem;
			RPNLabel *i = dynamic_cast<RPNLabel*>(o);
			if (i)
				i->SetLab(label_table);
			r = r->next;
		}
	}
	catch (const SyntaxError &Err) {
		fprintf(stderr, "line %d: %s\n",
			Err.GetStrNum(), Err.GetStr());
		return 0;
	}
	catch (const RPNEx &ex) {
		fprintf(stderr, "%s\n", ex.GetStr());
		return 0;
	}
	return rpn_first;
}

void SyntAnalyser::P()
{
	if (CurrentType(lex_label)) {
		AddToRPN(new RPNNoOp);
		label_table.AddToTable(curlex->str, rpn_last);
		Next();
		if (!CurrentIs(":"))
			throw SyntaxError("missing ':' after the label",
				StrNum());
		Next();
		S();
		P();
	} else if (curlex) {
		S();
		P();
	}
}

void SyntAnalyser::S()
{
	if (CurrentType(lex_var)) {
		V();
		A();
	} else if (CurrentIs("while")) {
		Next();
		Wh();
	} else if (CurrentIs("if")) {
		Next();
		If();
	} else if (CurrentIs("goto")) {
		Next();
		if (!CurrentType(lex_label))
			throw SyntaxError("expected label after 'goto'",
				StrNum());
		RPNLabel *lab = new RPNLabel(0, curlex->str);
		AddToRPN(lab);
		AddToRPN(new RPNJump);
		Next();
		if (!CurrentIs(";"))
			throw SyntaxError("expected ';' after the label",
				StrNum());
		Next();
	} else {
		O();
	}
}

void SyntAnalyser::Wh()
{
	RPNItem *before = rpn_last;
	Ex();
	RPNLabel *lab1 = new RPNLabel;
	AddToRPN(lab1);
	AddToRPN(new RPNJumpFalse);
	B();
	RPNLabel *lab2 = new RPNLabel(before->next);
	AddToRPN(lab2);
	AddToRPN(new RPNJump);
	AddToRPN(new RPNNoOp);
	lab1->Set(rpn_last);
}

void SyntAnalyser::If()
{
	Ex();
	RPNLabel *lab1 = new RPNLabel;
	AddToRPN(lab1);
	AddToRPN(new RPNJumpFalse);
	B();
	RPNLabel *lab2 = new RPNLabel;
	AddToRPN(lab2);
	AddToRPN(new RPNJump);
	AddToRPN(new RPNNoOp);
	lab1->Set(rpn_last);
	L();
	AddToRPN(new RPNNoOp);
	lab2->Set(rpn_last);
}

void SyntAnalyser::V()
{
	char *id = curlex->str;
	Next();
	if (CurrentIs("[")) {
		Next();
		E();
		AddToRPN(new RPNOpSubscript(id));
		if (!CurrentIs("]"))
			throw SyntaxError(
				"expected ']' after the expression", StrNum());
		Next();
	} else {
		AddToRPN(new RPNVar(id));
	}
}

void SyntAnalyser::Ex()
{
	if (!CurrentIs("("))
		throw SyntaxError(
			"expected '(' before the expression",
			StrNum());
	Next();
	E();
	if (!CurrentIs(")"))
		throw SyntaxError(
			"expected ')'after the expression",
			StrNum());
	Next();
}

void SyntAnalyser::L()
{
	if (CurrentIs("else")) {
		Next();
		B();
	}
}

void SyntAnalyser::O()
{
	if (CurrentIs("buy")) {
		Next();
		Buy();
	} else if (CurrentIs("sell")) {
		Next();
		Sell();
	} else if (CurrentIs("prod")) {
		Next();
		Prod();
	} else {
		O1();
	}
}

void SyntAnalyser::Buy()
{
	E();
	if (!CurrentIs(","))
		throw SyntaxError(
			"buy:expected ',' after first expression",
			StrNum());
	Next();
	E();
	if (!CurrentIs(";"))
		throw SyntaxError("expected ';' after 'buy'",
			StrNum());
	AddToRPN(new RPNBuy);
	Next();
}

void SyntAnalyser::Sell()
{
	E();
	if (!CurrentIs(","))
		throw SyntaxError(
			"sell:expected ',' after first expression",
			StrNum());
	Next();
	E();
	if (!CurrentIs(";"))
		throw SyntaxError("expected ';' after 'sell'",
			StrNum());
	AddToRPN(new RPNSell);
	Next();
}

void SyntAnalyser::Prod()
{
	E();
	if (!CurrentIs(";"))
		throw SyntaxError("expected ';' after 'prod'",
			StrNum());
	AddToRPN(new RPNProd);
	Next();
}

void SyntAnalyser::O1()
{
	if (CurrentIs("build")) {
		Next();
	//	E();
		if (!CurrentIs(";"))
			throw SyntaxError("expected ';' after 'build'",
				StrNum());
		AddToRPN(new RPNBuild);
		Next();
	} else if (CurrentIs("print")) {
		Next();
		Print();
	} else if (CurrentIs("endturn")) {
		Next();
		if (!CurrentIs(";"))
			throw SyntaxError("expected ';' after 'endturn'",
				StrNum());
		AddToRPN(new RPNEndturn);
		Next();
	} else {
		throw SyntaxError("unable to identify the statement",
			StrNum());
	}
}

void SyntAnalyser::Print()
{
	if (!CurrentIs("("))
		throw SyntaxError("expected '(' after 'print'",
			StrNum());
	Next();
	X();
	if (!CurrentIs(")"))
		throw SyntaxError(
			"expected ')' after after 'print(...'",
			StrNum());
	Next();
	if (!CurrentIs(";"))
		throw SyntaxError("expected ';' after 'print(...)'",
			StrNum());
	AddToRPN(new RPNPrint);
	Next();
}

void SyntAnalyser::X()
{
	if (CurrentType(lex_literal)) {
		AddToRPN(new RPNString(curlex->str));
		Next();
		X1();
	} else {
		E();
		X1();
	}
}

void SyntAnalyser::X1()
{
	if (CurrentIs(",")) {
		Next();
		X2();
	}
}

void SyntAnalyser::X2()
{
	if (CurrentType(lex_literal)) {
		AddToRPN(new RPNString(curlex->str));
		Next();
		X1();
	} else {
		E();
		X1();
	}
}

void SyntAnalyser::A()
{
	if (CurrentType(lex_assign)) {
		Next();
		E();
		if (!CurrentIs(";"))
			throw SyntaxError("expected ';' after assignment",
				StrNum());
		AddToRPN(new RPNAssign);
		Next();
	} else
		throw SyntaxError("expected assignment after variable",
			StrNum());
}

void SyntAnalyser::B()
{
	if (CurrentIs("{")) {
		Next();
		D();
		if (!CurrentIs("}"))
			throw SyntaxError("expected '}'", StrNum());
		Next();
	} else if (CurrentIs(";")) {
		Next();
	} else if (curlex) {
		S();
	}
}

void SyntAnalyser::D()
{
	if (CurrentType(lex_label)) {
		AddToRPN(new RPNNoOp);
		label_table.AddToTable(curlex->str, rpn_last);
		Next();
		if (!CurrentIs(":"))
			throw SyntaxError("missing ':' after the label",
				StrNum());
		Next();
		S();
		D();
	} else if (curlex && !CurrentIs("}")) {
		S();
		D();
	}

}

void SyntAnalyser::E()
{
	E1();
	K();
}

void SyntAnalyser::K()
{
	if (CurrentIs("|")) {
		Next();
		E1();
		AddToRPN(new RPNOpOr);
		K();
	}
}

void SyntAnalyser::E1()
{
	E2();
	K1();
}

void SyntAnalyser::K1()
{
	if (CurrentIs("&")) {
		Next();
		E2();
		AddToRPN(new RPNOpAnd);
		K1();
	}
}

void SyntAnalyser::E2()
{
	E3();
	K2();
}

void SyntAnalyser::K2()
{
	if (CurrentIs("=")) {
		Next();
		E3();
		AddToRPN(new RPNOpEq);
		K2();
	} else if (CurrentIs("<")) {
		Next();
		E3();
		AddToRPN(new RPNOpLess);
		K2();
	} else if (CurrentIs(">")) {
		Next();
		E3();
		AddToRPN(new RPNOpMore);
		K2();
	}
}

void SyntAnalyser::E3()
{
	E4();
	K3();
}

void SyntAnalyser::K3()
{
	if (CurrentIs("+")) {
		Next();
		E4();
		AddToRPN(new RPNOpPlus);
		K3();
	} else if (CurrentIs("-")) {
		Next();
		E4();
		AddToRPN(new RPNOpMinus);
		K3();
	}
}

void SyntAnalyser::E4()
{
	E5();
	K4();
}

void SyntAnalyser::K4()
{
	if (CurrentIs("*")) {
		Next();
		E5();
		AddToRPN(new RPNOpMult);
		K4();
	} else if (CurrentIs("/")) {
		Next();
		E5();
		AddToRPN(new RPNOpDiv);
		K4();
	} else if (CurrentIs("%")) {
		Next();
		E5();
		AddToRPN(new RPNOpMod);
		K4();
	}
}

void SyntAnalyser::E5()
{
	if (CurrentIs("!")) {
		Next();
		E6();
		AddToRPN(new RPNOpNot);
	} else if (CurrentIs("-")) {
		Next();
		E6();
		AddToRPN(new RPNOpUnaryMinus);
	} else if (CurrentIs("(")) {
		Next();
		E();
		if (!CurrentIs(")"))
			throw SyntaxError("brackets unballanced",
				StrNum());
		Next();
	} else {
		E6();
	}
}

void SyntAnalyser::E6()
{
	if (CurrentType(lex_var)) {
	//	Next();
		Var();
	} else if (CurrentType(lex_int)) {
		AddToRPN(new RPNInt(atoi(curlex->str)));
		Next();
	} else {
		Func();
	}
/*	} else if (CurrentType(lex_func)) {
		char *id = curlex->str;
		Next();
		F();
		AddToRPN(new RPNFunction(id));
	} else if (!CurrentType(lex_delim)) {
		throw SyntaxError("unknown identifier met", StrNum());
	} else {
		throw SyntaxError("Invalid expression", StrNum());
	}*/
}

void SyntAnalyser::Var()
{
	char *id = curlex->str;
	Next();
	if (CurrentIs("[")) {
		Next();
		E();
		AddToRPN(new RPNVarVal(id, true));
		if (!CurrentIs("]"))
			throw SyntaxError(
				"expected ']' after the expression", StrNum());
		Next();
	} else {
		AddToRPN(new RPNVarVal(id, false));
	}
}

void SyntAnalyser::Func()
{
	if (CurrentIs("?money")) {
		Next();
		F();
		AddToRPN(new RPNFuncMoney);
	} else if (CurrentIs("?raw")) {
		Next();
		F();
		AddToRPN(new RPNFuncRaw);
	} else if (CurrentIs("?demand")) {
		Next();
		AddToRPN(new RPNFuncDemand);
	} else if (CurrentIs("?supply")) {
		Next();
		AddToRPN(new RPNFuncSupply);
	} else if (CurrentIs("?production_price")) {
		Next();
		F();
		AddToRPN(new RPNFuncProdPrice);
	} else if (CurrentIs("?production")) {
		Next();
		F();
		AddToRPN(new RPNFuncProduction);
	} else if (CurrentIs("?factories")) {
		Next();
		F();
		AddToRPN(new RPNFuncFactories);
	} else if (CurrentIs("?active_players")) {
		Next();
		AddToRPN(new RPNFuncActive);
	} else if (CurrentIs("?turn")) {
		Next();
		AddToRPN(new RPNFuncTurn);
	} else if (CurrentIs("?raw_price")) {
		Next();
		AddToRPN(new RPNFuncRawPrice);
	} else if (CurrentIs("?my_id")) {
		Next();
		AddToRPN(new RPNFuncMyId);
	} else if (CurrentIs("?manufactured")) {
		Next();
		F();
		AddToRPN(new RPNFuncManufactured);
	} else if (CurrentIs("?result_raw_sold")) {
		Next();
		F();
		AddToRPN(new RPNFuncResRawSold);
	} else if (CurrentIs("?result_raw_price")) {
		Next();
		F();
		AddToRPN(new RPNFuncResRawPrice);
	} else if (CurrentIs("?result_prod_bought")) {
		Next();
		F();
		AddToRPN(new RPNFuncResProdBought);
	} else if (CurrentIs("?result_prod_price")) {
		Next();
		F();
		AddToRPN(new RPNFuncResProdPrice);
	} else if (CurrentType(lex_func)) {
		throw SyntaxError("unknown function identifier met", StrNum());
	} else if (!CurrentType(lex_delim)) {
		throw SyntaxError("unknown identifier met", StrNum());
	} else {
		throw SyntaxError("Invalid expression", StrNum());
	}
}

void SyntAnalyser::F()
{
	if (CurrentIs("(")) {
		Next();
		E();
		if (!CurrentIs(")"))
			throw SyntaxError("expected ')' after function call",
				StrNum());
		Next();
	}
}
