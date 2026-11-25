// Unit tests for the Markdown renderer. Build and run: make -C tests
#include "../src/MarkdownRender.h"

#include <stdio.h>
#include <stdlib.h>

using namespace markdown;

static int sFailures = 0;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			printf("FAIL line %d: %s\n", __LINE__, #cond); \
			sFailures++; \
		} \
	} while (0)

// Style in effect at the first occurrence of `needle` in the rendered text.
static bool
StyleAt(const Document& d, const char* needle, Style* style)
{
	size_t pos = d.text.find(needle);
	if (pos == std::string::npos)
		return false;
	for (size_t i = d.runs.size(); i > 0; i--) {
		if (d.runs[i - 1].offset <= pos) {
			*style = d.runs[i - 1].style;
			return true;
		}
	}
	return false;
}

static bool
Has(const Document& d, const char* needle)
{
	return d.text.find(needle) != std::string::npos;
}

int
main()
{
	Style s;

	{
		Document d = Render("# Title\n\nHello **bold** and *it* and `x`.\n");
		CHECK(d.text == "Title\n\nHello bold and it and x.\n");
		CHECK(StyleAt(d, "Title", &s) && s.heading == 1);
		CHECK(StyleAt(d, "Hello", &s) && s.heading == 0 && !s.bold);
		CHECK(StyleAt(d, "bold", &s) && s.bold && !s.italic);
		CHECK(StyleAt(d, "it ", &s) && s.italic && !s.bold);
		CHECK(StyleAt(d, "x.", &s) && s.code);
	}
	{
		Document d = Render("***both*** and __bold__ and _it_");
		CHECK(StyleAt(d, "both", &s) && s.bold && s.italic);
		CHECK(StyleAt(d, "bold", &s) && s.bold);
		CHECK(StyleAt(d, "it", &s) && s.italic);
	}
	{
		// Not emphasis: snake_case, spaced asterisks, unclosed markers.
		Document d = Render("snake_case_name and 2 * 3 * 4 and **open");
		CHECK(d.text == "snake_case_name and 2 * 3 * 4 and **open\n");
	}
	{
		Document d = Render("a \\*not italic\\* b");
		CHECK(d.text == "a *not italic* b\n");
		CHECK(d.runs.size() == 1);
	}
	{
		Document d = Render("See [the site](http://x.org/a_(b)) now ![pic](p.png)");
		CHECK(d.text == "See the site now pic\n");
		CHECK(StyleAt(d, "the site", &s) && s.link);
		CHECK(StyleAt(d, "now", &s) && !s.link);
		CHECK(StyleAt(d, "pic", &s) && s.italic && !s.link);
		CHECK(StyleAt(d, "the site", &s) && s.url == 0);
		CHECK(d.urls.size() == 1 && d.urls[0] == "http://x.org/a_(b)");
		CHECK(StyleAt(d, "now", &s) && s.url == -1);
	}
	{
		Document d = Render("[a](one.md \"Title\") [b](<two words.md>) <https://h.org/x> 1 < 2");
		CHECK(d.text == "a b https://h.org/x 1 < 2\n");
		CHECK(d.urls.size() == 3);
		CHECK(d.urls[0] == "one.md");
		CHECK(d.urls[1] == "two words.md");
		CHECK(d.urls[2] == "https://h.org/x");
		CHECK(StyleAt(d, "https", &s) && s.link && s.url == 2);
		CHECK(StyleAt(d, "< 2", &s) && !s.link);
		// Adjacent links with different targets must be separate runs.
		Document e = Render("[a](x)[b](y)");
		CHECK(e.runs.size() == 3 && e.urls.size() == 2);
	}
	{
		Document d = Render("- one\n- two\n  - nested\n\n1. first\n2. second\n");
		CHECK(Has(d, "\xE2\x80\xA2 one\n\xE2\x80\xA2 two\n"));
		CHECK(Has(d, "    \xE2\x80\xA2 nested\n"));
		CHECK(Has(d, "1. first\n2. second\n"));
	}
	{
		Document d = Render("```\n# not heading\n*x*\n```\nafter");
		CHECK(d.text == "# not heading\n*x*\n\nafter\n");
		CHECK(StyleAt(d, "# not", &s) && s.code && s.heading == 0);
		CHECK(StyleAt(d, "after", &s) && !s.code);
	}
	{
		Document d = Render("> quoted *text*\n> more\n\npara");
		CHECK(d.text == "quoted text\nmore\n\npara\n");
		CHECK(StyleAt(d, "quoted", &s) && s.quote);
		CHECK(StyleAt(d, "text", &s) && s.quote && s.italic);
		CHECK(StyleAt(d, "para", &s) && !s.quote);
	}
	{
		Document d = Render("line one\nline two\n\nnext");
		CHECK(d.text == "line one line two\n\nnext\n");
	}
	{
		Document d = Render("a\n\n---\n\nb");
		CHECK(Has(d, "\xE2\x94\x80"));
		CHECK(StyleAt(d, "b", &s) && !s.quote);
	}
	{
		Document d = Render("## Sub ##\r\n### Deep\r\n");
		CHECK(StyleAt(d, "Sub", &s) && s.heading == 2);
		CHECK(StyleAt(d, "Deep", &s) && s.heading == 3);
		CHECK(!Has(d, "#"));
	}
	{
		// Underscores between non-ASCII letters (UTF-8) are not emphasis.
		Document d = Render("caf\xC3\xA9" "_cr\xC3\xA8" "me_br\xC3\xBB" "l\xC3\xA9" "e");
		CHECK(d.text == "caf\xC3\xA9" "_cr\xC3\xA8" "me_br\xC3\xBB" "l\xC3\xA9" "e\n");
	}
	{
		Document d = Render("");
		CHECK(d.text.empty() && d.runs.empty());
	}

	if (sFailures == 0)
		printf("All markdown tests passed.\n");
	return sFailures == 0 ? 0 : 1;
}
