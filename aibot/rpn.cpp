#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rpn.hpp"
#include "bot.hpp"
#include "syntan.hpp"

bool RPNOperator::Game() const
{
	return bot->Game();
}

bool RPNOperator::Handle(char *mes) const
{
	return bot->Handle(mes, *serv);
}

char *RPNOperator::ReceiveMessage() const
{
	return serv->ReceiveMessage();
}

void RPNOperator::Sell(int i1, int i2) const
{
	serv->Sell(i1, i2);
}

void RPNOperator::Buy(int i1, int i2) const
{
	serv->Buy(i1, i2);
}

void RPNOperator::Produce(int i) const
{
	serv->Produce(i);
}

void RPNOperator::Build() const
{
	serv->Build();
}

void RPNOperator::Turn() const
{
	bot->DeletePlayersList();
	bot->DeleteAuctionLists();
	serv->Turn();
}

void RPNOperator::PrintInfo() const
{
	bot->PrintInfo();
}

int RPNOperator::GetMoney(int plid) const
{
	return bot->GetMoney(plid);
}

int RPNOperator::GetRaw(int plid) const
{
	return bot->GetRaw(plid);
}

int RPNOperator::GetDemand() const
{
	return bot->GetDemand();
}

int RPNOperator::GetSupply() const
{
	return bot->GetSupply();
}

int RPNOperator::GetManufactured(int plid) const
{
	return bot->GetManufactured(plid);
}

int RPNOperator::GetResRawSold(int plid) const
{
	return bot->GetResRawSold(plid);
}

int RPNOperator::GetResRawPrice(int plid) const
{
	return bot->GetResRawPrice(plid);
}

int RPNOperator::GetResProdBought(int plid) const
{
	return bot->GetResProdBought(plid);
}

int RPNOperator::GetResProdPrice(int plid) const
{
	return bot->GetResProdPrice(plid);
}

int RPNOperator::GetProdPrice() const
{
	return bot->GetProdPrice();
}
	
int RPNOperator::GetRawPrice() const
{
	return bot->GetRawPrice();
}

int RPNOperator::GetProduction(int plid) const
{
	return bot->GetProduction(plid);
}
	
int RPNOperator::GetFactories(int plid) const
{
	return bot->GetFactories(plid);
}

int RPNOperator::GetActive() const
{
	return bot->GetActive();
}

int RPNOperator::GetTurn() const
{
	return bot->GetTurn();
}

int RPNOperator::GetMyId() const
{
	return bot->GetBotId();
}

void RPNLabel::SetLab(LabelTable &lt)
{
	if (value)
		return;
	Set(lt.GetVal(ident));
}

void SyntAnalyser::AddToRPN(RPNElem *e)
{
	RPNItem *item = new RPNItem;
	item->elem = e;
	item->next = rpn_last->next;
	rpn_last->next = item;
	rpn_last = item;
	if (!rpn_first)
		rpn_first = rpn_last;
}

LabelTable::~LabelTable()
{
	while (ptr) {
		LabelItem *q = ptr;
		ptr = ptr->next;
		delete q;
	}
}

VarTable::~VarTable()
{
	while (ptr) {
		VarItem *q = ptr;
		ptr = ptr->next;
		delete q;
	}
}


void VarTable::AddToTable(const char *id, int val)
{
	VarItem *p = new VarItem;
	p->ident = strdup(id);
	p->value = val;
	p->next = ptr;
	ptr = p;
}

void LabelTable::AddToTable(const char *id, RPNItem *val)
{
	LabelItem *p = new LabelItem;
	p->ident = strdup(id);
	p->value = val;
	p->next = ptr;
	ptr = p;
}

RPNItem *LabelTable::GetVal(const char *id)
{
	LabelItem *p = ptr;
	while (p) {
		int r = strcmp(p->ident, id);
		if (r != 0)
			p = p->next;
		else
			break;
	}
	if (p)
		return p->value;
	else
		throw RPNExUnknownLabel(id);
}

int VarTable::GetVal(const char *id)
{
	VarItem *p = ptr;
	while (p) {
		int r = strcmp(p->ident, id);
		if (r != 0)
			p = p->next;
		else
			break;
	}
	if (p) {
		return p->value;
	} else {
		AddToTable(id);
		return 0;
	}
}

