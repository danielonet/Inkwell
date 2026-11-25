/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef THEME_H
#define THEME_H

#include <GraphicsDefs.h>


// Colors for the editor and the Markdown preview. Index 0 is "System": it
// follows the Appearance preferences; the others are fixed palettes.
struct Theme {
	const char*	id;			// stable name stored in the settings file
	rgb_color	background;
	rgb_color	text;
	rgb_color	link;
	rgb_color	quote;
};


int			ThemeCount();
Theme		ThemeAt(int index);				// out of range -> System
int			ThemeIndexForId(const char* id);	// unknown -> 0 (System)
bool		SameColor(const rgb_color& a, const rgb_color& b);

#endif	// THEME_H
