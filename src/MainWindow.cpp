/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <String.h>
#include <TabView.h>

#include <ctype.h>
#include <stdlib.h>

#include "EditorView.h"
#include "MarkdownView.h"
#include "Settings.h"
#include "SidebarView.h"
#include "Theme.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_OPEN_FOLDER		= 'opfd',
	MSG_FOLDER_CHOSEN	= 'fdch',
	MSG_OPEN_ITEM		= 'opit',
	MSG_SAVE			= 'save',
	MSG_CLOSE_TAB		= 'clst',
	MSG_TOGGLE_PREVIEW	= 'tgpv',
	MSG_SET_THEME		= 'sthm',
	MSG_BOLD			= 'bold',
	MSG_ITALIC			= 'ital',
	MSG_UNDERLINE		= 'undl',
	MSG_BIGGER			= 'bigr',
	MSG_SMALLER			= 'smlr'
};


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 1000, 700), B_TRANSLATE_SYSTEM_NAME("Inkwell"),
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fMenuBar(NULL),
	fSidebar(NULL),
	fTabView(NULL),
	fPreviewItem(NULL),
	fThemeMenu(NULL),
	fFormatMenu(NULL),
	fThemeIndex(ThemeIndexForId(Settings::LoadTheme().String())),
	fFolderPanel(NULL)
{
	fMenuBar = _BuildMenuBar();

	fSidebar = new SidebarView("sidebar");
	fSidebar->SetInvocationMessage(new BMessage(MSG_OPEN_ITEM));
	fSidebar->SetTarget(this);

	BScrollView* sidebarScroll = new BScrollView("sidebar scroll", fSidebar,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_FANCY_BORDER);

	fTabView = new BTabView("tabs", B_WIDTH_FROM_LABEL);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.AddSplit(B_HORIZONTAL, 0)
			.Add(sidebarScroll, 1)
			.Add(fTabView, 3)
		.End();

	BMessenger target(this);
	fFolderPanel = new BFilePanel(B_OPEN_PANEL, &target, NULL,
		B_DIRECTORY_NODE, false, new BMessage(MSG_FOLDER_CHOSEN));
}