void VarTable::SetVal(const char *id, int val)
{
	VarItem *p = ptr;
	while (p) {
		int r = strcmp(p->ident, id);
		if (r != 0)
			p = p->next;
		else
			break;
	}
	if (p)
		p->value = val;
	else
		AddToTable(id, val);
}

void RPNElem::Push(RPNItem **stack, RPNElem *elem)
{
	RPNItem *i = new RPNItem;
	i->elem = elem;
	i->next = *stack;
	*stack = i;
}

RPNElem *RPNElem::Pop(RPNItem **stack)
{
	if (!*stack)
		return 0;
	RPNItem *q = *stack;
	RPNElem *p = q->elem;
	*stack = (*stack)->next;
	delete q;
	return p;
}

void RPNJump::Evaluate(RPNItem **stack, RPNItem **cur_cmd, VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNLabel *lab = dynamic_cast<RPNLabel*>(o1);
	if (!lab)
		throw RPNExNotLabel(o1);
	RPNItem *addr = lab->Get();
	*cur_cmd = addr;
	delete o1;
}

void RPNJumpFalse::Evaluate(RPNItem **stack, RPNItem **cur_cmd,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNElem *o2 = Pop(stack);
	RPNLabel *lab = dynamic_cast<RPNLabel*>(o1);
	if (!lab)
		throw RPNExNotLabel(o1);
	RPNInt *i = dynamic_cast<RPNInt*>(o2);
	if (!i)
		throw RPNExNotInt(o2);
	if (!i->Get()) {
		RPNItem *addr = lab->Get();
		*cur_cmd = addr;
	} else {
		*cur_cmd = (*cur_cmd)->next;
	}
	delete o1;
	delete o2;
}

void RPNConst::Evaluate(RPNItem **stack, RPNItem **cur_cmd, VarTable &vt) const
{
	Push(stack, Clone());
	*cur_cmd = (*cur_cmd)->next;
}

void RPNOperator::Evaluate(RPNItem **stack, RPNItem **cur_cmd,
				VarTable &vt) const
{
	RPNElem *res = EvaluateOp(stack, vt);
	if (res)
		Push(stack, res);
	*cur_cmd = (*cur_cmd)->next;
}

/*RPNElem *RPNFunction::EvaluateOp(RPNItem **stack,
				VarTable &vt, LabelTable &lt) const
{
	if (strcmp(ident, "?money") == 0) {
		return new RPNInt(10000);
	} else if (strcmp(ident, "?raw") == 0) {
		RPNElem *o = Pop(stack);
		RPNInt *i = dynamic_cast<RPNInt*>(o);
		if (!i)
			throw RPNExNotInt(o);
		delete o;
		return new RPNInt(4);
	} else if (strcmp(ident, "?demand") == 0) {
		return new RPNInt(5);
	} else if (strcmp(ident, "?production_price") == 0) {
		return new RPNInt(5500);
	} else if (strcmp(ident, "?production") == 0) {
		RPNElem *o = Pop(stack);
		RPNInt *i = dynamic_cast<RPNInt*>(o);
		if (!i)
			throw RPNExNotInt(o);
		delete o;
		return new RPNInt(2);
	} else if (strcmp(ident, "?factories") == 0) {
		RPNElem *o = Pop(stack);
		RPNInt *i = dynamic_cast<RPNInt*>(o);
		if (!i)
			throw RPNExNotInt(o);
		delete o;
		return new RPNInt(1);
	} else if (strcmp(ident, "?active_players") == 0) {
		return new RPNInt(0);
	} else if (strcmp(ident, "?turn") == 0) {
		return new RPNInt(111);
	} else if (strcmp(ident, "?raw_price") == 0) {
		return new RPNInt(500);
	} else if (strcmp(ident, "?my_id") == 0) {
		return new RPNInt(1);
	} else {
		throw RPNExUnknownFunc(ident);
	}
}*/

/*RPNElem *RPNPrint::EvaluateOp(RPNItem **stack,
				VarTable &vt, LabelTable &lt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o1);
	if (!i)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNString *s = dynamic_cast<RPNString*>(o2);
	if (!s)
		throw RPNExNotStr(o2);
	char *p = s->GetRPNElemStr(), *str = new char[strlen(p) - 1], *q = str;
	while (*p) {
		if (*p != '"') {
			*q = *p;
			q++;
		}
		p++;
	}
	*q = '\0';
	printf("%s%d\n", str, i->Get());
	delete[] str;
	delete o1;
	delete o2;
	return 0;
}*/

