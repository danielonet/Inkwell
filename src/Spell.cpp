/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "Spell.h"

#include <ctype.h>
#include <stdlib.h>

#include <algorithm>
#include <set>
#include <sstream>


namespace spell {

namespace {

// #pragma mark - UTF-8 and case helpers


typedef std::vector<unsigned> CodePoints;


CodePoints
Decode(const std::string& s)
{
	CodePoints out;
	size_t i = 0;
	while (i < s.size()) {
		unsigned char c = (unsigned char)s[i];
		unsigned cp = c;
		size_t len = 1;
		if (c >= 0xF0 && c < 0xF8) {
			cp = c & 0x07;
			len = 4;
		} else if (c >= 0xE0) {
			cp = c & 0x0F;
			len = 3;
		} else if (c >= 0xC0) {
			cp = c & 0x1F;
			len = 2;
		}
		if (i + len > s.size())
			len = 1, cp = c;
		for (size_t k = 1; k < len; k++)
			cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3F);
		out.push_back(cp);
		i += len;
	}
	return out;
}


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


std::string
Encode(const CodePoints& cps)
{
	std::string out;
	for (size_t i = 0; i < cps.size(); i++)
		AppendUtf8(out, cps[i]);
	return out;
}


// Case mapping for ASCII, Latin-1 and (approximately) Latin Extended-A.
unsigned
ToLowerCp(unsigned cp)
{
	if (cp >= 'A' && cp <= 'Z')
		return cp + 32;
	if (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7)
		return cp + 32;
	if (((cp >= 0x100 && cp <= 0x137) || (cp >= 0x14A && cp <= 0x177))
		&& cp % 2 == 0) {
		return cp + 1;
	}
	if (cp >= 0x139 && cp <= 0x148 && cp % 2 == 1)
		return cp + 1;
	return cp;
}


unsigned
ToUpperCp(unsigned cp)
{
	if (cp >= 'a' && cp <= 'z')
		return cp - 32;
	if (cp >= 0xE0 && cp <= 0xFE && cp != 0xF7)
		return cp - 32;
	if (((cp >= 0x100 && cp <= 0x137) || (cp >= 0x14A && cp <= 0x177))
		&& cp % 2 == 1) {
		return cp - 1;
	}
	if (cp >= 0x139 && cp <= 0x148 && cp % 2 == 0)
		return cp - 1;
	return cp;
}


bool
IsUpperCp(unsigned cp)
{
	return ToLowerCp(cp) != cp;
}


bool
IsLowerCp(unsigned cp)
{
	return ToUpperCp(cp) != cp || cp == 0xDF;
}


enum CaseClass {
	kLower,			// hello, or no letters with case
	kCapitalized,	// Hello
	kAllCaps,		// HELLO
	kMixed			// iPhone, McDonald
};


CaseClass
Classify(const CodePoints& cps)
{
	size_t upper = 0;
	size_t lower = 0;
	for (size_t i = 0; i < cps.size(); i++) {
		if (IsUpperCp(cps[i]))
			upper++;
		else if (IsLowerCp(cps[i]))
			lower++;
	}
	if (upper == 0)
		return kLower;
	if (lower == 0)
		return cps.size() > 1 ? kAllCaps : kCapitalized;
	if (upper == 1 && IsUpperCp(cps[0]))
		return kCapitalized;
	return kMixed;
}


std::string
Lower(const std::string& s)
{
	CodePoints cps = Decode(s);
	for (size_t i = 0; i < cps.size(); i++)
		cps[i] = ToLowerCp(cps[i]);
	return Encode(cps);
}


std::string
Upper(const std::string& s)
{
	CodePoints cps = Decode(s);
	for (size_t i = 0; i < cps.size(); i++)
		cps[i] = ToUpperCp(cps[i]);
	return Encode(cps);
}


std::string
Capitalize(const std::string& s)
{
	CodePoints cps = Decode(s);
	if (!cps.empty())
		cps[0] = ToUpperCp(cps[0]);
	return Encode(cps);
}


bool
HasFlag(const std::string& flags, char flag)
{
	return flag != 0 && flags.find(flag) != std::string::npos;
}


// Splits on whitespace.
std::vector<std::string>
Words(const std::string& line)
{
	std::vector<std::string> out;
	std::istringstream in(line);
	std::string w;
	while (in >> w)
		out.push_back(w);
	return out;
}


std::vector<std::string>
Lines(const std::string& text)
{
	std::vector<std::string> out;
	size_t start = 0;
	while (start <= text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.size();
		std::string line = text.substr(start, end - start);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		out.push_back(line);
		start = end + 1;
	}
	return out;
}

}	// namespace


