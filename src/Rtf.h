/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef RTF_H
#define RTF_H

// Platform independent RTF reader/writer (no Haiku headers, see tests/).
//
// Reads: paragraphs, bold, italic, underline, font size, text color, tabs,
// unicode (\uN), code page escapes (\'hh, read as Windows-1252), common
// typographic symbols. Fonts families, paragraph formatting, tables, images
// and other destinations are ignored.
// Writes the same subset (UTF-8 text in, \uN escapes out).

#include <string>
#include <vector>


namespace rtf {

struct Run {
	Run()
		:
		offset(0), bold(false), italic(false), underline(false), size(0),
		color(-1)
	{
	}

	bool SameFormat(const Run& o) const
	{
		return bold == o.bold && italic == o.italic
			&& underline == o.underline && size == o.size && color == o.color;
	}

	size_t	offset;
	bool	bold;
	bool	italic;
	bool	underline;
	int		size;		// points; 0 = default
	int		color;		// 0xRRGGBB; -1 = default
};


// The format applies from `offset` until the next run starts.
struct Document {
	std::string			text;		// UTF-8
	std::vector<Run>	runs;
};


Document Parse(const std::string& source);
std::string Write(const Document& document);

}	// namespace rtf

#endif	// RTF_H