RPNElem *RPNFuncMoney::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int m = GetMoney(i->Get());
	delete o;
	return new RPNInt(m);
}

RPNElem *RPNFuncRaw::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int r = GetRaw(i->Get());
	delete o;
	return new RPNInt(r);
}

RPNElem *RPNFuncDemand::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int d = GetDemand();
	return new RPNInt(d);
}

RPNElem *RPNFuncSupply::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int d = GetSupply();
	return new RPNInt(d);
}

RPNElem *RPNFuncProdPrice::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int p = GetProdPrice();
	return new RPNInt(p);
}

RPNElem *RPNFuncProduction::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int p = GetProduction(i->Get());
	delete o;
	return new RPNInt(p);
}

RPNElem *RPNFuncFactories::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int f = GetFactories(i->Get());
	delete o;
	return new RPNInt(f);
}

RPNElem *RPNFuncManufactured::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int m = GetManufactured(i->Get());
	delete o;
	return new RPNInt(m);
}

RPNElem *RPNFuncResRawSold::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int rrs = GetResRawSold(i->Get());
	delete o;
	return new RPNInt(rrs);
}

RPNElem *RPNFuncResRawPrice::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int rrp = GetResRawPrice(i->Get());
	delete o;
	return new RPNInt(rrp);
}

RPNElem *RPNFuncResProdBought::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int rpb = GetResProdBought(i->Get());
	delete o;
	return new RPNInt(rpb);
}

RPNElem *RPNFuncResProdPrice::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int rpp = GetResProdPrice(i->Get());
	delete o;
	return new RPNInt(rpp);
}

RPNElem *RPNFuncActive::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int a = GetActive();
	return new RPNInt(a);
}

RPNElem *RPNFuncTurn::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int t = GetTurn();
	return new RPNInt(t);
}

RPNElem *RPNFuncRawPrice::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int r = GetRawPrice();
	return new RPNInt(r);
}

RPNElem *RPNFuncMyId::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int m = GetMyId();
	return new RPNInt(m);
}

RPNElem *RPNPrint::EvaluateOp(RPNItem **stack, VarTable &vt) const
{
	int i = 0, buflen = 25;
	char *buf = new char[buflen];
	EvaluatePrint(stack, &buf, i, buflen);
	printf("%s\n", buf);
	delete[] buf;
	return 0;
}

void RPNPrint::BufEnlarge(char **buf, int i, int &buflen) const
{
	char *newbuf;
	buflen = 2*(buflen + 1);
	newbuf = new char[buflen];
	(*buf)[i] = '\0';
	strcpy(newbuf, *buf);
	delete[] *buf;
	*buf = newbuf;
}

void RPNPrint::AddIntToBuf(char **buf, int &i, int &buflen, int val) const
{
	if (val) {
		if (val < 0) {
			(*buf)[i] = '-';
			i++;
			val = -val;
		}
		AddIntToBuf(buf, i, buflen, val / 10);
		(*buf)[i] = val % 10 + '0';
		i++;
		if (i == buflen)
			BufEnlarge(buf, i, buflen);
	}
}

void RPNPrint::EvaluatePrint(RPNItem **stack, char **buf, int &i,
				int &buflen) const
{
	if (!*stack)
		return;
	RPNElem *o1 = Pop(stack);
	RPNInt *v = dynamic_cast<RPNInt*>(o1);
	if (!v) {
		RPNString *s = dynamic_cast<RPNString*>(o1);
		if (!s)
			throw RPNExNotStr(o1);
		EvaluatePrint(stack, buf, i, buflen);
		char *str = s->GetRPNElemStr(), *q = str;;
		while (*q) {
			if (*q != '"') {
				(*buf)[i] = *q;
				i++;
			}
			q++;
			if (i == buflen)
				BufEnlarge(buf, i, buflen);
		}
		(*buf)[i] = '\0';
	} else {
		EvaluatePrint(stack, buf, i, buflen);
		int val = v->Get();
		if (val) {
			AddIntToBuf(buf, i, buflen, v->Get());
			(*buf)[i] = '\0';
		} else {
			(*buf)[i] = '0';
			i++;
			(*buf)[i] = '\0';
		}
	}
}

