/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

class BFilePanel;
class BMenuBar;
class BTabView;
class EditorView;
class SidebarView;


class MainWindow : public BWindow {
public:
								MainWindow();
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			BMenuBar*			_BuildMenuBar();
			void				_OpenFile(const entry_ref& ref);
			void				_SaveCurrent();
			void				_CloseCurrentTab();
			EditorView*			_CurrentEditor() const;

			BMenuBar*			fMenuBar;
			SidebarView*		fSidebar;
			BTabView*			fTabView;
			BFilePanel*			fFolderPanel;
};

#endif	// MAIN_WINDOW_H
