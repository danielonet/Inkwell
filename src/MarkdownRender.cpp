/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "MarkdownRender.h"

#include <ctype.h>


namespace markdown {

namespace {

const size_t kNone = (size_t)-1;


class Builder {
public:
	void Add(const std::string& text, const Style& style)
	{
		if (text.empty())
			return;
		if (fDoc.runs.empty() || !(fDoc.runs.back().style == style)) {
			Run run;
			run.offset = fDoc.text.size();
			run.style = style;
			fDoc.runs.push_back(run);
		}
		fDoc.text += text;
	}

	int AddUrl(const std::string& url)
	{
		fDoc.urls.push_back(url);
		return (int)fDoc.urls.size() - 1;
	}

	bool Empty() const { return fDoc.text.empty(); }
	Document& Doc() { return fDoc; }

private:
	Document fDoc;
};


bool
IsWordChar(char c)
{
	// Bytes >= 0x80 are parts of multi-byte letters (e.g. Hebrew): word chars.
	return isalnum((unsigned char)c) || (unsigned char)c >= 0x80;
}


bool
IsSpace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}


std::string
Trim(const std::string& s)
{
	size_t b = 0;
	size_t e = s.size();
	while (b < e && IsSpace(s[b]))
		b++;
	while (e > b && IsSpace(s[e - 1]))
		e--;
	return s.substr(b, e - b);
}


// Finds the closing delimiter run (exactly `n` copies of `c`) for emphasis.
size_t
FindClosing(const std::string& s, size_t from, size_t end, char c, size_t n)
{
	size_t j = from;
	while (j < end) {
		if (s[j] == '\\') {
			j += 2;
			continue;
		}
		if (s[j] != c) {
			j++;
			continue;
		}
		size_t m = 1;
		while (j + m < end && s[j + m] == c)
			m++;
		if (m == n && j > from && !IsSpace(s[j - 1])
			&& (c == '*' || j + m >= end || !IsWordChar(s[j + m]))) {
			return j;
		}
		j += m;
	}
	return kNone;
}


// Finds the ']' that matches the '[' at `open`, or kNone.
size_t
FindBracketEnd(const std::string& s, size_t open, size_t end)
{
	int depth = 0;
	for (size_t j = open; j < end; j++) {
		if (s[j] == '\\') {
			j++;
		} else if (s[j] == '[') {
			depth++;
		} else if (s[j] == ']') {
			if (--depth == 0)
				return j;
		}
	}
	return kNone;
}


// Finds the ')' that matches the '(' at `open`, or kNone.
size_t
FindParenEnd(const std::string& s, size_t open, size_t end)
{
	int depth = 0;
	for (size_t j = open; j < end; j++) {
		if (s[j] == '\\') {
			j++;
		} else if (s[j] == '(') {
			depth++;
		} else if (s[j] == ')') {
			if (--depth == 0)
				return j;
		}
	}
	return kNone;
}


// "(url)" contents: drop an optional title ("url \"title\"") and <> wrapping.
std::string
LinkTarget(const std::string& raw)
{
	std::string url = Trim(raw);
	if (!url.empty() && url[0] == '<') {
		size_t close = url.find('>');
		if (close != std::string::npos)
			return url.substr(1, close - 1);
	}
	size_t space = url.find(' ');
	if (space != std::string::npos)
		url = url.substr(0, space);
	return url;
}


// <scheme:rest> or <user@host>.
bool
IsAutolink(const std::string& t)
{
	if (t.empty() || t.find(' ') != std::string::npos)
		return false;
	size_t colon = t.find(':');
	if (colon != std::string::npos && colon >= 2 && colon + 1 < t.size()) {
		for (size_t i = 0; i < colon; i++) {
			if (!isalnum((unsigned char)t[i]) && t[i] != '+' && t[i] != '.'
				&& t[i] != '-') {
				return false;
			}
		}
		return isalpha((unsigned char)t[0]) != 0;
	}
	size_t at = t.find('@');
	return at != std::string::npos && at > 0 && at + 1 < t.size();
}


void
ParseInline(const std::string& s, size_t begin, size_t end, const Style& style,
	Builder& out)
{
	std::string literal;
	size_t i = begin;

	while (i < end) {
		char c = s[i];

		if (c == '\\' && i + 1 < end && ispunct((unsigned char)s[i + 1])) {
			literal += s[i + 1];
			i += 2;
			continue;
		}

		if (c == '`') {
			size_t n = 1;
			while (i + n < end && s[i + n] == '`')
				n++;

			size_t j = i + n;
			bool found = false;
			while (j < end) {
				if (s[j] != '`') {
					j++;
					continue;
				}
				size_t m = 1;
				while (j + m < end && s[j + m] == '`')
					m++;
				if (m == n) {
					found = true;
					break;
				}
				j += m;
			}

			if (found) {
				out.Add(literal, style);
				literal.clear();
				std::string code = s.substr(i + n, j - (i + n));
				if (code.size() >= 2 && code[0] == ' '
					&& code[code.size() - 1] == ' ') {
					code = code.substr(1, code.size() - 2);
				}
				Style codeStyle = style;
				codeStyle.code = true;
				out.Add(code, codeStyle);
				i = j + n;
			} else {
				literal.append(n, '`');
				i += n;
			}
			continue;
		}

		if (c == '*' || c == '_') {
			size_t run = 1;
			while (i + run < end && s[i + run] == c)
				run++;

			bool canOpen = run <= 3 && i + run < end && !IsSpace(s[i + run])
				&& (c == '*' || i == begin || !IsWordChar(s[i - 1]));
			size_t close = canOpen ? FindClosing(s, i + run, end, c, run)
				: kNone;

			if (close != kNone) {
				out.Add(literal, style);
				literal.clear();
				Style inner = style;
				if (run == 1) {
					inner.italic = true;
				} else if (run == 2) {
					inner.bold = true;
				} else {
					inner.italic = true;
					inner.bold = true;
				}
				ParseInline(s, i + run, close, inner, out);
				i = close + run;
			} else {
				literal.append(run, c);
				i += run;
			}
			continue;
		}

		if (c == '<') {
			size_t close = s.find('>', i + 1);
			if (close != std::string::npos && close < end
				&& IsAutolink(s.substr(i + 1, close - i - 1))) {
				out.Add(literal, style);
				literal.clear();
				std::string url = s.substr(i + 1, close - i - 1);
				Style inner = style;
				inner.link = true;
				inner.url = out.AddUrl(url);
				out.Add(url, inner);
				i = close + 1;
				continue;
			}
		}

		if (c == '[' || (c == '!' && i + 1 < end && s[i + 1] == '[')) {
			bool image = c == '!';
			size_t open = image ? i + 1 : i;
			size_t closeBracket = FindBracketEnd(s, open, end);
			if (closeBracket != kNone && closeBracket + 1 < end
				&& s[closeBracket + 1] == '(') {
				size_t closeParen = FindParenEnd(s, closeBracket + 1, end);
				if (closeParen != kNone) {
					out.Add(literal, style);
					literal.clear();
					Style inner = style;
					if (image) {
						inner.italic = true;
					} else {
						std::string url = LinkTarget(s.substr(closeBracket + 2,
							closeParen - (closeBracket + 2)));
						inner.link = true;
						if (!url.empty())
							inner.url = out.AddUrl(url);
					}
					ParseInline(s, open + 1, closeBracket, inner, out);
					i = closeParen + 1;
					continue;
				}
			}
		}

		literal += c;
		i++;
	}

	out.Add(literal, style);
}


enum BlockKind {
	kNoBlock,
	kParagraph,
	kHeading,
	kList,
	kQuote,
	kCode,
	kRule
};


class BlockParser {
public:
	BlockParser(Builder& out)
		:
		fOut(out),
		fPrevious(kNoBlock),
		fInFence(false),
		fFenceChar(0),
		fFenceLength(0)
	{
	}

