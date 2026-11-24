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

			// Children are read from disk only when first expanded.
			bool				IsLoaded() const { return fLoaded; }
			void				SetLoaded(bool loaded) { fLoaded = loaded; }

private:
			entry_ref			fRef;
			bool				fIsDirectory;
			bool				fLoaded;
};


// Folder tree shown on the left of the window. Folders are read lazily when
// first expanded (until then they hold a placeholder child so they show an
// expander). Every loaded folder is watched with the node monitor; changes
// rebuild the tree (debounced) while keeping expanded folders and selection.
class SidebarView : public BOutlineListView {
public:
								SidebarView(const char* name);
	virtual						~SidebarView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

			void				SetFolder(const entry_ref& folder);
			FileItem*			SelectedItem() const;

private:
			void				_Rebuild();
			void				_Refresh();
			void				_ScheduleRefresh();
			void				_Clear();
			void				_LoadChildren(FileItem* directory);
			void				_LoadExpanded();
			void				_AddDirectory(FileItem* parent);

			entry_ref			fRoot;
			std::set<std::string> fExpandedPaths;
			BMessageRunner*		fRefreshRunner;
};

#endif	// SIDEBAR_VIEW_H
