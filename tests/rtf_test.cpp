// Unit tests for the RTF reader/writer. Build and run: make -C tests test
#include "../src/Rtf.h"

#include <stdio.h>

using namespace rtf;

static int sFailures = 0;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			sFailures++; \
		} \
	} while (0)

static const Run*
RunAt(const Document& d, const char* needle)
{
	size_t pos = d.text.find(needle);
	if (pos == std::string::npos)
		return NULL;
	const Run* found = NULL;
	for (size_t i = 0; i < d.runs.size(); i++) {
		if (d.runs[i].offset <= pos)
			found = &d.runs[i];
	}
	return found;
}

int
main()
{
	{
		// Typical word processor output.
		Document d = Parse(
			"{\\rtf1\\ansi\\ansicpg1252\\deff0\\nouicompat"
			"{\\fonttbl{\\f0\\fnil\\fcharset0 Calibri;}}\n"
			"{\\colortbl ;\\red255\\green0\\blue0;}\n"
			"{\\*\\generator Riched20 10.0.19041}\\viewkind4\\uc1 \n"
			"\\pard\\sa200\\sl276\\slmult1\\f0\\fs22\\lang9 Hello \\b bold\\b0  "
			"and \\cf1 red\\cf0\\par\n"
			"next \\u1513?\\'e9\\par\n}");
		CHECK(d.text == "Hello bold and red\nnext \xD7\xA9\xC3\xA9\n");
		const Run* r;
		CHECK((r = RunAt(d, "Hello")) != NULL && !r->bold && r->size == 11);
		CHECK((r = RunAt(d, "bold")) != NULL && r->bold);
		CHECK((r = RunAt(d, "and")) != NULL && !r->bold);
		CHECK((r = RunAt(d, "red")) != NULL && r->color == 0xFF0000);
		CHECK((r = RunAt(d, "next")) != NULL && r->color == -1);
	}
	{
		Document d = Parse("{\\rtf1 a\\tab b \\i it\\i0\\ul u\\ulnone  \\{x\\\\y\\}}");
		CHECK(d.text == "a\tb itu {x\\y}");
		CHECK(RunAt(d, "it")->italic);
		CHECK(RunAt(d, "u ")->underline);
		CHECK(!RunAt(d, "{")->underline);
	}
	{
		// Surrogate pair (U+1F600) and typographic symbols.
		Document d = Parse("{\\rtf1 \\u-10179?\\u-8704?\\ldblquote x\\rdblquote\\emdash}");
		CHECK(d.text == "\xF0\x9F\x98\x80\xE2\x80\x9Cx\xE2\x80\x9D\xE2\x80\x94");
	}
	{
		// Unknown destinations and groups are skipped.
		Document d = Parse("{\\rtf1{\\*\\foo hidden}{\\info{\\title t}}shown}");
		CHECK(d.text == "shown");
	}
	{
		// Round trip.
		Document d;
		d.text = "Title\n\xC3\xA9 {x}\\ \xF0\x9F\x98\x80 tab\there";
		Run a;
		a.offset = 0;
		a.bold = true;
		a.size = 18;
		Run b;
		b.offset = 6;
		b.color = 0x336699;
		Run c;
		c.offset = 14;
		c.italic = true;
		c.underline = true;
		c.size = 12;
		d.runs.push_back(a);
		d.runs.push_back(b);
		d.runs.push_back(c);

		std::string rtf = Write(d);
		Document back = Parse(rtf);
		CHECK(back.text == d.text);
		CHECK(back.runs.size() == d.runs.size());
		for (size_t i = 0; i < back.runs.size() && i < d.runs.size(); i++) {
			CHECK(back.runs[i].offset == d.runs[i].offset);
			CHECK(back.runs[i].SameFormat(d.runs[i]));
		}
	}
	{
		Document d = Parse("");
		CHECK(d.text.empty());
		CHECK(Parse(Write(d)).text.empty());
	}

	if (sFailures == 0)
		printf("All rtf tests passed.\n");
	return sFailures == 0 ? 0 : 1;
}