	void Line(const std::string& rawLine);
	void Finish() { _FlushParagraph(); }

private:
	void _BlockStart(BlockKind kind);
	void _FlushParagraph();
	bool _IsRule(const std::string& line) const;

	Builder&	fOut;
	BlockKind	fPrevious;
	std::string	fParagraph;
	bool		fInFence;
	char		fFenceChar;
	size_t		fFenceLength;
};


// Blocks are separated by a blank line, except consecutive list items and
// consecutive quote lines.
void
BlockParser::_BlockStart(BlockKind kind)
{
	bool joined = kind == fPrevious && (kind == kList || kind == kQuote
		|| kind == kCode);
	if (!fOut.Empty() && !joined)
		fOut.Add("\n", Style());
	fPrevious = kind;
}


void
BlockParser::_FlushParagraph()
{
	if (fParagraph.empty())
		return;
	_BlockStart(kParagraph);
	ParseInline(fParagraph, 0, fParagraph.size(), Style(), fOut);
	fOut.Add("\n", Style());
	fParagraph.clear();
}


bool
BlockParser::_IsRule(const std::string& line) const
{
	char marker = 0;
	size_t count = 0;
	for (size_t i = 0; i < line.size(); i++) {
		if (line[i] == ' ')
			continue;
		if (line[i] != '-' && line[i] != '*' && line[i] != '_')
			return false;
		if (marker != 0 && line[i] != marker)
			return false;
		marker = line[i];
		count++;
	}
	return count >= 3;
}


void
BlockParser::Line(const std::string& rawLine)
{
	// Expand leading tabs, drop a trailing CR.
	std::string line;
	size_t pos = 0;
	while (pos < rawLine.size() && (rawLine[pos] == '\t' || rawLine[pos] == ' ')) {
		line += rawLine[pos] == '\t' ? "    " : " ";
		pos++;
	}
	line += rawLine.substr(pos);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1);

