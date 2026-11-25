/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "MarkdownView.h"

#include <Font.h>
#include <InterfaceDefs.h>
#include <ScrollBar.h>
#include <ScrollView.h>

#include <stdlib.h>
#include <string>

#include "MarkdownRender.h"


static const float kHeadingScale[7] = {
	1.0f, 1.8f, 1.5f, 1.3f, 1.15f, 1.05f, 1.0f
};


static BFont
FontFor(const markdown::Style& style)
{
	BFont font(style.code ? be_fixed_font : be_plain_font);

	uint16 face = 0;
	if (style.bold || style.heading > 0)
		face |= B_BOLD_FACE;
	if (style.italic)
		face |= B_ITALIC_FACE;
	if (style.link)
		face |= B_UNDERSCORE_FACE;
	font.SetFace(face != 0 ? face : B_REGULAR_FACE);

	if (style.heading > 0)
		font.SetSize(font.Size() * kHeadingScale[style.heading]);
	return font;
}


static rgb_color
ColorFor(const markdown::Style& style)
{
	rgb_color text = ui_color(B_DOCUMENT_TEXT_COLOR);
	if (style.link)
		return ui_color(B_LINK_TEXT_COLOR);
	if (style.quote)
		return tint_color(text, B_DISABLED_LABEL_TINT);
	return text;
}


MarkdownView::MarkdownView(const char* name)
	:
	BTextView(name)
{
	MakeEditable(false);
	SetStylable(true);
	SetWordWrap(true);
	SetInsets(12, 8, 12, 8);
}


void
MarkdownView::SetMarkdown(const char* text, size_t length)
{
	markdown::Document doc = markdown::Render(std::string(text, length));

	size_t count = doc.runs.size();
	if (count == 0) {
		SetText("");
		return;
	}

	text_run_array* runs = (text_run_array*)malloc(sizeof(text_run_array)
		+ (count - 1) * sizeof(text_run));
	if (runs == NULL)
		return;

	runs->count = (int32)count;
	for (size_t i = 0; i < count; i++) {
		runs->runs[i].offset = (int32)doc.runs[i].offset;
		runs->runs[i].font = FontFor(doc.runs[i].style);
		runs->runs[i].color = ColorFor(doc.runs[i].style);
	}

	// Keep the reading position when the preview refreshes while typing.
	BScrollView* scroll = dynamic_cast<BScrollView*>(Parent());
	BScrollBar* bar = scroll != NULL ? scroll->ScrollBar(B_VERTICAL) : NULL;
	float position = bar != NULL ? bar->Value() : 0;

	SetText(doc.text.c_str(), (int32)doc.text.size(), runs);
	free(runs);

	if (position > 0)
		ScrollTo(0, position);
}
