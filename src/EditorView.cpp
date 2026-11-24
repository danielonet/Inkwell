/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "EditorView.h"

#include <File.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <MessageRunner.h>
#include <ScrollView.h>
#include <SplitView.h>

#include <strings.h>
#include <string>

#include "MarkdownView.h"


static const bigtime_t kPreviewDelay = 250000;	// 0.25 s after the last edit


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
EditorText::SetChangeTarget(const BMessenger& target)
{
	fChangeTarget = target;
}


void
EditorText::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	BTextView::InsertText(text, length, offset, runs);
	SetModified(true);
	if (fChangeTarget.IsValid())
		fChangeTarget.SendMessage(MSG_TEXT_CHANGED);
}


void
EditorText::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	SetModified(true);
	if (fChangeTarget.IsValid())
		fChangeTarget.SendMessage(MSG_TEXT_CHANGED);
}


// #pragma mark - EditorView


static bool
IsMarkdownName(const char* name)
{
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return false;
	return strcasecmp(dot, ".md") == 0 || strcasecmp(dot, ".markdown") == 0
		|| strcasecmp(dot, ".mdown") == 0 || strcasecmp(dot, ".mkd") == 0;
}


EditorView::EditorView(const entry_ref& ref)
	:
	BView(ref.name, B_FRAME_EVENTS),
	fRef(ref),
	fText(new EditorText("text")),
	fTextScroll(NULL),
	fPreview(NULL),
	fPreviewScroll(NULL),
	fPreviewShown(false),
	fPreviewRunner(NULL)
{
	fText->SetWordWrap(true);
	fText->SetStylable(false);
	fText->SetFontAndColor(be_fixed_font);

	fTextScroll = new BScrollView("editor scroll", fText,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);

	if (IsMarkdownName(ref.name)) {
		fPreview = new MarkdownView("preview");
		fPreviewScroll = new BScrollView("preview scroll", fPreview,
			B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
		fPreviewShown = true;

		BSplitView* split = new BSplitView(B_HORIZONTAL, 0);
		split->AddChild(fTextScroll, 1);
		split->AddChild(fPreviewScroll, 1);
		BLayoutBuilder::Group<>(this, B_VERTICAL, 0).Add(split);
	} else {
		BLayoutBuilder::Group<>(this, B_VERTICAL, 0).Add(fTextScroll);
	}
}


EditorView::~EditorView()
{
	delete fPreviewRunner;
}


void
EditorView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (fPreview != NULL)
		fText->SetChangeTarget(BMessenger(this));
}


void
EditorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TEXT_CHANGED:
			_SchedulePreviewUpdate();
			break;

		case MSG_PREVIEW_UPDATE:
			delete fPreviewRunner;
			fPreviewRunner = NULL;
			_UpdatePreview();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
EditorView::IsModified() const
{
	return fText->IsModified();
}


void
EditorView::SetPreviewShown(bool show)
{
	if (fPreview == NULL || show == fPreviewShown)
		return;

	fPreviewShown = show;
	if (show) {
		fPreviewScroll->Show();
		_UpdatePreview();
	} else
		fPreviewScroll->Hide();
}


// Typing fires many edits; render once they pause.
void
EditorView::_SchedulePreviewUpdate()
{
	if (!fPreviewShown || fPreviewRunner != NULL)
		return;

	BMessage update(MSG_PREVIEW_UPDATE);
	fPreviewRunner = new BMessageRunner(BMessenger(this), &update,
		kPreviewDelay, 1);
	if (fPreviewRunner->InitCheck() != B_OK) {
		delete fPreviewRunner;
		fPreviewRunner = NULL;
	}
}


void
EditorView::_UpdatePreview()
{
	if (fPreview == NULL || !fPreviewShown)
		return;
	fPreview->SetMarkdown(fText->Text(), (size_t)fText->TextLength());
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

	fText->SetText(buffer.c_str(), (int32)buffer.size());
	fText->SetModified(false);
	_UpdatePreview();
	return B_OK;
}


status_t
EditorView::Save()
{
	BFile file(&fRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	ssize_t written = file.Write(fText->Text(), (size_t)fText->TextLength());
	if (written < 0)
		return (status_t)written;

	fText->SetModified(false);
	return B_OK;
}
