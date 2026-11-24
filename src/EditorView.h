/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include <Entry.h>
#include <Messenger.h>
#include <TextView.h>
#include <View.h>

class BMessageRunner;
class BScrollView;
class MarkdownView;


// Sent to the notify target whenever a document's modified state flips.
enum {
	MSG_MODIFIED_CHANGED	= 'edmc',
	MSG_TEXT_CHANGED		= 'edtc',	// every edit, sent to the change target
	MSG_PREVIEW_UPDATE		= 'edpu'
};


// BTextView that tracks whether its content differs from what is on disk.
class EditorText : public BTextView {
public:
								EditorText(const char* name);

			bool				IsModified() const { return fModified; }
			void				SetModified(bool modified);
			void				SetNotifyTarget(const BMessenger& target);
			void				SetChangeTarget(const BMessenger& target);

protected:
	virtual	void				InsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

private:
			bool				fModified;
			BMessenger			fTarget;
			BMessenger			fChangeTarget;
};


// One open file: an editor, plus a live Markdown preview beside it for
// Markdown files (.md, .markdown, ...).
class EditorView : public BView {
public:
								EditorView(const entry_ref& ref);
	virtual						~EditorView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			const entry_ref&	Ref() const { return fRef; }
			EditorText*			TextView() const { return fText; }
			bool				IsModified() const;

			bool				IsMarkdown() const { return fPreview != NULL; }
			bool				IsPreviewShown() const { return fPreviewShown; }
			void				SetPreviewShown(bool show);

			status_t			Load();
			status_t			Save();

private:
			void				_SchedulePreviewUpdate();
			void				_UpdatePreview();

			entry_ref			fRef;
			EditorText*			fText;
			BScrollView*		fTextScroll;
			MarkdownView*		fPreview;		// NULL for non-Markdown files
			BScrollView*		fPreviewScroll;
			bool				fPreviewShown;
			BMessageRunner*		fPreviewRunner;
};

#endif	// EDITOR_VIEW_H
