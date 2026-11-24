/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <TabView.h>

#include "EditorView.h"
#include "SidebarView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_OPEN_FOLDER		= 'opfd',
	MSG_FOLDER_CHOSEN	= 'fdch',
	MSG_OPEN_ITEM		= 'opit',
	MSG_SAVE			= 'save',
	MSG_CLOSE_TAB		= 'clst'
};


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 1000, 700), B_TRANSLATE_SYSTEM_NAME("Inkwell"),
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fMenuBar(NULL),
	fSidebar(NULL),
	fTabView(NULL),
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

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	// TODO: ask to save modified tabs before quitting.
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
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
	status_t status = editor->Load();
	if (status != B_OK) {
		delete editor;
		BAlert* alert = new BAlert(B_TRANSLATE("Error"),
			B_TRANSLATE("Could not open the file."), B_TRANSLATE("OK"),
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return;
	}

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

	// TODO: ask to save if the text was modified.
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
