/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "SidebarView.h"

#include <Directory.h>

#include <algorithm>
#include <strings.h>
#include <vector>


static const int kMaxDepth = 8;


FileItem::FileItem(const entry_ref& ref, bool isDirectory, bool expanded)
	:
	BStringItem(ref.name, 0, expanded),
	fRef(ref),
	fIsDirectory(isDirectory)
{
}


namespace {

struct Entry {
	entry_ref	ref;
	bool		isDirectory;
};


// Folders first, then case-insensitive by name.
bool
CompareEntries(const Entry& a, const Entry& b)
{
	if (a.isDirectory != b.isDirectory)
		return a.isDirectory;
	return strcasecmp(a.ref.name, b.ref.name) < 0;
}

}	// namespace


SidebarView::SidebarView(const char* name)
	:
	BOutlineListView(name, B_SINGLE_SELECTION_LIST)
{
}


SidebarView::~SidebarView()
{
	_Clear();
}


void
SidebarView::SetFolder(const entry_ref& folder)
{
	_Clear();

	FileItem* root = new FileItem(folder, true, true);
	AddItem(root);
	_AddDirectory(folder, root, 0);
}


FileItem*
SidebarView::SelectedItem() const
{
	int32 index = CurrentSelection();
	if (index < 0)
		return NULL;
	return dynamic_cast<FileItem*>(ItemAt(index));
}


void
SidebarView::_Clear()
{
	// BOutlineListView::MakeEmpty() does not delete the items.
	while (BListItem* item = FullListItemAt(0)) {
		RemoveItem(item);
		delete item;
	}
}


void
SidebarView::_AddDirectory(const entry_ref& dirRef, FileItem* parent,
	int depth)
{
	BDirectory directory(&dirRef);
	if (directory.InitCheck() != B_OK)
		return;

	std::vector<Entry> entries;
	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		if (ref.name[0] == '.')
			continue;

		// Don't traverse symlinks: avoids loops in the tree.
		BEntry entry(&ref, false);
		Entry e;
		e.ref = ref;
		e.isDirectory = entry.IsDirectory();
		entries.push_back(e);
	}
	std::sort(entries.begin(), entries.end(), CompareEntries);

	for (size_t i = 0; i < entries.size(); i++) {
		FileItem* item = new FileItem(entries[i].ref, entries[i].isDirectory);
		AddUnder(item, parent);
		if (entries[i].isDirectory && depth < kMaxDepth)
			_AddDirectory(entries[i].ref, item, depth + 1);
	}
}
