/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "MarkdownView.h"

#include <InterfaceDefs.h>
#include <Message.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Window.h>

#include <stdlib.h>


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


MarkdownView::MarkdownView(const char* name)
	:
	BTextView(name),
	fLinkCursor(B_CURSOR_ID_FOLLOW_LINK)
{
	MakeEditable(false);
	SetStylable(true);
	SetWordWrap(true);
	SetInsets(12, 8, 12, 8);

	SetTheme(ThemeAt(0));
}


void
MarkdownView::SetMarkdown(const char* text, size_t length)
{
	fSource.assign(text, length);
	_Render();
}


void
MarkdownView::SetTheme(const Theme& theme)
{
	fTextColor = theme.text;
	fLinkColor = theme.link;
	fQuoteColor = theme.quote;
	SetViewColor(theme.background);
	SetLowColor(theme.background);

	_Render();
	Invalidate();
}


void
MarkdownView::SetLinkTarget(const BMessenger& target)
{
	fLinkTarget = target;
}


void
MarkdownView::MouseDown(BPoint where)
{
	uint32 buttons = 0;
	BMessage* current = Window() != NULL ? Window()->CurrentMessage() : NULL;
	if (current != NULL)
		current->FindInt32("buttons", (int32*)&buttons);

	// Activate links immediately, like a browser; everything else (text
	// selection) is left to BTextView.
	const std::string* url = _UrlAt(where);
	if (url != NULL && (buttons & B_PRIMARY_MOUSE_BUTTON) != 0) {
		BMessage message(MSG_LINK_CLICKED);
		message.AddString("url", url->c_str());
		fLinkTarget.SendMessage(&message);
		return;
	}

	BTextView::MouseDown(where);
}


void
MarkdownView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	BTextView::MouseMoved(where, transit, dragMessage);

	if (transit != B_EXITED_VIEW && _UrlAt(where) != NULL)
		SetViewCursor(&fLinkCursor);
}


void
MarkdownView::_Render()
{
	markdown::Document doc = markdown::Render(fSource);
	fRuns = doc.runs;
	fUrls = doc.urls;

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
		const markdown::Style& style = doc.runs[i].style;
		runs->runs[i].offset = (int32)doc.runs[i].offset;
		runs->runs[i].font = FontFor(style);
		runs->runs[i].color = style.link ? fLinkColor
			: (style.quote ? fQuoteColor : fTextColor);
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


// The link under the mouse, if any. The character is chosen by which side of
// the nearest boundary the pointer is on, so a click only counts on the glyph.
const std::string*
MarkdownView::_UrlAt(BPoint where) const
{
	int32 offset = OffsetAt(where);
	BPoint boundary = PointAt(offset);
	int32 character = where.x < boundary.x ? offset - 1 : offset;
	if (character < 0 || character >= TextLength())
		return NULL;

	for (size_t i = fRuns.size(); i > 0; i--) {
		if (fRuns[i - 1].offset <= (size_t)character) {
			int url = fRuns[i - 1].style.url;
			if (url >= 0 && (size_t)url < fUrls.size())
				return &fUrls[url];
			return NULL;
		}
	}
	return NULL;
}