// #pragma mark - Dictionary


Dictionary::Dictionary()
	:
	fOnlyInCompound(0),
	fNoSuggest(0)
{
}


bool
Dictionary::Load(const std::string& affixText, const std::string& wordListText)
{
	fWords.clear();
	fPrefixes.clear();
	fSuffixes.clear();
	fIconv.clear();
	fTry.clear();
	fOnlyInCompound = 0;
	fNoSuggest = 0;

	std::vector<std::string> affLines = Lines(affixText);
	for (size_t i = 0; i < affLines.size(); i++) {
		std::vector<std::string> t = Words(affLines[i]);
		if (t.empty() || t[0][0] == '#')
			continue;

		if (t[0] == "TRY" && t.size() >= 2) {
			fTry = t[1];
		} else if (t[0] == "ONLYINCOMPOUND" && t.size() >= 2) {
			fOnlyInCompound = t[1][0];
		} else if (t[0] == "NOSUGGEST" && t.size() >= 2) {
			fNoSuggest = t[1][0];
		} else if (t[0] == "ICONV" && t.size() >= 3) {
			fIconv.push_back(std::make_pair(t[1], t[2]));
		} else if ((t[0] == "PFX" || t[0] == "SFX") && t.size() >= 5) {
			// "PFX A Y 1" is a header (4 tokens); rules have 5 or more:
			// "SFX G e ing e" = flag, strip, add, condition.
			Affix rule;
			rule.flag = t[1][0];
			rule.cross = false;
			rule.strip = t[2] == "0" ? "" : t[2];
			std::string add = t[3];
			size_t slash = add.find('/');
			if (slash != std::string::npos)
				add = add.substr(0, slash);
			rule.add = add == "0" ? "" : add;

			const std::string& cond = t[4];
			for (size_t k = 0; k < cond.size(); k++) {
				Condition c;
				c.any = false;
				c.negate = false;
				if (cond[k] == '.') {
					c.any = true;
				} else if (cond[k] == '[') {
					size_t close = cond.find(']', k);
					if (close == std::string::npos)
						close = cond.size();
					size_t from = k + 1;
					if (from < close && cond[from] == '^') {
						c.negate = true;
						from++;
					}
					c.chars = cond.substr(from, close - from);
					k = close;
				} else
					c.chars = std::string(1, cond[k]);
				rule.condition.push_back(c);
			}

			// The cross-product setting comes from the header line, which
			// precedes the rules; look it up by scanning back.
			for (size_t b = i; b > 0; b--) {
				std::vector<std::string> h = Words(affLines[b - 1]);
				if (h.size() == 4 && h[0] == t[0] && h[1] == t[1]) {
					rule.cross = h[2] == "Y";
					break;
				}
			}

			(t[0] == "PFX" ? fPrefixes : fSuffixes).push_back(rule);
		}
	}

	std::vector<std::string> dicLines = Lines(wordListText);
	for (size_t i = 0; i < dicLines.size(); i++) {
		const std::string& line = dicLines[i];
		if (line.empty() || (i == 0 && isdigit((unsigned char)line[0])
				&& line.find_first_not_of("0123456789") == std::string::npos)) {
			continue;	// empty line or the leading word count
		}

		// "word/FLAGS morphology": keep word and flags.
		size_t space = line.find_first_of(" \t");
		std::string entry = space == std::string::npos ? line
			: line.substr(0, space);
		size_t slash = entry.find('/');
		std::string word = slash == std::string::npos ? entry
			: entry.substr(0, slash);
		std::string flags = slash == std::string::npos ? std::string()
			: entry.substr(slash + 1);
		if (word.empty())
			continue;
		fWords[word] += flags;
	}

	return !fWords.empty();
}


void
Dictionary::AddWord(const std::string& word)
{
	if (!word.empty() && fWords.find(word) == fWords.end())
		fWords[word] = std::string();
}


std::string
Dictionary::_Convert(const std::string& word) const
{
	std::string out = word;
	for (size_t i = 0; i < fIconv.size(); i++) {
		const std::string& from = fIconv[i].first;
		if (from.empty())
			continue;
		size_t pos = 0;
		while ((pos = out.find(from, pos)) != std::string::npos) {
			out.replace(pos, from.size(), fIconv[i].second);
			pos += fIconv[i].second.size();
		}
	}
	return out;
}


