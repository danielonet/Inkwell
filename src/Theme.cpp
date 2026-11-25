/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "Theme.h"

#include <InterfaceDefs.h>

#include <string.h>


static rgb_color
Rgb(uint8 r, uint8 g, uint8 b)
{
	rgb_color color = {r, g, b, 255};
	return color;
}


// `amount` of `b` mixed into `a` (0..1).
static rgb_color
Mix(const rgb_color& a, const rgb_color& b, float amount)
{
	return Rgb((uint8)(a.red + (b.red - a.red) * amount),
		(uint8)(a.green + (b.green - a.green) * amount),
		(uint8)(a.blue + (b.blue - a.blue) * amount));
}


enum {
	kSystem,
	kLight,
	kDark,
	kSepia,
	kThemeCount
};


int
ThemeCount()
{
	return kThemeCount;
}


Theme
ThemeAt(int index)
{
	Theme theme;
	switch (index) {
		case kLight:
			theme.id = "light";
			theme.background = Rgb(0xFF, 0xFF, 0xFF);
			theme.text = Rgb(0x1E, 0x1E, 0x1E);
			theme.link = Rgb(0x0B, 0x5F, 0xB0);
			theme.quote = Rgb(0x6B, 0x6B, 0x6B);
			break;

		case kDark:
			theme.id = "dark";
			theme.background = Rgb(0x1E, 0x1E, 0x1E);
			theme.text = Rgb(0xD8, 0xD8, 0xD8);
			theme.link = Rgb(0x6C, 0xB6, 0xFF);
			theme.quote = Rgb(0x9A, 0x9A, 0x9A);
			break;

		case kSepia:
			theme.id = "sepia";
			theme.background = Rgb(0xF4, 0xEC, 0xD8);
			theme.text = Rgb(0x3B, 0x2F, 0x20);
			theme.link = Rgb(0x8A, 0x4B, 0x08);
			theme.quote = Rgb(0x7A, 0x6A, 0x55);
			break;

		default:
			theme.id = "system";
			theme.background = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
			theme.text = ui_color(B_DOCUMENT_TEXT_COLOR);
			theme.link = ui_color(B_LINK_TEXT_COLOR);
			theme.quote = Mix(theme.text, theme.background, 0.4f);
			break;
	}
	return theme;
}


int
ThemeIndexForId(const char* id)
{
	if (id == NULL)
		return kSystem;
	for (int i = 0; i < kThemeCount; i++) {
		if (strcmp(ThemeAt(i).id, id) == 0)
			return i;
	}
	return kSystem;
}


bool
SameColor(const rgb_color& a, const rgb_color& b)
{
	return a.red == b.red && a.green == b.green && a.blue == b.blue;
}
