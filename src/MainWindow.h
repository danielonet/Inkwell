/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

class BFilePanel;
class BMenu;
class BMenuBar;
class BMenuItem;
class BTabView;
class EditorView;
class SidebarView;


class MainWindow : public BWindow {
public:
								MainWindow();
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual	void				MenusBeginning();

private:
			BMenuBar*			_BuildMenuBar();
			void				_OpenFile(const entry_ref& ref);
			void				_SaveCurrent();
			void				_CloseCurrentTab();
			EditorView*			_CurrentEditor() const;
			bool				_ConfirmClose(EditorView* editor);
			void				_OpenLink(const char* url);
			void				_SetTheme(int index);
			void				_RefreshSpelling();
			void				_UpdateTabLabels();

			BMenuBar*			fMenuBar;
			SidebarView*		fSidebar;
			BTabView*			fTabView;
			BMenuItem*			fPreviewItem;
			BMenu*				fThemeMenu;
			BMenu*				fFormatMenu;
			BMenuItem*			fSpellItem;
			BMenu*				fLanguageMenu;
			int					fThemeIndex;
			BFilePanel*			fFolderPanel;
};

#endif	// MAIN_WINDOW_H