bool
Dictionary::_MatchesEnd(const Affix& rule, const std::string& stem) const
{
	size_t n = rule.condition.size();
	if (stem.size() < n)
		return false;
	for (size_t i = 0; i < n; i++) {
		const Condition& c = rule.condition[i];
		if (c.any)
			continue;
		bool in = c.chars.find(stem[stem.size() - n + i]) != std::string::npos;
		if (in == c.negate)
			return false;
	}
	return true;
}


bool
Dictionary::_MatchesStart(const Affix& rule, const std::string& stem) const
{
	size_t n = rule.condition.size();
	if (stem.size() < n)
		return false;
	for (size_t i = 0; i < n; i++) {
		const Condition& c = rule.condition[i];
		if (c.any)
			continue;
		bool in = c.chars.find(stem[i]) != std::string::npos;
		if (in == c.negate)
			return false;
	}
	return true;
}


// Is `stem` a dictionary word carrying the given affix flags (0 = none needed)?
bool
Dictionary::_HasEntry(const std::string& stem, char flag1, char flag2,
	bool forSuggestion) const
{
	std::unordered_map<std::string, std::string>::const_iterator it
		= fWords.find(stem);
	if (it == fWords.end())
		return false;

	const std::string& flags = it->second;
	if (HasFlag(flags, fOnlyInCompound))
		return false;
	if (forSuggestion && HasFlag(flags, fNoSuggest))
		return false;
	return (flag1 == 0 || HasFlag(flags, flag1))
		&& (flag2 == 0 || HasFlag(flags, flag2));
}


// Suffix check on `word`; if `prefix` is set, the prefix has already been
// removed and the stem must accept both affixes.
bool
Dictionary::_CheckSuffix(const std::string& word, char prefixFlag,
	const Affix* prefix, bool forSuggestion) const
{
	for (size_t i = 0; i < fSuffixes.size(); i++) {
		const Affix& rule = fSuffixes[i];
		if (prefix != NULL && !rule.cross)
			continue;
		if (word.size() < rule.add.size()
			|| word.compare(word.size() - rule.add.size(), rule.add.size(),
				rule.add) != 0) {
			continue;
		}

		std::string stem = word.substr(0, word.size() - rule.add.size())
			+ rule.strip;
		if (stem.empty() || !_MatchesEnd(rule, stem))
			continue;
		if (prefix != NULL && !_MatchesStart(*prefix, stem))
			continue;
		if (_HasEntry(stem, rule.flag, prefixFlag, forSuggestion))
			return true;
	}
	return false;
}


bool
Dictionary::_Lookup(const std::string& word, bool forSuggestion) const
{
	if (word.empty())
		return false;
	if (_HasEntry(word, 0, 0, forSuggestion))
		return true;
	if (_CheckSuffix(word, 0, NULL, forSuggestion))
		return true;

	for (size_t i = 0; i < fPrefixes.size(); i++) {
		const Affix& rule = fPrefixes[i];
		if (word.size() <= rule.add.size()
			|| word.compare(0, rule.add.size(), rule.add) != 0) {
			continue;
		}

		std::string stem = rule.strip + word.substr(rule.add.size());
		if (!_MatchesStart(rule, stem))
			continue;
		if (_HasEntry(stem, rule.flag, 0, forSuggestion))
			return true;
		if (rule.cross && _CheckSuffix(stem, rule.flag, &rule, forSuggestion))
			return true;
	}
	return false;
}


bool
Dictionary::IsCorrect(const std::string& rawWord) const
{
	std::string word = _Convert(rawWord);
	if (word.empty())
		return true;
	if (_Lookup(word, false))
		return true;

	switch (Classify(Decode(word))) {
		case kCapitalized:
			return _Lookup(Lower(word), false);
		case kAllCaps:
			return _Lookup(Lower(word), false)
				|| _Lookup(Capitalize(Lower(word)), false);
		default:
			return false;
	}
}


