/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef SIDEBAR_VIEW_H
#define SIDEBAR_VIEW_H

#include <Entry.h>
#include <ListItem.h>
#include <OutlineListView.h>

#include <set>
#include <string>

class BMessageRunner;


class FileItem : public BStringItem {
public:
								FileItem(const entry_ref& ref, bool isDirectory,
									bool expanded = false);

			const entry_ref&	Ref() const { return fRef; }
			bool				IsDirectory() const { return fIsDirectory; }

private:
			entry_ref			fRef;
			bool				fIsDirectory;
};


// Folder tree shown on the left of the window. Every loaded directory is
// watched with the node monitor; changes rebuild the tree (debounced) while
// keeping expanded folders and the selection.
// TODO: lazy loading (load a folder's children when first expanded).
class SidebarView : public BOutlineListView {
public:
								SidebarView(const char* name);
	virtual						~SidebarView();

	virtual	void				MessageReceived(BMessage* message);

			void				SetFolder(const entry_ref& folder);
			FileItem*			SelectedItem() const;

private:
			void				_Rebuild();
			void				_Refresh();
			void				_ScheduleRefresh();
			void				_Clear();
			void				_AddDirectory(const entry_ref& dir,
									FileItem* parent, int depth);

			entry_ref			fRoot;
			std::set<std::string> fExpandedPaths;
			int					fDirectoryCount;
			BMessageRunner*		fRefreshRunner;
};

#endif	// SIDEBAR_VIEW_H
