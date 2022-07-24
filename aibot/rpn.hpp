#ifndef RPN_HPP_SENTRY
#define RPN_HPP_SENTRY

#include <string.h>
#include <stdlib.h>
#include "bot.hpp"

class RPNElem;

class Server;

class Bot;

const int len = 64;

struct RPNItem {
	RPNItem *next;
	RPNElem *elem;
};

struct VarItem {
	VarItem *next;
	char *ident;
	int value;
};

struct LabelItem {
	LabelItem *next;
	char *ident;
	RPNItem *value;
};

class VarTable {
	VarItem *ptr;
public:
	VarTable() { ptr = 0; }
	~VarTable();
	void AddToTable(const char *id, int val = 0);
	int GetVal(const char *id);
	void SetVal(const char *id, int val);
};

class LabelTable {
	LabelItem *ptr;
public:
	LabelTable() { ptr = 0; }
	~LabelTable();
	void AddToTable(const char *id, RPNItem*val);
	RPNItem *GetVal(const char *id);
};

class RPNElem {
public:
	virtual ~RPNElem() {}
	virtual void Evaluate(RPNItem **stack,
				RPNItem **cur_cmd, VarTable &vt) const = 0;
	virtual char *GetRPNElemStr() const = 0;
protected:
	static void Push(RPNItem **stack, RPNElem *elem);
	static RPNElem *Pop(RPNItem **stack);
};

class RPNNoOp: public RPNElem {
public:
	void Evaluate(RPNItem **stack,
			RPNItem **cur_cmd,
			VarTable &vt) const { (*cur_cmd) = (*cur_cmd)->next;}
	char *GetRPNElemStr() const { return strdup("noop"); }
};

class RPNJump: public RPNElem {
	char *ident;
public:
	RPNJump() {}
	virtual ~RPNJump() {}
	void Evaluate(RPNItem **stack,
			RPNItem **cur_cmd, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("!"); }
};

class RPNJumpFalse: public RPNElem {
public:
	RPNJumpFalse() {}
	virtual ~RPNJumpFalse() {}
	void Evaluate(RPNItem **stack,
			RPNItem **cur_cmd, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("!F"); }
};

class RPNConst: public RPNElem {
public:
	virtual RPNElem* Clone() const = 0;
	virtual ~RPNConst() {}
	void Evaluate(RPNItem **stack,
			RPNItem **cur_cmd, VarTable &vt) const;
};

class RPNInt: public RPNConst {
	int value;
public:
	RPNInt(int a):value(a) {}
	virtual ~RPNInt() {}
	RPNElem *Clone() const { return new RPNInt(value); }
	int Get() const { return value; }
	char *GetRPNElemStr() const;
};

class RPNString: public RPNConst {
	char *str;
public:
	RPNString(const char *s) { str = strdup(s); }
	virtual ~RPNString() { free(str); }
	RPNElem *Clone() const { return new RPNString(str); }
	char *GetRPNElemStr() const { return strdup(str); }
};

class RPNVar: public RPNConst {
	char *ident;
public:
	RPNVar(const char *i) { ident = strdup(i); }
	virtual ~RPNVar() { free(ident); }
	RPNElem *Clone() const { return new RPNVar(ident); }
	char *GetIdent() const { return strdup(ident); }
	char *GetRPNElemStr() const { return strdup(ident); }
};

class RPNLabel: public RPNConst {
	RPNItem *value;
	char *ident;
public:
	RPNLabel(RPNItem *a = 0, const char *id = 0)
		: value(a) { if (id) ident = strdup(id); else ident = 0; }
	virtual ~RPNLabel() { if (ident) free(ident); }
	virtual RPNElem *Clone() const { return new RPNLabel(value, ident); }
	RPNItem *Get() const { return value; }
	char *GetIdent() const { return strdup(ident); }
	void Set(RPNItem *v) { value = v; }
	void SetLab(LabelTable &lt);
	char *GetRPNElemStr() const;
};

