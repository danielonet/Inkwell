/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef MARKDOWN_VIEW_H
#define MARKDOWN_VIEW_H

#include <TextView.h>


// Read-only view showing rendered Markdown (styled text).
// TODO: clickable links, follow system color/theme changes.
class MarkdownView : public BTextView {
public:
								MarkdownView(const char* name);

			void				SetMarkdown(const char* text, size_t length);
};

#endif	// MARKDOWN_VIEW_H
