/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "EditorView.h"

#include <Catalog.h>
#include <File.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <Window.h>

#include <stdlib.h>
#include <strings.h>
#include <string>

#include "MarkdownView.h"
#include "Rtf.h"
#include "SpellService.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorView"


static const bigtime_t kPreviewDelay = 250000;	// 0.25 s after the last edit
static const float kMinFontSize = 6.0f;
static const float kMaxFontSize = 96.0f;
static const bigtime_t kSpellDelay = 500000;	// 0.5 s after the last edit
static const size_t kMaxSpellCheckBytes = 2 * 1024 * 1024;
static const rgb_color kSquiggleColor = {200, 30, 30, 255};

enum {
	MSG_SPELL_CHECK		= 'spck',
	MSG_SPELL_REPLACE	= 'sprp',
	MSG_SPELL_ADD		= 'spad'
};


// #pragma mark - EditorText


EditorText::EditorText(const char* name)
	:
	BTextView(name),
	fModified(false),
	fTextColor(ui_color(B_DOCUMENT_TEXT_COLOR)),
	fSpellActive(false),
	fMarkdown(false),
	fSpellRunner(NULL)
{
}


EditorText::~EditorText()
{
	delete fSpellRunner;
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
EditorText::ApplyTheme(const Theme& theme)
{
	rgb_color oldColor = fTextColor;
	fTextColor = theme.text;

	SetViewColor(theme.background);
	SetLowColor(theme.background);

	if (!IsStylable()) {
		SetFontAndColor(be_fixed_font, B_FONT_ALL, &fTextColor);
		Invalidate();
		return;
	}

	// Rich text: only runs that used the old default color follow the theme;
	// explicitly colored text keeps its color.
	int32 selectionStart;
	int32 selectionEnd;
	GetSelection(&selectionStart, &selectionEnd);

	int32 length = TextLength();
	int32 size = 0;
	text_run_array* runs = length > 0 ? RunArray(0, length, &size) : NULL;
	if (runs != NULL) {
		for (int32 i = 0; i < runs->count; i++) {
			if (!SameColor(runs->runs[i].color, oldColor))
				continue;
			int32 start = runs->runs[i].offset;
			int32 end = i + 1 < runs->count ? runs->runs[i + 1].offset
				: length;
			Select(start, end);
			SetFontAndColor(&runs->runs[i].font, B_FONT_ALL, &fTextColor);
		}
		free(runs);
	}

	Select(selectionStart, selectionEnd);
	if (selectionStart == selectionEnd) {
		// Style of the text typed next.
		BFont font;
		GetFontAndColor(&font);
		SetFontAndColor(&font, B_FONT_ALL, &fTextColor);
	}
	Invalidate();
}


void
EditorText::ToggleFace(uint16 face)
{
	BFont font;
	GetFontAndColor(&font);

	uint16 faces = font.Face();
	if ((faces & face) != 0)
		faces &= ~face;
	else
		faces |= face;

	// B_REGULAR_FACE is only valid on its own.
	const uint16 styleBits = B_BOLD_FACE | B_ITALIC_FACE | B_UNDERSCORE_FACE;
	if ((faces & styleBits) == 0)
		faces = B_REGULAR_FACE;
	else
		faces &= ~B_REGULAR_FACE;

	font.SetFace(faces);
	SetFontAndColor(&font, B_FONT_FACE);
	SetModified(true);
}


void
EditorText::ChangeSize(float delta)
{
	BFont font;
	GetFontAndColor(&font);

	float size = font.Size() + delta;
	if (size < kMinFontSize)
		size = kMinFontSize;
	if (size > kMaxFontSize)
		size = kMaxFontSize;

	font.SetSize(size);
	SetFontAndColor(&font, B_FONT_SIZE);
	SetModified(true);
}


void
EditorText::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	BTextView::InsertText(text, length, offset, runs);
	SetModified(true);
	if (fChangeTarget.IsValid())
		fChangeTarget.SendMessage(MSG_TEXT_CHANGED);
	if (fSpellActive) {
		_ShiftMisspellings(offset, length);
		_ScheduleSpellCheck();
	}
}


