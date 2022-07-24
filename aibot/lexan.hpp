#ifndef LEXAN_HPP_SENTRY
#define LEXAN_HPP_SENTRY

enum LexType {
	lex_key,
	lex_var,
	lex_func,
	lex_label,
	lex_int,
	lex_literal,
	lex_assign,
	lex_delim,
};

struct Lexeme {
	LexType type;
	int strnum;
	char *str;
};

class SyntaxError {
	char *str;
	int strnum;
public:
	SyntaxError(const char *s, int snum);
	SyntaxError(const SyntaxError &E);
	~SyntaxError();
	const char *GetStr() const { return str; }
	int GetStrNum() const { return strnum; }
};

class LexAnalyser {
	enum States {
		Home,
		Key,
		Ident,
		Integer,
		Str,
		Assign,
		Error,
	};
	States state;
	Lexeme *lex;
	LexType curtype;
	int strc, i, buflen, curstr;
	char *buf, *strerr, res;
	void BufEnlarge();
	bool Step(char c);
	void AddToBuf(char c);
	void FormLexeme();
	void KeyError();
	void IntegerError();
	void IdentError();
	void HandleHome(char c);
	void HandleKey(char c);
	void HandleIdent(char c);
	void HandleInteger(char c);
	void HandleStr(char c);
	void HandleAssign(char c);
public:
	LexAnalyser();
	~LexAnalyser();
	Lexeme *FeedChar(char c);
	char *ErrorCheck();
};

#endif
