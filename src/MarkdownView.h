/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef MARKDOWN_VIEW_H
#define MARKDOWN_VIEW_H

#include <Cursor.h>
#include <Messenger.h>
#include <TextView.h>

#include <string>
#include <vector>

#include "MarkdownRender.h"
#include "Theme.h"


// Sent to the link target when a link is clicked; string field "url".
enum {
	MSG_LINK_CLICKED	= 'lnkc'
};


// Read-only view showing rendered Markdown (styled text) with clickable links.
class MarkdownView : public BTextView {
public:
								MarkdownView(const char* name);

			void				SetMarkdown(const char* text, size_t length);
			void				SetTheme(const Theme& theme);
			void				SetLinkTarget(const BMessenger& target);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

private:
			void				_Render();
			const std::string*	_UrlAt(BPoint where) const;

			std::string			fSource;
			std::vector<markdown::Run> fRuns;
			std::vector<std::string> fUrls;
			rgb_color			fTextColor;
			rgb_color			fLinkColor;
			rgb_color			fQuoteColor;
			BCursor				fLinkCursor;
			BMessenger			fLinkTarget;
};

#endif	// MARKDOWN_VIEW_H
