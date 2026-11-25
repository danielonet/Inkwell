/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "Rtf.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace rtf {

namespace {

enum Destination {
	kNormal,
	kSkip,
	kColorTable
};


struct State {
	State()
		:
		bold(false), italic(false), underline(false), size(0), colorIndex(0),
		uc(1), destination(kNormal)
	{
	}

	bool		bold;
	bool		italic;
	bool		underline;
	int			size;
	int			colorIndex;
	int			uc;
	Destination	destination;
};


void
AppendUtf8(std::string& out, unsigned cp)
{
	if (cp < 0x80) {
		out += (char)cp;
	} else if (cp < 0x800) {
		out += (char)(0xC0 | (cp >> 6));
		out += (char)(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		out += (char)(0xE0 | (cp >> 12));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	} else {
		out += (char)(0xF0 | (cp >> 18));
		out += (char)(0x80 | ((cp >> 12) & 0x3F));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	}
}


// Windows-1252 0x80..0x9F; 0 = unassigned (use the byte value).
const unsigned kCp1252[32] = {
	0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
	0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
	0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178
};


bool
IsSkippedDestination(const std::string& w)
{
	static const char* const kNames[] = {
		"fonttbl", "stylesheet", "info", "pict", "header", "headerl",
		"headerr", "headerf", "footer", "footerl", "footerr", "footerf",
		"generator", "listtable", "listoverridetable", "rsidtbl",
		"themedata", "colorschememapping", "latentstyles", "datastore",
		"xmlnstbl", "object", "fldinst", "private", "revtbl", "filetbl",
		"template", "operator", "title", "author", "company", "comment",
		NULL
	};
	for (int i = 0; kNames[i] != NULL; i++) {
		if (w == kNames[i])
			return true;
	}
	return false;
}


int
HexValue(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


class Parser {
public:
	Parser(const std::string& source)
		:
		fSource(source),
		fStack(1),
		fGroupStart(false),
		fPendingHigh(0),
		fSkipChars(0),
		fRed(-1), fGreen(-1), fBlue(-1)
	{
	}

	Document Process();

private:
	void _Control(size_t& i);
	void _Symbol(size_t& i);
	void _Emit(const std::string& text);
	void _EmitChar(unsigned cp);
	void _Word(const std::string& word, bool hasParam, int param,
		bool groupStart);

	const std::string&	fSource;
	std::vector<State>	fStack;
	bool				fGroupStart;
	unsigned			fPendingHigh;
	int					fSkipChars;
	std::vector<int>	fColors;
	int					fRed, fGreen, fBlue;
	Document			fDocument;
};


Document
Parser::Process()
{
	const std::string& s = fSource;
	size_t n = s.size();
	size_t i = 0;

	while (i < n) {
		char c = s[i];

		if (c == '{') {
			fStack.push_back(fStack.back());
			fGroupStart = true;
			i++;
		} else if (c == '}') {
			if (fStack.size() > 1)
				fStack.pop_back();
			fGroupStart = false;
			i++;
		} else if (c == '\r' || c == '\n') {
			i++;
		} else if (c == '\\') {
			i++;
			if (i < n)
				_Control(i);
		} else {
			fGroupStart = false;
			State& state = fStack.back();
			if (state.destination == kColorTable) {
				if (c == ';') {
					bool hasColor = fRed >= 0 || fGreen >= 0 || fBlue >= 0;
					fColors.push_back(hasColor ? (((fRed < 0 ? 0 : fRed) << 16)
						| ((fGreen < 0 ? 0 : fGreen) << 8)
						| (fBlue < 0 ? 0 : fBlue)) : -1);
					fRed = fGreen = fBlue = -1;
				}
			} else if (state.destination == kNormal) {
				if (fSkipChars > 0)
					fSkipChars--;
				else
					_Emit(std::string(1, c));
			}
			i++;
		}
	}
	return fDocument;
}


// `i` is just after the backslash.
void
Parser::_Control(size_t& i)
{
	const std::string& s = fSource;
	size_t n = s.size();

	if (!isalpha((unsigned char)s[i])) {
		_Symbol(i);
		return;
	}

	size_t j = i;
	while (j < n && isalpha((unsigned char)s[j]))
		j++;
	std::string word = s.substr(i, j - i);

	bool hasParam = false;
	int param = 0;
	size_t k = j;
	bool negative = false;
	if (k < n && s[k] == '-') {
		negative = true;
		k++;
	}
	size_t digits = k;
	while (k < n && isdigit((unsigned char)s[k]))
		k++;
	if (k > digits) {
		hasParam = true;
		param = atoi(s.substr(digits, k - digits).c_str());
		if (negative)
			param = -param;
		j = k;
	}
	if (j < n && s[j] == ' ')
		j++;	// the single space delimiter belongs to the control word
	i = j;

	bool groupStart = fGroupStart;
	fGroupStart = false;
	_Word(word, hasParam, param, groupStart);
}


void
Parser::_Symbol(size_t& i)
{
	const std::string& s = fSource;
	char d = s[i];
	fGroupStart = false;
	i++;

	State& state = fStack.back();
	switch (d) {
		case '*':
			state.destination = kSkip;
			break;
		case '\\':
		case '{':
		case '}':
			_Emit(std::string(1, d));
			break;
		case '\'':
		{
			if (i + 1 < s.size()) {
				int hi = HexValue(s[i]);
				int lo = HexValue(s[i + 1]);
				if (hi >= 0 && lo >= 0) {
					unsigned v = (unsigned)(hi * 16 + lo);
					if (v >= 0x80 && v <= 0x9F && kCp1252[v - 0x80] != 0)
						v = kCp1252[v - 0x80];
					_EmitChar(v);
					i += 2;
				}
			}
			break;
		}
		case '~':
			_EmitChar(0xA0);
			break;
		case '_':
			_Emit("-");
			break;
		case '\n':
		case '\r':
			_Emit("\n");
			break;
		default:
			break;
	}
}


void
Parser::_Word(const std::string& word, bool hasParam, int param,
	bool groupStart)
{
	State& state = fStack.back();

	if (groupStart && IsSkippedDestination(word)) {
		state.destination = kSkip;
		return;
	}
	if (word == "colortbl") {
		state.destination = kColorTable;
		fRed = fGreen = fBlue = -1;
		return;
	}
	if (state.destination == kSkip)
		return;
	if (state.destination == kColorTable) {
		if (word == "red")
			fRed = param & 255;
		else if (word == "green")
			fGreen = param & 255;
		else if (word == "blue")
			fBlue = param & 255;
		return;
	}

	if (word == "par" || word == "line") {
		_Emit("\n");
	} else if (word == "tab") {
		_Emit("\t");
	} else if (word == "b") {
		state.bold = !hasParam || param != 0;
	} else if (word == "i") {
		state.italic = !hasParam || param != 0;
	} else if (word == "ul") {
		state.underline = !hasParam || param != 0;
	} else if (word == "ulnone") {
		state.underline = false;
	} else if (word == "plain") {
		state.bold = state.italic = state.underline = false;
		state.size = 0;
		state.colorIndex = 0;
	} else if (word == "fs") {
		state.size = hasParam ? (param + 1) / 2 : 0;
	} else if (word == "cf") {
		state.colorIndex = hasParam ? param : 0;
	} else if (word == "uc") {
		state.uc = hasParam ? param : 1;
	} else if (word == "u" && hasParam) {
		unsigned v = (unsigned)(param < 0 ? param + 65536 : param) & 0xFFFF;
		if (v >= 0xD800 && v <= 0xDBFF) {
			fPendingHigh = v;
		} else if (v >= 0xDC00 && v <= 0xDFFF && fPendingHigh != 0) {
			_Emit(std::string());
			std::string out;
			AppendUtf8(out, 0x10000 + ((fPendingHigh - 0xD800) << 10)
				+ (v - 0xDC00));
			fPendingHigh = 0;
			_Emit(out);
		} else {
			std::string out;
			AppendUtf8(out, v);
			_Emit(out);
		}
		fSkipChars = state.uc;
	} else if (word == "emdash") {
		_EmitChar(0x2014);
	} else if (word == "endash") {
		_EmitChar(0x2013);
	} else if (word == "lquote") {
		_EmitChar(0x2018);
	} else if (word == "rquote") {
		_EmitChar(0x2019);
	} else if (word == "ldblquote") {
		_EmitChar(0x201C);
	} else if (word == "rdblquote") {
		_EmitChar(0x201D);
	} else if (word == "bullet") {
		_EmitChar(0x2022);
	} else if (word == "emspace" || word == "enspace") {
		_Emit(" ");
	}
}


void
Parser::_EmitChar(unsigned cp)
{
	std::string out;
	AppendUtf8(out, cp);
	_Emit(out);
}


// Appends text in the current state. Fallback characters that follow a \u
// escape are dropped here.
void
Parser::_Emit(const std::string& text)
{
	const State& state = fStack.back();
	if (state.destination != kNormal || text.empty())
		return;
	if (fSkipChars > 0) {
		fSkipChars--;
		return;
	}

	Run run;
	run.offset = fDocument.text.size();
	run.bold = state.bold;
	run.italic = state.italic;
	run.underline = state.underline;
	run.size = state.size;
	run.color = -1;
	// Entry 0 of the color table is the (empty) "auto" color.
	if (state.colorIndex > 0 && (size_t)state.colorIndex < fColors.size())
		run.color = fColors[state.colorIndex];

	if (fDocument.runs.empty() || !run.SameFormat(fDocument.runs.back()))
		fDocument.runs.push_back(run);
	fDocument.text += text;
}


// Number as a decimal string.
std::string
Num(long v)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%ld", v);
	return buffer;
}


void
AppendEscaped(std::string& out, const std::string& t, size_t begin, size_t end)
{
	size_t i = begin;
	while (i < end) {
		unsigned char c = (unsigned char)t[i];
		if (c == '\n') {
			out += "\\par\n";
			i++;
		} else if (c == '\t') {
			out += "\\tab ";
			i++;
		} else if (c == '\\' || c == '{' || c == '}') {
			out += '\\';
			out += (char)c;
			i++;
		} else if (c < 32) {
			i++;
		} else if (c < 128) {
			out += (char)c;
			i++;
		} else {
			unsigned cp = 0;
			size_t len = 0;
			if ((c & 0xE0) == 0xC0) {
				len = 2;
				cp = c & 0x1F;
			} else if ((c & 0xF0) == 0xE0) {
				len = 3;
				cp = c & 0x0F;
			} else if ((c & 0xF8) == 0xF0) {
				len = 4;
				cp = c & 0x07;
			}

			bool valid = len > 0 && i + len <= end;
			for (size_t k = 1; valid && k < len; k++) {
				unsigned char cc = (unsigned char)t[i + k];
				if ((cc & 0xC0) != 0x80)
					valid = false;
				else
					cp = (cp << 6) | (cc & 0x3F);
			}

			if (!valid) {
				out += '?';
				i++;
				continue;
			}
			i += len;

			if (cp > 0xFFFF) {
				cp -= 0x10000;
				unsigned high = 0xD800 + (cp >> 10);
				unsigned low = 0xDC00 + (cp & 0x3FF);
				out += "\\u" + Num((long)high - 65536) + "?\\u"
					+ Num((long)low - 65536) + "?";
			} else {
				out += "\\u" + Num(cp > 32767 ? (long)cp - 65536 : (long)cp)
					+ "?";
			}
		}
	}
}

}	// namespace


Document
Parse(const std::string& source)
{
	Parser parser(source);
	return parser.Process();
}


std::string
Write(const Document& d)
{
	std::vector<int> colors;
	for (size_t i = 0; i < d.runs.size(); i++) {
		int color = d.runs[i].color;
		if (color < 0)
			continue;
		bool known = false;
		for (size_t k = 0; k < colors.size(); k++) {
			if (colors[k] == color)
				known = true;
		}
		if (!known)
			colors.push_back(color);
	}

	std::string out = "{\\rtf1\\ansi\\ansicpg1252\\deff0\n"
		"{\\fonttbl{\\f0\\fnil Arial;}}\n";
	if (!colors.empty()) {
		out += "{\\colortbl ;";
		for (size_t i = 0; i < colors.size(); i++) {
			out += "\\red" + Num((colors[i] >> 16) & 255) + "\\green"
				+ Num((colors[i] >> 8) & 255) + "\\blue"
				+ Num(colors[i] & 255) + ";";
		}
		out += "}\n";
	}
	out += "\\uc1\n";

	std::vector<Run> runs = d.runs;
	if (runs.empty() && !d.text.empty())
		runs.push_back(Run());

	for (size_t i = 0; i < runs.size(); i++) {
		const Run& run = runs[i];
		size_t begin = run.offset;
		size_t end = i + 1 < runs.size() ? runs[i + 1].offset : d.text.size();
		if (begin > d.text.size())
			begin = d.text.size();
		if (end > d.text.size())
			end = d.text.size();

		out += "\\plain\\f0";
		if (run.bold)
			out += "\\b";
		if (run.italic)
			out += "\\i";
		if (run.underline)
			out += "\\ul";
		if (run.size > 0)
			out += "\\fs" + Num(run.size * 2);
		if (run.color >= 0) {
			for (size_t k = 0; k < colors.size(); k++) {
				if (colors[k] == run.color)
					out += "\\cf" + Num((long)k + 1);
			}
		}
		out += ' ';
		AppendEscaped(out, d.text, begin, end);
	}

	out += "}\n";
	return out;
}

}	// namespace rtf