std::vector<std::string>
Dictionary::Suggest(const std::string& rawWord, size_t maxCount) const
{
	std::vector<std::string> result;
	std::set<std::string> seen;
	std::string word = _Convert(rawWord);
	CodePoints cps = Decode(word);
	CaseClass cls = Classify(cps);
	seen.insert(word);

	// Letters to insert/replace with: the dictionary's TRY string, else a-z.
	CodePoints alphabet = Decode(fTry.empty()
		? std::string("abcdefghijklmnopqrstuvwxyz'") : fTry);

	// Candidates that are checked like typed text, so capitalization rules
	// (proper nouns) apply, but that are never "suggest-hidden" words.
	struct Collector {
		const Dictionary* self;
		CaseClass cls;
		std::vector<std::string>* result;
		std::set<std::string>* seen;
		size_t max;

		void Try(const CodePoints& candidate)
		{
			if (result->size() >= max || candidate.empty())
				return;
			std::string c = Encode(candidate);
			if (!seen->insert(c).second)
				return;
			if (!self->IsCorrect(c))
				return;
			// Hidden words (NOSUGGEST) are valid but never offered.
			std::string lower = Lower(c);
			if (!self->_Lookup(c, true) && !self->_Lookup(lower, true)
				&& !self->_Lookup(Capitalize(lower), true)) {
				return;
			}
			if (cls == kAllCaps)
				c = Upper(c);
			result->push_back(c);
		}
	} collect = {this, cls, &result, &seen, maxCount};

	// Most likely typos first: swapped letters, a missing letter, an extra
	// letter, then a wrong letter.
	for (size_t i = 0; i + 1 < cps.size(); i++) {
		CodePoints c = cps;
		std::swap(c[i], c[i + 1]);
		collect.Try(c);
	}
	for (size_t i = 0; i <= cps.size(); i++) {
		for (size_t a = 0; a < alphabet.size(); a++) {
			CodePoints c = cps;
			c.insert(c.begin() + i, alphabet[a]);
			collect.Try(c);
		}
	}
	for (size_t i = 0; i < cps.size(); i++) {
		CodePoints c = cps;
		c.erase(c.begin() + i);
		collect.Try(c);
	}
	for (size_t i = 0; i < cps.size(); i++) {
		for (size_t a = 0; a < alphabet.size(); a++) {
			if (alphabet[a] == cps[i])
				continue;
			CodePoints c = cps;
			c[i] = alphabet[a];
			collect.Try(c);
		}
	}

	// Two words run together: "alot" -> "a lot". Offered in addition to the
	// edits above, as the last entries.
	for (size_t i = 1; i + 1 < cps.size(); i++) {
		std::string left = Encode(CodePoints(cps.begin(), cps.begin() + i));
		std::string right = Encode(CodePoints(cps.begin() + i, cps.end()));
		if ((left.size() > 1 || left == "a" || left == "I")
			&& right.size() > 1 && IsCorrect(left) && IsCorrect(right)) {
			result.push_back(left + " " + right);
			break;
		}
	}

	return result;
}


// #pragma mark - Tokenizing


namespace {

enum CharKind {
	kLetter,		// Latin letter
	kForeign,		// letter of another script: the word is not checked
	kDigit,
	kApostrophe,
	kOther
};


CharKind
KindOf(unsigned cp)
{
	if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z'))
		return kLetter;
	if (cp >= '0' && cp <= '9')
		return kDigit;
	if (cp == '\'' || cp == 0x2019)
		return kApostrophe;
	if (cp < 0xC0)
		return kOther;
	if (cp == 0xD7 || cp == 0xF7)
		return kOther;
	if (cp <= 0x24F)
		return kLetter;
	// Punctuation, symbols, arrows, CJK punctuation, emoji, ...
	if ((cp >= 0x2000 && cp <= 0x2BFF) || (cp >= 0x3000 && cp <= 0x303F)
		|| (cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xFF00 && cp <= 0xFF0F)
		|| cp >= 0x1F000) {
		return kOther;
	}
	return kForeign;
}


bool
IsSpaceByte(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'
		|| c == '\v';
}


// Chunks that are code, paths or addresses rather than prose.
bool
SkipChunk(const std::string& chunk)
{
	if (chunk.find("://") != std::string::npos)
		return true;
	if (chunk.compare(0, 4, "www.") == 0)
		return true;
	return chunk.find_first_of("@/\\_=#<>|{}~^") != std::string::npos;
}


void
CheckChunk(const Dictionary& dictionary, const std::string& text, size_t begin,
	size_t end, std::vector<Range>& out)
{
	if (SkipChunk(text.substr(begin, end - begin)))
		return;

	// Decode with byte offsets.
	struct Item {
		unsigned	cp;
		size_t		offset;
		size_t		length;
		CharKind	kind;
	};
	std::vector<Item> items;
	size_t i = begin;
	while (i < end) {
		unsigned char c = (unsigned char)text[i];
		size_t len = c >= 0xF0 ? 4 : (c >= 0xE0 ? 3 : (c >= 0xC0 ? 2 : 1));
		if (i + len > end)
			len = 1;
		Item item;
		item.offset = i;
		item.length = len;
		item.cp = Decode(text.substr(i, len))[0];
		item.kind = KindOf(item.cp);
		items.push_back(item);
		i += len;
	}

	size_t k = 0;
	while (k < items.size()) {
		if (items[k].kind != kLetter) {
			k++;
			continue;
		}

		size_t first = k;
		size_t last = k;	// last letter of the word
		while (last + 1 < items.size()) {
			CharKind next = items[last + 1].kind;
			if (next == kLetter) {
				last++;
			} else if (next == kApostrophe && last + 2 < items.size()
				&& items[last + 2].kind == kLetter) {
				last += 2;
			} else
				break;
		}
		k = last + 1;

		// Letters mixed with digits or other scripts: not prose.
		bool attached = (first > 0 && (items[first - 1].kind == kDigit
				|| items[first - 1].kind == kForeign))
			|| (last + 1 < items.size() && (items[last + 1].kind == kDigit
				|| items[last + 1].kind == kForeign));
		if (attached || last == first)
			continue;

		size_t wordBegin = items[first].offset;
		size_t wordEnd = items[last].offset + items[last].length;
		std::string word = text.substr(wordBegin, wordEnd - wordBegin);

		CaseClass cls = Classify(Decode(word));
		if (cls == kAllCaps || cls == kMixed)
			continue;

		if (!dictionary.IsCorrect(word)) {
			Range range;
			range.begin = wordBegin;
			range.end = wordEnd;
			out.push_back(range);
		}
	}
}

}	// namespace


