/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef EDITOR_VIEW_H
#define EDITOR_VIEW_H

#include <Entry.h>
#include <ScrollView.h>
#include <TextView.h>


// One open file: a scrollable text view bound to an entry_ref.
class EditorView : public BScrollView {
public:
								EditorView(const entry_ref& ref);

			const entry_ref&	Ref() const { return fRef; }
			BTextView*			TextView() const;

			status_t			Load();
			status_t			Save();

private:
			entry_ref			fRef;
};

#endif	// EDITOR_VIEW_H
