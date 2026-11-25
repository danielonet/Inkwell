/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "Settings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>


static const char* kSettingsFile = "Inkwell_settings";


static status_t
SettingsPath(BPath* path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, path, true);
	if (status != B_OK)
		return status;
	return path->Append(kSettingsFile);
}


static void
Load(BMessage* settings)
{
	BPath path;
	if (SettingsPath(&path) != B_OK)
		return;
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() == B_OK)
		settings->Unflatten(&file);
}


BString
Settings::LoadTheme()
{
	BMessage settings;
	Load(&settings);

	BString theme;
	settings.FindString("theme", &theme);
	return theme;
}


void
Settings::SaveTheme(const char* themeId)
{
	BMessage settings;
	Load(&settings);
	settings.RemoveName("theme");
	settings.AddString("theme", themeId);

	BPath path;
	if (SettingsPath(&path) != B_OK)
		return;
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK)
		settings.Flatten(&file);
}
