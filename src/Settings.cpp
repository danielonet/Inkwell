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


static void
Save(const BMessage& settings)
{
	BPath path;
	if (SettingsPath(&path) != B_OK)
		return;
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() == B_OK)
		settings.Flatten(&file);
}


BString
Settings::GetString(const char* key)
{
	BMessage settings;
	Load(&settings);

	BString value;
	settings.FindString(key, &value);
	return value;
}


void
Settings::SetString(const char* key, const char* value)
{
	BMessage settings;
	Load(&settings);
	settings.RemoveName(key);
	settings.AddString(key, value);
	Save(settings);
}


bool
Settings::GetBool(const char* key, bool defaultValue)
{
	BMessage settings;
	Load(&settings);

	bool value;
	if (settings.FindBool(key, &value) != B_OK)
		return defaultValue;
	return value;
}


void
Settings::SetBool(const char* key, bool value)
{
	BMessage settings;
	Load(&settings);
	settings.RemoveName(key);
	settings.AddBool(key, value);
	Save(settings);
}


BString
Settings::LoadTheme()
{
	return GetString("theme");
}


void
Settings::SaveTheme(const char* themeId)
{
	SetString("theme", themeId);
}