class RPNOperator: public RPNElem {
	Server *serv;
	Bot *bot;
public:
	RPNOperator(Server *s = 0, Bot *b = 0) {}
	virtual RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const = 0;
	void Evaluate(RPNItem **stack,
			RPNItem **cur_cmd, VarTable &vt) const;
	void SetBotAndServ(Bot *b, Server *s) { bot = b; serv = s;}
	int GetMoney(int plid) const;
	int GetRaw(int plid) const;
	int GetManufactured(int plid) const;
	int GetResRawSold(int plid) const;
	int GetResRawPrice(int plid) const;
	int GetResProdBought(int plid) const;
	int GetResProdPrice(int plid) const;
	int GetDemand() const;
	int GetSupply() const;
	int GetProdPrice() const;
	int GetRawPrice() const;
	int GetProduction(int plid) const;
	int GetFactories(int plid) const;
	int GetActive() const;
	int GetTurn() const;
	int GetMyId() const;
	bool Game() const;
	void Sell(int i1, int i2) const;
	void Buy(int i1, int i2) const;
	void Produce(int i) const;
	void Build() const;
	void Turn() const;
	bool Handle(char *mes) const;
	char *ReceiveMessage() const;
	void PrintInfo() const;
};

/*class RPNFunction: public RPNOperator {
	char *ident;
public:
	RPNFunction(char *id) { ident = strdup(id); }
	virtual ~RPNFunction() { free(ident); }
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt,
				LabelTable &lt) const;
	char *GetRPNElemStr() const { return strdup(ident); }
};*/

class RPNFuncMoney: public RPNOperator {
public:
	RPNFuncMoney() {}
	virtual ~RPNFuncMoney() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?money"); }
};

class RPNFuncRaw: public RPNOperator {
public:
	RPNFuncRaw() {}
	virtual ~RPNFuncRaw() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?raw"); }
};

class RPNFuncDemand: public RPNOperator {
public:
	RPNFuncDemand() {}
	virtual ~RPNFuncDemand() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?demand"); }
};

class RPNFuncSupply: public RPNOperator {
public:
	RPNFuncSupply() {}
	virtual ~RPNFuncSupply() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?supply"); }
};

class RPNFuncProdPrice: public RPNOperator {
public:
	RPNFuncProdPrice() {}
	virtual ~RPNFuncProdPrice() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?production_price"); }
};

class RPNFuncProduction: public RPNOperator {
public:
	RPNFuncProduction() {}
	virtual ~RPNFuncProduction() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?production"); }
};

class RPNFuncFactories: public RPNOperator {
public:
	RPNFuncFactories() {}
	virtual ~RPNFuncFactories() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?factories"); }
};

class RPNFuncActive: public RPNOperator {
public:
	RPNFuncActive() {}
	virtual ~RPNFuncActive() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?active_players"); }
};

class RPNFuncTurn: public RPNOperator {
public:
	RPNFuncTurn() {}
	virtual ~RPNFuncTurn() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?turn"); }
};

class RPNFuncRawPrice: public RPNOperator {
public:
	RPNFuncRawPrice() {}
	virtual ~RPNFuncRawPrice() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?raw_price"); }
};

class RPNFuncManufactured: public RPNOperator {
public:
	RPNFuncManufactured() {}
	virtual ~RPNFuncManufactured() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?manufactured"); }
};

class RPNFuncResRawSold: public RPNOperator {
public:
	RPNFuncResRawSold() {}
	virtual ~RPNFuncResRawSold() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?result_raw_sold"); }
};

class RPNFuncResRawPrice: public RPNOperator {
public:
	RPNFuncResRawPrice() {}
	virtual ~RPNFuncResRawPrice() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?result_raw_price"); }
};

class RPNFuncResProdBought: public RPNOperator {
public:
	RPNFuncResProdBought() {}
	virtual ~RPNFuncResProdBought() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?result_prod_bought"); }
};

class RPNFuncResProdPrice: public RPNOperator {
public:
	RPNFuncResProdPrice() {}
	virtual ~RPNFuncResProdPrice() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?result_prod_price"); }
};

class RPNFuncMyId: public RPNOperator {
public:
	RPNFuncMyId() {}
	virtual ~RPNFuncMyId() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("?my_id"); }
};

class RPNAssign: public RPNOperator {
public:
	RPNAssign() {}
	virtual ~RPNAssign() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup(":="); }
};

class RPNVarVal: public RPNOperator {
	char *ident;
	bool subscript;
public:
	RPNVarVal(const char *id, bool subs)
		: subscript(subs) { ident = strdup(id); }
	virtual ~RPNVarVal() { free(ident); }
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const;
};

