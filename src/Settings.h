/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <String.h>


// Persistent preferences in ~/config/settings/Inkwell_settings.
namespace Settings {

BString	GetString(const char* key);		// empty if never saved
void	SetString(const char* key, const char* value);
bool	GetBool(const char* key, bool defaultValue);
void	SetBool(const char* key, bool value);

BString	LoadTheme();
void	SaveTheme(const char* themeId);

}	// namespace Settings

#endif	// SETTINGS_H