void
EditorText::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	SetModified(true);
	if (fChangeTarget.IsValid())
		fChangeTarget.SendMessage(MSG_TEXT_CHANGED);
	if (fSpellActive) {
		// Drop what overlaps the removed text, then close the gap.
		int32 removed = toOffset - fromOffset;
		std::vector<spell::Range> kept;
		for (size_t i = 0; i < fMisspelled.size(); i++) {
			const spell::Range& r = fMisspelled[i];
			if ((int32)r.end <= fromOffset) {
				kept.push_back(r);
			} else if ((int32)r.begin >= toOffset) {
				spell::Range moved;
				moved.begin = r.begin - removed;
				moved.end = r.end - removed;
				kept.push_back(moved);
			}
		}
		fMisspelled.swap(kept);
		_ScheduleSpellCheck();
	}
}


// #pragma mark - EditorText: spell checking


void
EditorText::SetSpellChecking(bool active, bool markdown)
{
	fSpellActive = active;
	fMarkdown = markdown;

	delete fSpellRunner;
	fSpellRunner = NULL;

	if (active)
		_RunSpellCheck();
	else {
		fMisspelled.clear();
		Invalidate();
	}
}


void
EditorText::_ScheduleSpellCheck()
{
	if (fSpellRunner != NULL)
		return;

	BMessage check(MSG_SPELL_CHECK);
	fSpellRunner = new BMessageRunner(BMessenger(this), &check, kSpellDelay, 1);
	if (fSpellRunner->InitCheck() != B_OK) {
		delete fSpellRunner;
		fSpellRunner = NULL;
	}
}


void
EditorText::_RunSpellCheck()
{
	fMisspelled.clear();

	const spell::Dictionary* dictionary
		= SpellService::Instance().Dictionary();
	if (dictionary != NULL && (size_t)TextLength() <= kMaxSpellCheckBytes) {
		fMisspelled = spell::FindMisspellings(*dictionary,
			std::string(Text(), (size_t)TextLength()), fMarkdown);
	}
	Invalidate();
}


// Text was inserted at `offset`: keep the underlines on the right words until
// the next check runs. A word that was edited in the middle is dropped.
void
EditorText::_ShiftMisspellings(int32 offset, int32 delta)
{
	std::vector<spell::Range> kept;
	for (size_t i = 0; i < fMisspelled.size(); i++) {
		spell::Range r = fMisspelled[i];
		if ((int32)r.end <= offset) {
			kept.push_back(r);
		} else if ((int32)r.begin >= offset) {
			r.begin += delta;
			r.end += delta;
			kept.push_back(r);
		}
	}
	fMisspelled.swap(kept);
}


const spell::Range*
EditorText::_MisspellingAt(int32 offset) const
{
	for (size_t i = 0; i < fMisspelled.size(); i++) {
		if ((int32)fMisspelled[i].begin <= offset
			&& offset <= (int32)fMisspelled[i].end) {
			return &fMisspelled[i];
		}
	}
	return NULL;
}


void
EditorText::Draw(BRect updateRect)
{
	BTextView::Draw(updateRect);
	if (!fSpellActive || fMisspelled.empty())
		return;

	PushState();
	SetHighColor(kSquiggleColor);
	for (size_t i = 0; i < fMisspelled.size(); i++) {
		float startHeight;
		float endHeight;
		BPoint start = PointAt((int32)fMisspelled[i].begin, &startHeight);
		BPoint end = PointAt((int32)fMisspelled[i].end, &endHeight);
		if (start.y != end.y)
			continue;	// word wrapped across lines: skip the underline

		float y = start.y + startHeight - 1;
		if (y < updateRect.top - 2 || y > updateRect.bottom + 2)
			continue;

		// Zigzag, two pixels per half wave.
		bool up = false;
		BPoint previous(start.x, y);
		for (float x = start.x + 2; x < end.x + 2; x += 2) {
			BPoint next(x < end.x ? x : end.x, up ? y : y - 2);
			StrokeLine(previous, next);
			previous = next;
			up = !up;
		}
	}
	PopState();
}