	if (fInFence) {
		std::string t = Trim(line);
		size_t n = 0;
		while (n < t.size() && t[n] == fFenceChar)
			n++;
		if (n >= fFenceLength && n == t.size()) {
			fInFence = false;
			return;
		}
		Style code;
		code.code = true;
		fOut.Add(line + "\n", code);
		return;
	}

	size_t indent = 0;
	while (indent < line.size() && line[indent] == ' ')
		indent++;
	std::string body = line.substr(indent);

	if (Trim(body).empty()) {
		_FlushParagraph();
		return;
	}

	// Fenced code block start.
	if (indent < 4 && body.size() >= 3
		&& (body[0] == '`' || body[0] == '~')) {
		size_t n = 0;
		while (n < body.size() && body[n] == body[0])
			n++;
		if (n >= 3) {
			_FlushParagraph();
			_BlockStart(kCode);
			fInFence = true;
			fFenceChar = body[0];
			fFenceLength = n;
			return;
		}
	}

	// Heading.
	if (indent < 4 && body[0] == '#') {
		size_t level = 0;
		while (level < body.size() && body[level] == '#')
			level++;
		if (level <= 6 && (level == body.size() || body[level] == ' ')) {
			_FlushParagraph();
			std::string text = Trim(body.substr(level));
			// Closing hashes: "## Title ##".
			size_t e = text.size();
			while (e > 0 && text[e - 1] == '#')
				e--;
			if (e == 0 || text[e - 1] == ' ')
				text = Trim(text.substr(0, e));
			_BlockStart(kHeading);
			Style style;
			style.heading = (int)level;
			ParseInline(text, 0, text.size(), style, fOut);
			fOut.Add("\n", style);
			return;
		}
	}

	// Horizontal rule (before lists: "* * *" and "- - -").
	if (indent < 4 && _IsRule(body)) {
		_FlushParagraph();
		_BlockStart(kRule);
		Style style;
		style.quote = true;
		fOut.Add("\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"
			"\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"
			"\xE2\x94\x80\xE2\x94\x80\n", style);
		return;
	}

	// Block quote.
	if (indent < 4 && body[0] == '>') {
		_FlushParagraph();
		std::string text = body.substr(1);
		if (!text.empty() && text[0] == ' ')
			text.erase(0, 1);
		_BlockStart(kQuote);
		Style style;
		style.quote = true;
		ParseInline(text, 0, text.size(), style, fOut);
		fOut.Add("\n", style);
		return;
	}

	// List item.
	{
		std::string marker;
		size_t contentStart = 0;
		if (body.size() >= 2 && (body[0] == '-' || body[0] == '*'
				|| body[0] == '+') && body[1] == ' ') {
			marker = "\xE2\x80\xA2 ";	// bullet
			contentStart = 2;
		} else {
			size_t d = 0;
			while (d < body.size() && isdigit((unsigned char)body[d]))
				d++;
			if (d > 0 && d <= 9 && d + 1 < body.size()
				&& (body[d] == '.' || body[d] == ')') && body[d + 1] == ' ') {
				marker = body.substr(0, d) + ". ";
				contentStart = d + 2;
			}
		}

		if (contentStart > 0) {
			_FlushParagraph();
			_BlockStart(kList);
			size_t level = indent / 2;
			if (level > 6)
				level = 6;
			fOut.Add(std::string(level * 4, ' ') + marker, Style());
			std::string text = Trim(body.substr(contentStart));
			ParseInline(text, 0, text.size(), Style(), fOut);
			fOut.Add("\n", Style());
			return;
		}
	}

	// Paragraph text (soft line breaks become spaces).
	if (!fParagraph.empty())
		fParagraph += ' ';
	fParagraph += Trim(body);
}

}	// namespace


Document
Render(const std::string& source)
{
	Builder out;
	BlockParser parser(out);

	size_t start = 0;
	while (start <= source.size()) {
		size_t end = source.find('\n', start);
		if (end == std::string::npos)
			end = source.size();
		parser.Line(source.substr(start, end - start));
		start = end + 1;
	}
	parser.Finish();

	return out.Doc();
}

}	// namespace markdown
