// Unit tests for the spell checker, using the bundled en_US dictionary.
// Run from the tests directory: make -C tests test
#include "../src/Spell.h"

#include <stdio.h>

#include <fstream>
#include <sstream>

using namespace spell;

static int sFailures = 0;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			sFailures++; \
		} \
	} while (0)

static std::string
ReadFile(const char* path)
{
	std::ifstream in(path, std::ios::binary);
	std::stringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

static bool
Contains(const std::vector<std::string>& v, const char* s)
{
	for (size_t i = 0; i < v.size(); i++) {
		if (v[i] == s)
			return true;
	}
	return false;
}

// The misspelled words found in `text`, space separated.
static std::string
Misspelled(const Dictionary& d, const std::string& text, bool markdown = false)
{
	std::vector<Range> ranges = FindMisspellings(d, text, markdown);
	std::string out;
	for (size_t i = 0; i < ranges.size(); i++) {
		if (i > 0)
			out += ' ';
		out += text.substr(ranges[i].begin, ranges[i].end - ranges[i].begin);
	}
	return out;
}

int
main()
{
	Dictionary d;
	CHECK(d.Load(ReadFile("../dictionaries/en_US.aff"),
		ReadFile("../dictionaries/en_US.dic")));

	// Plain words, capitalization.
	CHECK(d.IsCorrect("hello"));
	CHECK(d.IsCorrect("Hello"));
	CHECK(d.IsCorrect("HELLO"));
	CHECK(d.IsCorrect("Paris"));
	CHECK(!d.IsCorrect("paris"));
	CHECK(!d.IsCorrect("wrold"));
	CHECK(!d.IsCorrect("teh"));
	CHECK(!d.IsCorrect("colour"));
	CHECK(d.IsCorrect("color"));

	// Suffix and prefix rules, cross product.
	CHECK(d.IsCorrect("walked"));
	CHECK(d.IsCorrect("running"));
	CHECK(d.IsCorrect("happier"));
	CHECK(d.IsCorrect("unhappy"));
	CHECK(d.IsCorrect("reworked"));
	CHECK(d.IsCorrect("dog's"));
	CHECK(d.IsCorrect("dogs"));
	CHECK(d.IsCorrect("flies"));
	CHECK(!d.IsCorrect("walkded"));
	CHECK(d.IsCorrect("don\xE2\x80\x99t"));	// curly apostrophe (ICONV)
	CHECK(d.IsCorrect("don't"));

	// Only valid in compounds (1th etc. are skipped anyway as numbers).
	CHECK(!d.IsCorrect("th"));

	// Suggestions.
	CHECK(Contains(d.Suggest("teh"), "the"));
	CHECK(Contains(d.Suggest("helo"), "hello"));
	CHECK(Contains(d.Suggest("recieve"), "receive"));
	CHECK(Contains(d.Suggest("Wrold"), "World"));
	CHECK(Contains(d.Suggest("TEH"), "THE"));
	CHECK(Contains(d.Suggest("alot"), "a lot"));
	CHECK(d.Suggest("hello").empty() || !Contains(d.Suggest("hello"), "hello"));

	// Personal words.
	CHECK(!d.IsCorrect("Zorblax"));
	d.AddWord("Zorblax");
	CHECK(d.IsCorrect("Zorblax"));
	CHECK(!d.IsCorrect("zorblax"));		// capitalized entry: exact case only
	d.AddWord("flibbertigibbet");
	CHECK(d.IsCorrect("Flibbertigibbet") && d.IsCorrect("flibbertigibbet"));
	CHECK(!d.IsCorrect("zorblaxx"));

	// Tokenizing: what is and is not checked.
	CHECK(Misspelled(d, "Hello wrold, this is fine.") == "wrold");
	CHECK(Misspelled(d, "It's the dog's bone; \"quoted\" (teh) end.") == "teh");
	CHECK(Misspelled(d, "abc123 x2 3rd 21st 100% HTML iPhone NASA") == "");
	CHECK(Misspelled(d, "see https://exmple.org/foo and me@exmple.com or "
		"/usr/lcal/bin or snake_cse") == "");
	CHECK(Misspelled(d, "well-known wrld-class") == "wrld");
	// Other scripts are not judged; accented Latin words are (this dictionary
	// spells "cafe" without the accent).
	CHECK(Misspelled(d, "and \xCE\xB1\xCE\xB2\xCE\xB3 fine") == "");
	CHECK(Misspelled(d, "caf\xC3\xA9 fine") == "caf\xC3\xA9");
	CHECK(Misspelled(d, "a b I x") == "");
	{
		std::string text = "one tww three";
		std::vector<Range> r = FindMisspellings(d, text, false);
		CHECK(r.size() == 1 && r[0].begin == 4 && r[0].end == 7);
	}
	{
		// Byte offsets with multi-byte characters before the word.
		std::string text = "\xE2\x80\x9C" "teh";
		std::vector<Range> r = FindMisspellings(d, text, false);
		CHECK(r.size() == 1 && r[0].begin == 3 && r[0].end == 6);
	}

	// Markdown: code and link targets are not prose.
	CHECK(Misspelled(d, "Use `teh` here and teh there", true) == "teh");
	CHECK(Misspelled(d, "```\nteh code\n```\nafter teh", true) == "teh");
	CHECK(Misspelled(d, "[good link](othr.md) text", true) == "");
	CHECK(Misspelled(d, "[wrng link](x.md)", true) == "wrng");
	CHECK(Misspelled(d, "`teh`", false) == "teh");
	CHECK(MaskMarkdownCode("a `b` c").size() == 7);

	if (sFailures == 0)
		printf("All spell tests passed.\n");
	return sFailures == 0 ? 0 : 1;
}