MainWindow::~MainWindow()
{
	delete fFolderPanel;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_OPEN_FOLDER:
			fFolderPanel->Show();
			break;

		case MSG_FOLDER_CHOSEN:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK)
				fSidebar->SetFolder(ref);
			break;
		}

		case MSG_OPEN_ITEM:
		{
			FileItem* item = fSidebar->SelectedItem();
			if (item != NULL && !item->IsDirectory())
				_OpenFile(item->Ref());
			break;
		}

		case MSG_SAVE:
			_SaveCurrent();
			break;

		case MSG_CLOSE_TAB:
			_CloseCurrentTab();
			break;

		case MSG_TOGGLE_PREVIEW:
		{
			EditorView* editor = _CurrentEditor();
			if (editor != NULL && editor->IsMarkdown())
				editor->SetPreviewShown(!editor->IsPreviewShown());
			break;
		}

		case MSG_SET_THEME:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				_SetTheme((int)index);
			break;
		}

		case MSG_BOLD:
		case MSG_ITALIC:
		case MSG_UNDERLINE:
		case MSG_BIGGER:
		case MSG_SMALLER:
		{
			EditorView* editor = _CurrentEditor();
			if (editor == NULL || !editor->IsRtf())
				break;
			if (message->what == MSG_BOLD)
				editor->TextView()->ToggleFace(B_BOLD_FACE);
			else if (message->what == MSG_ITALIC)
				editor->TextView()->ToggleFace(B_ITALIC_FACE);
			else if (message->what == MSG_UNDERLINE)
				editor->TextView()->ToggleFace(B_UNDERSCORE_FACE);
			else
				editor->TextView()->ChangeSize(message->what == MSG_BIGGER
					? 1.0f : -1.0f);
			break;
		}

		case MSG_LINK_CLICKED:
		{
			const char* url;
			if (message->FindString("url", &url) == B_OK)
				_OpenLink(url);
			break;
		}

		case MSG_MODIFIED_CHANGED:
			_UpdateTabLabels();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	for (int32 i = 0; i < fTabView->CountTabs(); i++) {
		EditorView* editor
			= dynamic_cast<EditorView*>(fTabView->TabAt(i)->View());
		if (editor == NULL || !editor->IsModified())
			continue;

		fTabView->Select(i);
		if (!_ConfirmClose(editor))
			return false;
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


// The preview toggle only applies to the current tab, if it is Markdown.
void
MainWindow::MenusBeginning()
{
	BWindow::MenusBeginning();

	EditorView* editor = _CurrentEditor();
	bool markdown = editor != NULL && editor->IsMarkdown();
	fPreviewItem->SetEnabled(markdown);
	fPreviewItem->SetMarked(markdown && editor->IsPreviewShown());

	bool rtf = editor != NULL && editor->IsRtf();
	for (int32 i = 0; i < fFormatMenu->CountItems(); i++)
		fFormatMenu->ItemAt(i)->SetEnabled(rtf);

	for (int32 i = 0; i < fThemeMenu->CountItems(); i++)
		fThemeMenu->ItemAt(i)->SetMarked(i == fThemeIndex);
}


static const char*
_ThemeLabel(int index)
{
	switch (index) {
		case 1:
			return B_TRANSLATE("Light");
		case 2:
			return B_TRANSLATE("Dark");
		case 3:
			return B_TRANSLATE("Sepia");
		default:
			return B_TRANSLATE("System colors");
	}
}


BMenuBar*
MainWindow::_BuildMenuBar()
{
	BMenuBar* bar = new BMenuBar("menu bar");

	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open folder" B_UTF8_ELLIPSIS),
		new BMessage(MSG_OPEN_FOLDER), 'O'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(MSG_SAVE), 'S'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Close tab"),
		new BMessage(MSG_CLOSE_TAB), 'W'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	bar->AddItem(fileMenu);

	BMenu* viewMenu = new BMenu(B_TRANSLATE("View"));
	fPreviewItem = new BMenuItem(B_TRANSLATE("Markdown preview"),
		new BMessage(MSG_TOGGLE_PREVIEW), 'P');
	viewMenu->AddItem(fPreviewItem);

	fThemeMenu = new BMenu(B_TRANSLATE("Theme"));
	for (int i = 0; i < ThemeCount(); i++) {
		BMessage* message = new BMessage(MSG_SET_THEME);
		message->AddInt32("index", i);
		fThemeMenu->AddItem(new BMenuItem(_ThemeLabel(i), message));
	}
	viewMenu->AddItem(fThemeMenu);
	bar->AddItem(viewMenu);

	// Only enabled while a rich text (.rtf) tab is in front.
	fFormatMenu = new BMenu(B_TRANSLATE("Format"));
	fFormatMenu->AddItem(new BMenuItem(B_TRANSLATE("Bold"),
		new BMessage(MSG_BOLD), 'B'));
	fFormatMenu->AddItem(new BMenuItem(B_TRANSLATE("Italic"),
		new BMessage(MSG_ITALIC), 'I'));
	fFormatMenu->AddItem(new BMenuItem(B_TRANSLATE("Underline"),
		new BMessage(MSG_UNDERLINE), 'U'));
	fFormatMenu->AddSeparatorItem();
	fFormatMenu->AddItem(new BMenuItem(B_TRANSLATE("Bigger"),
		new BMessage(MSG_BIGGER), '+'));
	fFormatMenu->AddItem(new BMenuItem(B_TRANSLATE("Smaller"),
		new BMessage(MSG_SMALLER), '-'));
	bar->AddItem(fFormatMenu);

	return bar;
}


void
MainWindow::_OpenFile(const entry_ref& ref)
{
	// Already open? Just switch to it.
	for (int32 i = 0; i < fTabView->CountTabs(); i++) {
		EditorView* editor = dynamic_cast<EditorView*>(fTabView->TabAt(i)->View());
		if (editor != NULL && editor->Ref() == ref) {
			fTabView->Select(i);
			return;
		}
	}

	EditorView* editor = new EditorView(ref);
	editor->ApplyTheme(ThemeAt(fThemeIndex));	// before Load: sets text color
	status_t status = editor->Load();
	if (status != B_OK) {
		delete editor;
		BAlert* alert = new BAlert(B_TRANSLATE("Error"),
			B_TRANSLATE("Could not open the file."), B_TRANSLATE("OK"),
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return;
	}

	editor->TextView()->SetNotifyTarget(BMessenger(this));
	fTabView->AddTab(editor);
	fTabView->Select(fTabView->CountTabs() - 1);
}


void
MainWindow::_SaveCurrent()
{
	EditorView* editor = _CurrentEditor();
	if (editor == NULL)
		return;

	if (editor->Save() != B_OK) {
		BAlert* alert = new BAlert(B_TRANSLATE("Error"),
			B_TRANSLATE("Could not save the file."), B_TRANSLATE("OK"),
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
	}
}


void
MainWindow::_CloseCurrentTab()
{
	int32 selection = fTabView->Selection();
	if (selection < 0)
		return;

	EditorView* editor = _CurrentEditor();
	if (editor != NULL && !_ConfirmClose(editor))
		return;

	// The caller owns the removed tab; deleting it also deletes its view.
	BTab* tab = fTabView->RemoveTab(selection);
	delete tab;
}


EditorView*
MainWindow::_CurrentEditor() const
{
	int32 selection = fTabView->Selection();
	if (selection < 0)
		return NULL;
	return dynamic_cast<EditorView*>(fTabView->TabAt(selection)->View());
}


// Asks what to do with unsaved changes. Returns false if closing should be
// aborted (user cancelled, or saving failed).
bool
MainWindow::_ConfirmClose(EditorView* editor)
{
	if (!editor->IsModified())
		return true;

	BString text;
	text.SetToFormat(B_TRANSLATE("Save changes to \"%s\" before closing?"),
		editor->Ref().name);

	BAlert* alert = new BAlert(B_TRANSLATE("Unsaved changes"), text.String(),
		B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);

	switch (alert->Go()) {
		case 0:
			return false;
		case 2:
			if (editor->Save() != B_OK) {
				BAlert* error = new BAlert(B_TRANSLATE("Error"),
					B_TRANSLATE("Could not save the file."),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL,
					B_STOP_ALERT);
				error->Go();
				return false;
			}
			return true;
		default:
			return true;
	}
}


// Show a bullet in front of the names of tabs with unsaved changes.
void
MainWindow::_UpdateTabLabels()
{
	for (int32 i = 0; i < fTabView->CountTabs(); i++) {
		BTab* tab = fTabView->TabAt(i);
		EditorView* editor = dynamic_cast<EditorView*>(tab->View());
		if (editor == NULL)
			continue;

		BString label;
		if (editor->IsModified())
			label << B_UTF8_BULLET << " ";
		label << editor->Ref().name;
		tab->SetLabel(label.String());
	}
	fTabView->Invalidate();
}


void
MainWindow::_SetTheme(int index)
{
	if (index < 0 || index >= ThemeCount())
		return;

	fThemeIndex = index;
	Theme theme = ThemeAt(index);
	for (int32 i = 0; i < fTabView->CountTabs(); i++) {
		EditorView* editor
			= dynamic_cast<EditorView*>(fTabView->TabAt(i)->View());
		if (editor != NULL)
			editor->ApplyTheme(theme);
	}
	Settings::SaveTheme(theme.id);
}


static BString
DecodePercent(const BString& in)
{
	BString out;
	for (int32 i = 0; i < in.Length(); i++) {
		if (in[i] == '%' && i + 2 < in.Length() && isxdigit(in[i + 1])
			&& isxdigit(in[i + 2])) {
			char hex[3] = {in[i + 1], in[i + 2], 0};
			out << (char)strtol(hex, NULL, 16);
			i += 2;
		} else
			out << in[i];
	}
	return out;
}


static void
ShowError(const char* text)
{
	BAlert* alert = new BAlert(B_TRANSLATE("Error"), text, B_TRANSLATE("OK"),
		NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}


// Web and mail links go to the preferred application; anything else is a
// file path relative to the current document, opened in a new tab.
void
MainWindow::_OpenLink(const char* url)
{
	BString link(url);

	int32 colon = link.FindFirst(':');
	bool hasScheme = colon > 1;
	for (int32 i = 0; hasScheme && i < colon; i++) {
		if (!isalnum((unsigned char)link[i]) && link[i] != '+'
			&& link[i] != '.' && link[i] != '-') {
			hasScheme = false;
		}
	}

	if (hasScheme) {
		BString scheme(link.String(), colon);
		scheme.ToLower();
		BString mimeType("application/x-vnd.Be.URL.");
		mimeType << scheme;

		const char* argv[1] = {link.String()};
		if (be_roster->Launch(mimeType.String(), 1, argv) != B_OK) {
			ShowError(B_TRANSLATE("No application is set up to open this "
				"kind of link."));
		}
		return;
	}

	// Strip "#fragment" and "?query"; in-page anchors have nothing to open.
	int32 cut = link.FindFirst('#');
	if (cut >= 0)
		link.Truncate(cut);
	cut = link.FindFirst('?');
	if (cut >= 0)
		link.Truncate(cut);
	if (link.Length() == 0)
		return;
	link = DecodePercent(link);

	BPath target;
	if (link[0] == '/') {
		target.SetTo(link.String());
	} else {
		EditorView* editor = _CurrentEditor();
		if (editor == NULL)
			return;
		BPath document(&editor->Ref());
		BPath folder;
		if (document.GetParent(&folder) != B_OK)
			return;
		target.SetTo(folder.Path(), link.String(), true);
	}

	BEntry entry(target.Path(), true);
	entry_ref ref;
	if (entry.InitCheck() == B_OK && entry.IsFile()
		&& entry.GetRef(&ref) == B_OK) {
		_OpenFile(ref);
	} else
		ShowError(B_TRANSLATE("The linked file could not be found."));
}