void
EditorText::MouseDown(BPoint where)
{
	uint32 buttons = 0;
	BMessage* current = Window() != NULL ? Window()->CurrentMessage() : NULL;
	if (current != NULL)
		current->FindInt32("buttons", (int32*)&buttons);

	if (fSpellActive && (buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		const spell::Range* range = _MisspellingAt(OffsetAt(where));
		if (range != NULL) {
			_ShowSuggestions(where, *range);
			return;
		}
	}
	BTextView::MouseDown(where);
}


void
EditorText::_ShowSuggestions(BPoint where, const spell::Range& range)
{
	const spell::Dictionary* dictionary
		= SpellService::Instance().Dictionary();
	if (dictionary == NULL)
		return;

	std::string word(Text() + range.begin, range.end - range.begin);
	std::vector<std::string> suggestions = dictionary->Suggest(word);

	BPopUpMenu* menu = new BPopUpMenu("spelling", false, false);
	for (size_t i = 0; i < suggestions.size(); i++) {
		BMessage* message = new BMessage(MSG_SPELL_REPLACE);
		message->AddInt32("begin", (int32)range.begin);
		message->AddInt32("end", (int32)range.end);
		message->AddString("word", word.c_str());
		message->AddString("replacement", suggestions[i].c_str());
		menu->AddItem(new BMenuItem(suggestions[i].c_str(), message));
	}
	if (suggestions.empty()) {
		BMenuItem* none = new BMenuItem(B_TRANSLATE("(no suggestions)"), NULL);
		none->SetEnabled(false);
		menu->AddItem(none);
	}
	menu->AddSeparatorItem();

	BMessage* add = new BMessage(MSG_SPELL_ADD);
	add->AddString("word", word.c_str());
	menu->AddItem(new BMenuItem(B_TRANSLATE("Add to dictionary"), add));

	menu->SetTargetForItems(this);
	// Asynchronous: the menu deletes itself when it closes.
	menu->Go(ConvertToScreen(where), true, true, true);
}


void
EditorText::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SPELL_CHECK:
			delete fSpellRunner;
			fSpellRunner = NULL;
			if (fSpellActive)
				_RunSpellCheck();
			break;

		case MSG_SPELL_REPLACE:
		{
			int32 begin;
			int32 end;
			const char* word;
			const char* replacement;
			if (message->FindInt32("begin", &begin) != B_OK
				|| message->FindInt32("end", &end) != B_OK
				|| message->FindString("word", &word) != B_OK
				|| message->FindString("replacement", &replacement) != B_OK) {
				break;
			}

			// The text may have changed while the menu was open.
			if (begin < 0 || end > TextLength() || end <= begin
				|| std::string(Text() + begin, end - begin) != word) {
				break;
			}
			Select(begin, end);
			Delete();
			Insert(replacement);
			break;
		}

		case MSG_SPELL_ADD:
		{
			const char* word;
			if (message->FindString("word", &word) != B_OK)
				break;
			SpellService::Instance().AddPersonalWord(word);
			_RunSpellCheck();
			// Other open tabs should forget the word too.
			if (fTarget.IsValid())
				fTarget.SendMessage(MSG_SPELLING_CHANGED);
			break;
		}

		default:
			BTextView::MessageReceived(message);
			break;
	}
}


// #pragma mark - EditorView


static bool
HasExtension(const char* name, const char* const* extensions)
{
	const char* dot = strrchr(name, '.');
	if (dot == NULL)
		return false;
	for (int i = 0; extensions[i] != NULL; i++) {
		if (strcasecmp(dot, extensions[i]) == 0)
			return true;
	}
	return false;
}


static bool
IsMarkdownName(const char* name)
{
	static const char* const kExtensions[]
		= {".md", ".markdown", ".mdown", ".mkd", NULL};
	return HasExtension(name, kExtensions);
}


static bool
IsRtfName(const char* name)
{
	static const char* const kExtensions[] = {".rtf", NULL};
	return HasExtension(name, kExtensions);
}


