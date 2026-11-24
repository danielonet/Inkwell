/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include <Entry.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <TextView.h>


// Sent to the notify target whenever a document's modified state flips.
enum {
	MSG_MODIFIED_CHANGED	= 'edmc'
};


// BTextView that tracks whether its content differs from what is on disk.
class EditorText : public BTextView {
public:
								EditorText(const char* name);

			bool				IsModified() const { return fModified; }
			void				SetModified(bool modified);
			void				SetNotifyTarget(const BMessenger& target);

protected:
	virtual	void				InsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

private:
			bool				fModified;
			BMessenger			fTarget;
};


// One open file: a scrollable text view bound to an entry_ref.
class EditorView : public BScrollView {
public:
								EditorView(const entry_ref& ref);

			const entry_ref&	Ref() const { return fRef; }
			EditorText*			TextView() const;
			bool				IsModified() const;

			status_t			Load();
			status_t			Save();

private:
			entry_ref			fRef;
};

#endif	// EDITOR_VIEW_H
