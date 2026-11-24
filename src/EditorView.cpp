/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "EditorView.h"

#include <File.h>
#include <Message.h>

#include <string>


// #pragma mark - EditorText


EditorText::EditorText(const char* name)
	:
	BTextView(name),
	fModified(false)
{
}


void
EditorText::SetModified(bool modified)
{
	if (fModified == modified)
		return;

	fModified = modified;
	if (fTarget.IsValid())
		fTarget.SendMessage(MSG_MODIFIED_CHANGED);
}


void
EditorText::SetNotifyTarget(const BMessenger& target)
{
	fTarget = target;
}


void
EditorText::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	BTextView::InsertText(text, length, offset, runs);
	SetModified(true);
}


void
EditorText::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	SetModified(true);
}


// #pragma mark - EditorView


EditorView::EditorView(const entry_ref& ref)
	:
	BScrollView(ref.name, new EditorText("text"),
		B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER),
	fRef(ref)
{
	EditorText* text = TextView();
	text->SetWordWrap(true);
	text->SetStylable(false);
	text->SetFontAndColor(be_fixed_font);
}


EditorText*
EditorView::TextView() const
{
	return static_cast<EditorText*>(Target());
}


bool
EditorView::IsModified() const
{
	return TextView()->IsModified();
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

	EditorText* text = TextView();
	text->SetText(buffer.c_str(), (int32)buffer.size());
	text->SetModified(false);
	return B_OK;
}


status_t
EditorView::Save()
{
	BFile file(&fRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	EditorText* text = TextView();
	ssize_t written = file.Write(text->Text(), (size_t)text->TextLength());
	if (written < 0)
		return (status_t)written;

	text->SetModified(false);
	return B_OK;
}
