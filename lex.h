/*
* lex.h - Common functionality for lexical analysis.
*
*  Created on: Jan 6, 2015
*      Author: asaparov
*/

#ifndef LEX_H_
#define LEX_H_

#include "utility.h"

/* TODO: test with benchmarks whether we should inline these functions */
inline bool compare_strings(const array<char>& first, const char* second) {
	for (unsigned int i = 0; i < first.length; i++) {
		if (first[i] != second[i])
			return false;
	}
	if (second[first.length] != '\0')
		return false;
	return true;
}

inline bool compare_strings(const string& first, const char* second) {
	for (unsigned int i = 0; i < first.length; i++) {
		if (first[i] != second[i])
			return false;
	}
	if (second[first.length] != '\0')
		return false;
	return true;
}

inline bool compare_strings(const string& first, const char* second, unsigned int second_length) {
	if (first.length != second_length)
		return false;
	for (unsigned int i = 0; i < first.length; i++) {
		if (first[i] != second[i])
			return false;
	}
	return true;
}

struct position {
	unsigned int line;
	unsigned int column;

	position(unsigned int line, unsigned int column) :
		line(line), column(column) { }

	position(const position& p) :
		line(p.line), column(p.column) { }

	position operator + (unsigned int i) const {
		return position(line, column + i);
	}

	position operator - (unsigned int i) const {
		return position(line, column - i);
	}

	static inline bool copy(const position& src, position& dst) {
		dst.line = src.line;
		dst.column = src.column;
		return true;
	}
};

template<typename TokenType>
struct lexical_token {
	TokenType type;
	position start;
	position end;
	string text;
};

template<typename TokenType, typename Stream, typename Printer>
bool print(const lexical_token<TokenType>& token, Stream& stream, Printer& printer) {
	bool success = true;
	success &= print(token.type, stream, printer);
	if (!is_empty(token.text)) {
		success &= print('(', stream);
		success &= print(token.text, stream, printer);
		success &= print(')', stream);
	}
	return success;
}

inline void read_error(const char* error, const position& pos) {
	fprintf(stderr, "ERROR at %d:%d: %s.\n", pos.line, pos.column, error);
}

template<typename TokenType>
bool emit_token(array<lexical_token<TokenType>>& tokens,
	const position& start, const position& end, TokenType type)
{
	if (!tokens.ensure_capacity(tokens.length + 1)) {
		fprintf(stderr, "emit_token ERROR: Unable to create token.\n");
		return false;
	}

	lexical_token<TokenType>& new_token = tokens[(unsigned int) tokens.length];
	new_token.text.data = NULL;
	new_token.type = type;
	new_token.start = start;
	new_token.end = end;
	tokens.length++;
	return true;
}

template<typename TokenType>
bool emit_token(
	array<lexical_token<TokenType>>& tokens, array<char>& token,
	const position& start, const position& end, TokenType type)
{
	if (!tokens.ensure_capacity(tokens.length + 1)) {
		fprintf(stderr, "emit_token ERROR: Unable to create token.\n");
		return false;
	}

	lexical_token<TokenType>& new_token = tokens[(unsigned int) tokens.length];
	if (!string_init(new_token.text, token.data, (unsigned int) token.length)) {
		fprintf(stderr, "emit_token ERROR: Unable to create string.\n");
		return false;
	}
	new_token.type = type;
	new_token.start = start;
	new_token.end = end;
	tokens.length++;
	token.clear();
	return true;
}

template<typename TokenType>
void free_tokens(array<lexical_token<TokenType>>& tokens) {
	for (unsigned int i = 0; i < tokens.length; i++)
		if (tokens[i].text.data != NULL)
			string_free(tokens[i].text);
}

template<typename TokenType>
bool expect_token(const array<lexical_token<TokenType>>& tokens,
	const unsigned int& index, TokenType type, const char* name)
{
	FILE* out = stderr;
	if (index == tokens.length) {
		/* unexpected end of input */
		fprintf(out, "ERROR: Unexpected end of input. Expected %s.\n", name);
		return false;
	}
	else if (tokens[index].type != type) {
		fprintf(out, "ERROR at %d:%d: Unexpected token ", tokens[index].start.line, tokens[index].start.column);
		print(tokens[index].type, out);
		fprintf(out, ". Expected %s.\n", name);
		return false;
	}
	return true;
}

#endif /* LEX_H_ */
