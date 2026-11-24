/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "EditorView.h"

#include <File.h>

#include <string>


EditorView::EditorView(const entry_ref& ref)
	:
	BScrollView(ref.name, new BTextView("text"),
		B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER),
	fRef(ref)
{
	BTextView* text = TextView();
	text->SetWordWrap(true);
	text->SetStylable(false);
	text->SetFontAndColor(be_fixed_font);
}


BTextView*
EditorView::TextView() const
{
	return static_cast<BTextView*>(Target());
}


status_t
EditorView::Load()
{
	BFile file(&fRef, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	off_t size;
	status = file.GetSize(&size);
	if (status != B_OK)
		return status;

	std::string buffer((size_t)size, '\0');
	if (size > 0) {
		ssize_t read = file.Read(&buffer[0], (size_t)size);
		if (read < 0)
			return (status_t)read;
		buffer.resize((size_t)read);
	}

	TextView()->SetText(buffer.c_str(), (int32)buffer.size());
	return B_OK;
}


status_t
EditorView::Save()
{
	BFile file(&fRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BTextView* text = TextView();
	ssize_t written = file.Write(text->Text(), (size_t)text->TextLength());
	return written < 0 ? (status_t)written : B_OK;
}
