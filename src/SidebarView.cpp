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


static const int kMaxDepth = 8;
// The system limits how many nodes a team may watch, so stop well short.
static const int kMaxDirectories = 1000;
static const bigtime_t kRefreshDelay = 300000;	// 0.3 s debounce

enum {
	MSG_REFRESH	= 'rfsh'
};


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
	fDirectoryCount(0),
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
	fDirectoryCount = 0;

	if (fRoot.device < 0)
		return;

	FileItem* root = new FileItem(fRoot, true, true);
	AddItem(root);
	_AddDirectory(fRoot, root, 0);
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
	if (fDirectoryCount >= kMaxDirectories)
		return;

	BDirectory directory(&dirRef);
	if (directory.InitCheck() != B_OK)
		return;
	fDirectoryCount++;

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
		if (entries[i].isDirectory && depth < kMaxDepth)
			_AddDirectory(entries[i].ref, item, depth + 1);
	}
}