class RPNOpPlus: public RPNOperator {
public:
	RPNOpPlus() {}
	virtual ~RPNOpPlus() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("+"); }
};

class RPNOpSubscript: public RPNOperator {
	char *ident;
public:
	RPNOpSubscript(const char *id) { ident = strdup(id); }
	virtual ~RPNOpSubscript() { free(ident); }
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const;
};

class RPNOpMinus: public RPNOperator {
public:
	RPNOpMinus() {}
	virtual ~RPNOpMinus() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("+"); }
};

class RPNOpUnaryMinus: public RPNOperator {
public:
	RPNOpUnaryMinus() {}
	virtual ~RPNOpUnaryMinus() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("@"); }
};

class RPNOpNot: public RPNOperator {
public:
	RPNOpNot() {}
	virtual ~RPNOpNot() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("not"); }
};

class RPNOpAnd: public RPNOperator {
public:
	RPNOpAnd() {}
	virtual ~RPNOpAnd() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("&"); }
};

class RPNOpOr: public RPNOperator {
public:
	RPNOpOr() {}
	virtual ~RPNOpOr() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("|"); }
};

class RPNOpEq: public RPNOperator {
public:
	RPNOpEq() {}
	virtual ~RPNOpEq() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("="); }
};

class RPNOpLess: public RPNOperator {
public:
	RPNOpLess() {}
	virtual ~RPNOpLess() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("<"); }
};

class RPNOpMore: public RPNOperator {
public:
	RPNOpMore() {}
	virtual ~RPNOpMore() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup(">"); }
};

class RPNOpMult: public RPNOperator {
public:
	RPNOpMult() {}
	virtual ~RPNOpMult() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("*"); }
};

class RPNOpDiv: public RPNOperator {
public:
	RPNOpDiv() {}
	virtual ~RPNOpDiv() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("/"); }
};

class RPNOpMod: public RPNOperator {
public:
	RPNOpMod() {}
	virtual ~RPNOpMod() {}
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("%"); }
};

class RPNSell: public RPNOperator {
public:
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("sell"); }
};

class RPNBuy: public RPNOperator {
public:
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("buy"); }
};

class RPNProd: public RPNOperator {
public:
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("prod"); }
};

class RPNEndturn: public RPNOperator {
public:
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("endturn"); }
};

class RPNBuild: public RPNOperator {
public:
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("build"); }
};

class RPNPrint: public RPNOperator {
	void BufEnlarge(char **buf, int i, int &buflen) const;
	void EvaluatePrint(RPNItem **stack, char **buf, int &i,
				int &buflen) const;
	void AddIntToBuf(char **buf, int &i, int &buflen, int val) const;
public:
	RPNElem *EvaluateOp(RPNItem **stack, VarTable &vt) const;
	char *GetRPNElemStr() const { return strdup("print"); }
};

class RPNEx {
	RPNElem *er;
public:
	RPNEx(RPNElem *e): er(e) {}
	virtual char *GetStr() const = 0;
	RPNElem *GetElem() const { return er;}
};

class RPNExNotInt: public RPNEx {
public:
	RPNExNotInt(RPNElem *er):RPNEx(er) {}
	char *GetStr() const { return strdup("integer expected in stack"); }
};

class RPNExNotLabel: public RPNEx {
public:
	RPNExNotLabel(RPNElem *er):RPNEx(er) {}
	char *GetStr() const { return strdup("label expected in stack"); }
};

class RPNExNotVar: public RPNEx {
public:
	RPNExNotVar(RPNElem *er):RPNEx(er) {}
	char *GetStr() const { return strdup("var address expected in stack"); }
};

class RPNExNotStr: public RPNEx {
public:
	RPNExNotStr(RPNElem *er):RPNEx(er) {}
	char *GetStr() const { return strdup("string expected in stack"); }
};

class RPNExUnknownFunc: public RPNEx {
	char *ident;
public:
	RPNExUnknownFunc(const char *id):RPNEx(0) { ident = strdup(id); }
	char *GetStr() const;
};

class RPNExUnknownLabel: public RPNEx {
	char *ident;
public:
	RPNExUnknownLabel(const char *id):RPNEx(0) { ident = strdup(id); }
	~RPNExUnknownLabel() { free(ident); }
	char *GetStr() const;
};

#endif
