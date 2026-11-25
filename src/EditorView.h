/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include <DataIO.h>
#include <Entry.h>
#include <Messenger.h>
#include <TextView.h>
#include <View.h>

#include <string>
#include <vector>

#include "Spell.h"
#include "Theme.h"

class BMessageRunner;
class BScrollView;
class MarkdownView;


// Sent to the notify target whenever a document's modified state flips.
enum {
	MSG_MODIFIED_CHANGED	= 'edmc',
	MSG_TEXT_CHANGED		= 'edtc',	// every edit, sent to the change target
	MSG_PREVIEW_UPDATE		= 'edpu',
	MSG_SPELLING_CHANGED	= 'edsc'	// personal word added: recheck all tabs
};


// BTextView that tracks whether its content differs from what is on disk.
class EditorText : public BTextView {
public:
								EditorText(const char* name);
	virtual						~EditorText();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			bool				IsModified() const { return fModified; }
			void				SetModified(bool modified);
			void				SetNotifyTarget(const BMessenger& target);
			void				SetChangeTarget(const BMessenger& target);

			rgb_color			TextColor() const { return fTextColor; }
			void				ApplyTheme(const Theme& theme);

			// Rich text formatting of the selection (or of what is typed next).
			void				ToggleFace(uint16 face);
			void				ChangeSize(float delta);

			// Red squiggles under misspelled words, suggestions on right click.
			void				SetSpellChecking(bool active, bool markdown);

protected:
	virtual	void				InsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

private:
			void				_ScheduleSpellCheck();
			void				_RunSpellCheck();
			void				_ShiftMisspellings(int32 offset, int32 delta);
			const spell::Range*	_MisspellingAt(int32 offset) const;
			void				_ShowSuggestions(BPoint where,
									const spell::Range& range);

			bool				fModified;
			rgb_color			fTextColor;
			BMessenger			fTarget;
			BMessenger			fChangeTarget;

			bool				fSpellActive;
			bool				fMarkdown;
			std::vector<spell::Range> fMisspelled;
			BMessageRunner*		fSpellRunner;
};


// One open file: an editor, plus a live Markdown preview beside it for
// Markdown files (.md, .markdown, ...). .rtf files are edited as rich text.
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
			bool				IsRtf() const { return fIsRtf; }
			bool				IsPreviewShown() const { return fPreviewShown; }
			void				SetPreviewShown(bool show);

			void				ApplyTheme(const Theme& theme);

			// Re-reads the spell checking settings (on/off, language, words).
			void				UpdateSpelling();

			status_t			Load();
			status_t			Save();

private:
			status_t			_LoadRtf(const std::string& data);
			status_t			_SaveRtf(BPositionIO& file);
			void				_SchedulePreviewUpdate();
			void				_UpdatePreview();

			entry_ref			fRef;
			EditorText*			fText;
			BScrollView*		fTextScroll;
			MarkdownView*		fPreview;		// NULL for non-Markdown files
			BScrollView*		fPreviewScroll;
			bool				fIsRtf;
			bool				fPreviewShown;
			BMessageRunner*		fPreviewRunner;
};

#endif	// EDITOR_VIEW_H