std::vector<Range>
FindMisspellings(const Dictionary& dictionary, const std::string& rawText,
	bool markdown)
{
	std::string masked;
	if (markdown)
		masked = MaskMarkdownCode(rawText);
	const std::string& text = markdown ? masked : rawText;

	std::vector<Range> out;
	size_t i = 0;
	while (i < text.size()) {
		if (IsSpaceByte(text[i])) {
			i++;
			continue;
		}
		size_t start = i;
		while (i < text.size() && !IsSpaceByte(text[i]))
			i++;
		CheckChunk(dictionary, text, start, i, out);
	}
	return out;
}


std::string
MaskMarkdownCode(const std::string& text)
{
	std::string out = text;
	size_t n = text.size();
	size_t lineStart = 0;
	bool inFence = false;
	char fenceChar = 0;

	while (lineStart <= n) {
		size_t lineEnd = text.find('\n', lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = n;

		size_t p = lineStart;
		size_t indent = 0;
		while (p < lineEnd && text[p] == ' ' && indent < 4) {
			p++;
			indent++;
		}
		bool isFence = p + 2 < lineEnd && (text[p] == '`' || text[p] == '~')
			&& text[p + 1] == text[p] && text[p + 2] == text[p]
			&& indent < 4;

		if (inFence) {
			for (size_t k = lineStart; k < lineEnd; k++)
				out[k] = ' ';
			if (isFence && text[p] == fenceChar)
				inFence = false;
		} else if (isFence) {
			inFence = true;
			fenceChar = text[p];
			for (size_t k = lineStart; k < lineEnd; k++)
				out[k] = ' ';
		} else {
			// Inline code and link targets within the line.
			size_t k = lineStart;
			while (k < lineEnd) {
				if (text[k] == '`') {
					size_t run = 1;
					while (k + run < lineEnd && text[k + run] == '`')
						run++;
					size_t j = k + run;
					size_t close = std::string::npos;
					while (j < lineEnd) {
						if (text[j] != '`') {
							j++;
							continue;
						}
						size_t m = 1;
						while (j + m < lineEnd && text[j + m] == '`')
							m++;
						if (m == run) {
							close = j;
							break;
						}
						j += m;
					}
					if (close != std::string::npos) {
						for (size_t q = k; q < close + run; q++)
							out[q] = ' ';
						k = close + run;
					} else
						k += run;
				} else if (text[k] == ']' && k + 1 < lineEnd
					&& text[k + 1] == '(') {
					size_t close = text.find(')', k + 2);
					if (close != std::string::npos && close < lineEnd) {
						for (size_t q = k + 1; q <= close; q++)
							out[q] = ' ';
						k = close + 1;
					} else
						k++;
				} else
					k++;
			}
		}

		lineStart = lineEnd + 1;
	}
	return out;
}

}	// namespace spell
