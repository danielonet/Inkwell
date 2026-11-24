/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef MARKDOWN_RENDER_H
#define MARKDOWN_RENDER_H

// Platform independent Markdown -> styled plain text. No Haiku headers here,
// so it can be unit tested anywhere (see tests/).
//
// Supported: ATX headings, paragraphs, bullet/numbered lists (nested by
// indentation), block quotes, fenced code blocks, horizontal rules, and
// inline **bold**, *italic*, ***both***, `code`, [links](url), ![images](url)
// (alt text only) and backslash escapes.

#include <string>
#include <vector>


namespace markdown {

struct Style {
	Style()
		:
		bold(false), italic(false), code(false), link(false), quote(false),
		heading(0)
	{
	}

	bool bold;
	bool italic;
	bool code;
	bool link;
	bool quote;
	int heading;		// 0 = not a heading, else 1..6

	bool operator==(const Style& o) const
	{
		return bold == o.bold && italic == o.italic && code == o.code
			&& link == o.link && quote == o.quote && heading == o.heading;
	}
};


// The style applies from `offset` until the next run starts.
struct Run {
	size_t	offset;
	Style	style;
};


struct Document {
	std::string			text;
	std::vector<Run>	runs;
};


Document Render(const std::string& source);

}	// namespace markdown

#endif	// MARKDOWN_RENDER_H
