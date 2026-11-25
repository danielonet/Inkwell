/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <String.h>


// Persistent preferences in ~/config/settings/Inkwell_settings.
namespace Settings {

BString	LoadTheme();	// empty if never saved
void	SaveTheme(const char* themeId);

}	// namespace Settings

#endif	// SETTINGS_H
