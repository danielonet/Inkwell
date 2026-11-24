/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef SIDEBAR_VIEW_H
#define SIDEBAR_VIEW_H

#include <Entry.h>
#include <ListItem.h>
#include <OutlineListView.h>


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


// Folder tree shown on the left of the window.
// TODO: lazy loading + node monitoring so the tree follows disk changes.
class SidebarView : public BOutlineListView {
public:
								SidebarView(const char* name);
	virtual						~SidebarView();

			void				SetFolder(const entry_ref& folder);
			FileItem*			SelectedItem() const;

private:
			void				_Clear();
			void				_AddDirectory(const entry_ref& dir,
									FileItem* parent, int depth);
};

#endif	// SIDEBAR_VIEW_H
