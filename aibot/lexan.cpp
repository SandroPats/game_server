#include "lexan.hpp"
#include "syntan.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LexAnalyser::LexAnalyser()
{
	strc = 1;
	curstr = 1;
	i = 0;
	strerr = 0;
	buflen = 25;
	buf = new char[buflen];
	buf[i] = '\0';
	lex = 0;
	res = '\0';
	state = Home;
}

LexAnalyser::~LexAnalyser()
{
	delete[] buf;
}

void LexAnalyser::AddToBuf(char c)
{
	buf[i] = c;
	i++;
}

static bool IsDelimeter(char c)
{
	return c == ' ' || c == '\n' || c == '\r' ||
		c == '\t';
}

static bool IsDelimeterLexeme(char c)
{
	return c == '+' || c == '-' || c == '*' ||
		c == '/' || c == '%' || c == '=' ||
		c == '<' || c == '>' || c == '|' ||
		c == '&' || c == '!' || c == '(' ||
		c == ')' || c == '[' || c == ']' ||
		c == '{' || c == '}' || c == ';' ||
		c == ',';
}

void LexAnalyser::FormLexeme()
{
	buf[i] = '\0';
	lex = new Lexeme;
	lex->str = new char[strlen(buf) + 1];
	strcpy(lex->str, buf);
	lex->type = curtype;
	lex->strnum = curstr;
	i = 0;
}

void LexAnalyser::KeyError()
{
	strerr = new char[128];
	sprintf(strerr, "line %d: Keyword contains "
			"invalid characters\n", strc);
}

void LexAnalyser::IntegerError()
{
	strerr = new char[128];
	sprintf(strerr, "line %d: Integer value contains"
			" invalid characters\n", strc);
}

void LexAnalyser::IdentError()
{
	strerr = new char[128];
	sprintf(strerr, "line %d: Identifier contains"
			" invalid characters\n", strc);
}

char *LexAnalyser::ErrorCheck()
{
	if (state != Home && state != Error) {
		strerr = new char[128];
		sprintf(strerr, "line %d: unexpected EOF\n",
			strc);
	}
	return strerr;
}

void LexAnalyser::HandleHome(char c)
{
	if (c == '?') {
		state = Ident;
		curtype = lex_func;
	} else if (c == '$') {
		state = Ident;
		curtype = lex_var;
	} else if (c <= 'z' && c >= 'a') {
		state = Key;
		curtype = lex_key;
	} else if (c == '@') {
		state = Ident;
		curtype = lex_label;
	} else if (c == '"') {
		state = Str;
		curtype = lex_literal;
	} else if (c == ':') {
		state = Assign;
	} else if (c >= '0' && c <= '9') {
		state = Integer;
		curtype = lex_int;
	} else if (!IsDelimeter(c)) {
		curtype = lex_delim;
		FormLexeme();
	}
	curstr = strc;
}

void LexAnalyser::HandleKey(char c)
{
	if (IsDelimeter(c)) {
		state = Home;
		FormLexeme();
	} else if (IsDelimeterLexeme(c) || c == ':') { 
		state = Home;
		FormLexeme();
	} else if (c > 'z' || c < 'a') {
		KeyError();
		state = Error;
	}
}

void LexAnalyser::HandleIdent(char c)
{
	if (IsDelimeterLexeme(c) || IsDelimeter(c) || c == ':') {
		state = Home;
		FormLexeme();
	} else if (c == '"') {
		IdentError();
		state = Error;
	}
}

void LexAnalyser::HandleInteger(char c)
{
	if (IsDelimeter(c) || IsDelimeterLexeme(c) || c == ':') {
		state = Home;
		FormLexeme();
	} else if (c > '9' || c < '0') {
		IntegerError();
		state = Error;
	}
}

void LexAnalyser::HandleStr(char c)
{
	if (c == '"') {
		state = Home;
		FormLexeme();
	}
}

void LexAnalyser::HandleAssign(char c)
{
	if (c == '=') {
		state = Home;
		curtype = lex_assign;
		FormLexeme();
	} else {
		state = Home;
		curtype = lex_delim;
		FormLexeme();
	}
}

bool LexAnalyser::Step(char c)
{
	bool eaten = false;
	if (((!IsDelimeterLexeme(c) || state == Home ||
		(state == Assign && c == '=')) &&
		!IsDelimeter(c) && state != Error &&
		(c != ':' || state == Home ) &&
		(state != Assign || c == '=')) ||
		state == Str)
	{
		AddToBuf(c);
		eaten = true;
	}
	switch (state) {
		case Home:
			HandleHome(c);
			break;
		case Key:
			HandleKey(c);
			break;
		case Ident:
			HandleIdent(c);
			break;
		case Integer:
			HandleInteger(c);
			break;
		case Str:
			HandleStr(c);
			break;
		case Assign:
			HandleAssign(c);
			break;
		case Error:
			break;
	}
	return eaten;
}

void LexAnalyser::BufEnlarge()
{
	char *newbuf;
	buflen = 2*(buflen + 1);
	newbuf = new char[2*(buflen + 1)];
	buf[i] = '\0';
	strcpy(newbuf, buf);
	delete[] buf;
	buf = newbuf;
}

Lexeme *LexAnalyser::FeedChar(char c)
{
	int r;
	Lexeme *l;
	if (i == buflen - 1)
		BufEnlarge();
	if (c == '\n')
		strc++;
	if (res) {
		r = Step(res);
		if (!lex || IsDelimeter(c)) {
			r = Step(c);
			res = '\0';
		} else {
			res = c;
		}
	} else {
		r = Step(c);
	}
	if (!r)
		res = c;
	l = lex;
	lex = 0;
	return l;
}