RPNElem *RPNAssign::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o1);
	if (!i)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNVar *v = dynamic_cast<RPNVar*>(o2);
	if (!v)
		throw RPNExNotVar(o2);
	vt.SetVal(v->GetIdent(), i->Get());
	delete o1;
	delete o2;
	return 0;
}

RPNElem *RPNVarVal::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	int val;
	if (subscript) {
		RPNElem *o = Pop(stack);
		RPNInt *i = dynamic_cast<RPNInt*>(o);
		if (!i)
			throw RPNExNotInt(o);
		int value = i->Get(), v = value, c = 1;
		while (v) {
			v = v / 10;
			c++;
		}
		char *id = new char[strlen(ident) + c + 1];
		sprintf(id, "%s%d", ident, value);
		val = vt.GetVal(id);
		delete o;
	} else {
		val = vt.GetVal(ident);
	}
	return new RPNInt(val);
}

RPNElem *RPNOpSubscript::EvaluateOp(RPNItem **stack,
					VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int val = i->Get(), v = val, c = 1;
	while (v) {
		v = v / 10;
		c++;
	}
	char *id = new char[strlen(ident) + c + 1];
	sprintf(id, "%s%d", ident, val);
	vt.GetVal(id);
	delete o;
	return new RPNVar(id);
}

RPNElem *RPNOpPlus::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i1->Get() + i2->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpMinus::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() - i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpUnaryMinus::EvaluateOp(RPNItem **stack,
					VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int res = -i->Get();
	delete o;
	return new RPNInt(res);
}

RPNElem *RPNOpNot::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	int res = !i->Get();
	delete o;
	return new RPNInt(res);
}

RPNElem *RPNOpAnd::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() && i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpOr::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() || i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpEq::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() == i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpLess::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() < i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpMore::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() > i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpMult::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() * i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}

RPNElem *RPNOpDiv::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() / i1->Get();
	delete o1;
	delete o2;
	if (!i2->Get())
		return new RPNInt(0);
	else
		return new RPNInt(res);
}

RPNElem *RPNOpMod::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	int res = i2->Get() % i1->Get();
	delete o1;
	delete o2;
	return new RPNInt(res);
}


RPNElem *RPNSell::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	Sell(i2->Get(), i1->Get());
	delete o1;
	delete o2;
	return 0;
}

RPNElem *RPNBuy::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o1 = Pop(stack);
	RPNInt *i1 = dynamic_cast<RPNInt*>(o1);
	if (!i1)
		throw RPNExNotInt(o1);
	RPNElem *o2 = Pop(stack);
	RPNInt *i2 = dynamic_cast<RPNInt*>(o2);
	if (!i2)
		throw RPNExNotInt(o2);
	Buy(i2->Get(), i1->Get());
	delete o1;
	delete o2;
	return 0;
}

RPNElem *RPNProd::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	RPNElem *o = Pop(stack);
	RPNInt *i = dynamic_cast<RPNInt*>(o);
	if (!i)
		throw RPNExNotInt(o);
	Produce(i->Get());
	delete o;
	return 0;
}

RPNElem *RPNEndturn::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	//PrintInfo();
	Turn();
	return 0;
}

RPNElem *RPNBuild::EvaluateOp(RPNItem **stack,
				VarTable &vt) const
{
	Build();
	return 0;
}


char *RPNInt::GetRPNElemStr() const
{
	char *str = new char[len];
	sprintf(str, "%d", value);
	return str;
}

char *RPNLabel::GetRPNElemStr() const
{
	char *s = value ? value->elem->GetRPNElemStr() : ident;
	char *str = new char[strlen(s) + 3];
	sprintf(str, "{%s}", s);
	return str;
}

char *RPNOpSubscript::GetRPNElemStr() const
{
	char *str = new char[strlen(ident) + 3];
	sprintf(str, "%s[]", ident);
	return str;
}

char *RPNVarVal::GetRPNElemStr() const
{
	char *str = new char[strlen(ident) + 4];
	if (subscript)
		sprintf(str, "%s[]$", ident);
	else
		sprintf(str, "%s$", ident);
	return str;
}

char *RPNExUnknownFunc::GetStr() const
{
	char *str = new char[strlen(ident) + len];
	sprintf(str, "unknown function identifier: %s", ident);
	return str;
}

char *RPNExUnknownLabel::GetStr() const
{
	char *str = new char[strlen(ident) + len];
	sprintf(str, "unknown label identifier: %s", ident);
	return str;
}