EditorView::EditorView(const entry_ref& ref)
	:
	BView(ref.name, B_FRAME_EVENTS),
	fRef(ref),
	fText(new EditorText("text")),
	fTextScroll(NULL),
	fPreview(NULL),
	fPreviewScroll(NULL),
	fIsRtf(IsRtfName(ref.name)),
	fPreviewShown(false),
	fPreviewRunner(NULL)
{
	fText->SetWordWrap(true);
	if (fIsRtf) {
		fText->SetStylable(true);
		fText->SetFontAndColor(be_plain_font);
	} else {
		fText->SetStylable(false);
		fText->SetFontAndColor(be_fixed_font);
	}

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
	if (fPreview != NULL) {
		fText->SetChangeTarget(BMessenger(this));
		// Link clicks are handled by the window (it knows the other tabs).
		fPreview->SetLinkTarget(BMessenger(Window()));
	}
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


void
EditorView::UpdateSpelling()
{
	SpellService& service = SpellService::Instance();
	bool active = service.IsEnabled() && SpellService::IsProseFile(fRef.name)
		&& service.Dictionary() != NULL;
	fText->SetSpellChecking(active, fPreview != NULL);
}


void
EditorView::ApplyTheme(const Theme& theme)
{
	fText->ApplyTheme(theme);
	if (fPreview != NULL)
		fPreview->SetTheme(theme);
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

	if (fIsRtf) {
		status = _LoadRtf(buffer);
		if (status != B_OK)
			return status;
	} else
		fText->SetText(buffer.c_str(), (int32)buffer.size());

	fText->SetModified(false);
	_UpdatePreview();
	UpdateSpelling();
	return B_OK;
}


status_t
EditorView::Save()
{
	BFile file(&fRef, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	if (fIsRtf) {
		status = _SaveRtf(file);
	} else {
		ssize_t written = file.Write(fText->Text(),
			(size_t)fText->TextLength());
		status = written < 0 ? (status_t)written : B_OK;
	}
	if (status != B_OK)
		return status;

	fText->SetModified(false);
	return B_OK;
}


// RTF -> styled BTextView content.
status_t
EditorView::_LoadRtf(const std::string& data)
{
	rtf::Document doc = rtf::Parse(data);

	size_t count = doc.runs.size();
	text_run_array* runs = NULL;
	if (count > 0) {
		runs = (text_run_array*)malloc(sizeof(text_run_array)
			+ (count - 1) * sizeof(text_run));
		if (runs == NULL)
			return B_NO_MEMORY;

		runs->count = (int32)count;
		for (size_t i = 0; i < count; i++) {
			const rtf::Run& run = doc.runs[i];

			uint16 face = 0;
			if (run.bold)
				face |= B_BOLD_FACE;
			if (run.italic)
				face |= B_ITALIC_FACE;
			if (run.underline)
				face |= B_UNDERSCORE_FACE;

			BFont font(be_plain_font);
			font.SetFace(face != 0 ? face : B_REGULAR_FACE);
			if (run.size > 0)
				font.SetSize((float)run.size);

			rgb_color color = fText->TextColor();
			if (run.color >= 0) {
				color.red = (run.color >> 16) & 255;
				color.green = (run.color >> 8) & 255;
				color.blue = run.color & 255;
			}

			runs->runs[i].offset = (int32)run.offset;
			runs->runs[i].font = font;
			runs->runs[i].color = color;
		}
	}

	fText->SetText(doc.text.c_str(), (int32)doc.text.size(), runs);
	free(runs);
	return B_OK;
}


// Styled BTextView content -> RTF. Text in the theme's default color is
// written without a color, so it follows whatever theme is active later.
status_t
EditorView::_SaveRtf(BPositionIO& file)
{
	rtf::Document doc;
	int32 length = fText->TextLength();
	doc.text.assign(fText->Text(), (size_t)length);

	int32 size = 0;
	text_run_array* runs = length > 0 ? fText->RunArray(0, length, &size)
		: NULL;
	if (runs != NULL) {
		for (int32 i = 0; i < runs->count; i++) {
			const BFont& font = runs->runs[i].font;
			const rgb_color& color = runs->runs[i].color;
			uint16 face = font.Face();

			rtf::Run run;
			run.offset = (size_t)runs->runs[i].offset;
			run.bold = (face & B_BOLD_FACE) != 0;
			run.italic = (face & B_ITALIC_FACE) != 0;
			run.underline = (face & B_UNDERSCORE_FACE) != 0;
			run.size = (int)(font.Size() + 0.5f);
			run.color = SameColor(color, fText->TextColor()) ? -1
				: ((color.red << 16) | (color.green << 8) | color.blue);
			doc.runs.push_back(run);
		}
		free(runs);
	}

	std::string out = rtf::Write(doc);
	ssize_t written = file.Write(out.data(), out.size());
	return written < 0 ? (status_t)written : B_OK;
}
