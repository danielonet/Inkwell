/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "SidebarView.h"

#include <Directory.h>
#include <Message.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <Path.h>

#include <algorithm>
#include <strings.h>
#include <vector>


static const bigtime_t kRefreshDelay = 300000;	// 0.3 s debounce

enum {
	MSG_REFRESH	= 'rfsh'
};


FileItem::FileItem(const entry_ref& ref, bool isDirectory, bool expanded)
	:
	BStringItem(ref.name, 0, expanded),
	fRef(ref),
	fIsDirectory(isDirectory),
	fLoaded(false)
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


std::string
PathOf(const entry_ref& ref)
{
	BPath path(&ref);
	return path.InitCheck() == B_OK ? std::string(path.Path()) : std::string();
}

}	// namespace


SidebarView::SidebarView(const char* name)
	:
	BOutlineListView(name, B_SINGLE_SELECTION_LIST),
	fRefreshRunner(NULL)
{
}


SidebarView::~SidebarView()
{
	stop_watching(this);
	delete fRefreshRunner;
	_Clear();
}


void
SidebarView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			_ScheduleRefresh();
			break;

		case MSG_REFRESH:
			delete fRefreshRunner;
			fRefreshRunner = NULL;
			_Refresh();
			break;

		default:
			BOutlineListView::MessageReceived(message);
			break;
	}
}


// Expanding happens inside the base class; look for newly expanded, not yet
// loaded folders afterwards.
void
SidebarView::MouseDown(BPoint where)
{
	BOutlineListView::MouseDown(where);
	_LoadExpanded();
}


void
SidebarView::KeyDown(const char* bytes, int32 numBytes)
{
	BOutlineListView::KeyDown(bytes, numBytes);
	_LoadExpanded();
}


void
SidebarView::SetFolder(const entry_ref& folder)
{
	fRoot = folder;
	fExpandedPaths.clear();
	_Rebuild();
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
SidebarView::_Rebuild()
{
	stop_watching(this);
	_Clear();

	if (fRoot.device < 0)
		return;

	FileItem* root = new FileItem(fRoot, true, true);
	AddItem(root);
	_LoadChildren(root);
}


// Rebuild after a disk change, remembering what the user had open.
void
SidebarView::_Refresh()
{
	std::string selected;
	if (FileItem* item = SelectedItem())
		selected = PathOf(item->Ref());

	fExpandedPaths.clear();
	for (int32 i = 0; i < FullListCountItems(); i++) {
		FileItem* item = dynamic_cast<FileItem*>(FullListItemAt(i));
		if (item != NULL && item->IsDirectory() && item->IsExpanded())
			fExpandedPaths.insert(PathOf(item->Ref()));
	}

	_Rebuild();

	if (selected.empty())
		return;
	for (int32 i = 0; i < FullListCountItems(); i++) {
		FileItem* item = dynamic_cast<FileItem*>(FullListItemAt(i));
		if (item != NULL && PathOf(item->Ref()) == selected) {
			int32 visibleIndex = IndexOf(item);
			if (visibleIndex >= 0)
				Select(visibleIndex);
			break;
		}
	}
}


// A single operation (e.g. a copy) fires many events; wait until they stop.
void
SidebarView::_ScheduleRefresh()
{
	if (fRefreshRunner != NULL)
		return;

	BMessage refresh(MSG_REFRESH);
	fRefreshRunner = new BMessageRunner(BMessenger(this), &refresh,
		kRefreshDelay, 1);
	if (fRefreshRunner->InitCheck() != B_OK) {
		delete fRefreshRunner;
		fRefreshRunner = NULL;
	}
}


void
SidebarView::_Clear()
{
	// RemoveItem() on a parent drops its subitems from the list without
	// deleting them, so collect every item first and delete them all.
	std::vector<BListItem*> items;
	for (int32 i = 0; i < FullListCountItems(); i++)
		items.push_back(FullListItemAt(i));

	MakeEmpty();
	for (size_t i = 0; i < items.size(); i++)
		delete items[i];
}


// Reads a folder's contents from disk and starts watching it.
void
SidebarView::_LoadChildren(FileItem* directoryItem)
{
	if (directoryItem->IsLoaded())
		return;
	directoryItem->SetLoaded(true);

	// Drop the placeholder that made the folder expandable.
	while (BListItem* placeholder = ItemUnderAt(directoryItem, true, 0)) {
		RemoveItem(placeholder);
		delete placeholder;
	}

	_AddDirectory(directoryItem);
}


// Load folders the user has just expanded.
void
SidebarView::_LoadExpanded()
{
	for (int32 i = 0; i < FullListCountItems(); i++) {
		FileItem* item = dynamic_cast<FileItem*>(FullListItemAt(i));
		if (item != NULL && item->IsDirectory() && item->IsExpanded()
			&& !item->IsLoaded()) {
			_LoadChildren(item);
		}
	}
}


void
SidebarView::_AddDirectory(FileItem* parent)
{
	BDirectory directory(&parent->Ref());
	if (directory.InitCheck() != B_OK)
		return;

	// Failing to watch (e.g. watch limit reached) only costs live updates.
	node_ref nodeRef;
	if (directory.GetNodeRef(&nodeRef) == B_OK)
		watch_node(&nodeRef, B_WATCH_DIRECTORY, this);

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
		bool expanded = entries[i].isDirectory
			&& fExpandedPaths.count(PathOf(entries[i].ref)) > 0;
		FileItem* item = new FileItem(entries[i].ref, entries[i].isDirectory,
			expanded);
		AddUnder(item, parent);

		if (!entries[i].isDirectory)
			continue;
		if (expanded)
			_LoadChildren(item);
		else
			AddUnder(new BStringItem(B_UTF8_ELLIPSIS), item);
	}
}
