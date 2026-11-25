/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */

#include "SpellService.h"

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>

#include <string.h>
#include <strings.h>

#include <string>

#include "Settings.h"


static const char* kEnabledKey = "spell_enabled";
static const char* kLanguageKey = "spell_language";
static const char* kDefaultLanguage = "en_US";
static const char* kPersonalFile = "Inkwell_dictionary";


static bool
ReadFile(const char* path, std::string* out)
{
	BFile file(path, B_READ_ONLY);
	off_t size;
	if (file.InitCheck() != B_OK || file.GetSize(&size) != B_OK)
		return false;

	out->assign((size_t)size, '\0');
	if (size > 0) {
		ssize_t read = file.Read(&(*out)[0], (size_t)size);
		if (read < 0)
			return false;
		out->resize((size_t)read);
	}
	return true;
}


SpellService&
SpellService::Instance()
{
	static SpellService service;
	return service;
}


SpellService::SpellService()
	:
	fEnabled(Settings::GetBool(kEnabledKey, true)),
	fLoaded(false)
{
	_FindLanguages();

	// Saved choice, else English, else whatever is installed.
	BString wanted = Settings::GetString(kLanguageKey);
	if (wanted.Length() == 0)
		wanted = kDefaultLanguage;

	for (size_t i = 0; i < fLanguages.size(); i++) {
		if (fLanguages[i].id == wanted)
			fCurrent = wanted;
	}
	if (fCurrent.Length() == 0 && !fLanguages.empty())
		fCurrent = fLanguages[0].id;
}


void
SpellService::_FindLanguages()
{
	BPath folders[4];
	int count = 0;

	BPath path;
	if (find_directory(B_USER_NONPACKAGED_DATA_DIRECTORY, &path) == B_OK
		&& path.Append("Inkwell/dictionaries") == B_OK) {
		folders[count++] = path;
	}
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) == B_OK
		&& path.Append("Inkwell/dictionaries") == B_OK) {
		folders[count++] = path;
	}

	app_info info;
	if (be_app != NULL && be_app->GetAppInfo(&info) == B_OK) {
		BPath app(&info.ref);
		BPath appFolder;
		if (app.GetParent(&appFolder) == B_OK) {
			BPath dev(appFolder.Path(), "dictionaries");
			folders[count++] = dev;
			BPath parent;
			if (appFolder.GetParent(&parent) == B_OK) {
				BPath dev2(parent.Path(), "dictionaries");
				folders[count++] = dev2;
			}
		}
	}

	for (int f = 0; f < count; f++) {
		BDirectory directory(folders[f].Path());
		if (directory.InitCheck() != B_OK)
			continue;

		entry_ref ref;
		while (directory.GetNextRef(&ref) == B_OK) {
			BString name(ref.name);
			if (!name.IEndsWith(".aff"))
				continue;
			name.Truncate(name.Length() - 4);

			// Already found in a folder with higher priority?
			bool known = false;
			for (size_t i = 0; i < fLanguages.size(); i++) {
				if (fLanguages[i].id == name)
					known = true;
			}
			if (known)
				continue;

			BPath aff(folders[f].Path(), ref.name);
			BString dicName(name);
			dicName << ".dic";
			BPath dic(folders[f].Path(), dicName.String());
			if (!BEntry(dic.Path()).Exists())
				continue;

			Language language;
			language.id = name;
			language.affixPath = aff.Path();
			language.wordListPath = dic.Path();
			fLanguages.push_back(language);
		}
	}
}


bool
SpellService::SetLanguage(const char* id)
{
	for (size_t i = 0; i < fLanguages.size(); i++) {
		if (fLanguages[i].id == id) {
			fCurrent = id;
			fLoaded = false;
			Settings::SetString(kLanguageKey, id);
			return true;
		}
	}
	return false;
}


void
SpellService::SetEnabled(bool enabled)
{
	fEnabled = enabled;
	Settings::SetBool(kEnabledKey, enabled);
}


const spell::Dictionary*
SpellService::Dictionary() const
{
	if (!fLoaded) {
		fLoaded = true;
		fDictionary = spell::Dictionary();

		for (size_t i = 0; i < fLanguages.size(); i++) {
			if (fLanguages[i].id != fCurrent)
				continue;
			std::string affix;
			std::string words;
			if (ReadFile(fLanguages[i].affixPath.String(), &affix)
				&& ReadFile(fLanguages[i].wordListPath.String(), &words)) {
				fDictionary.Load(affix, words);
			}
		}
		const_cast<SpellService*>(this)->_LoadPersonalWords();
	}
	return fDictionary.IsLoaded() ? &fDictionary : NULL;
}


void
SpellService::_LoadPersonalWords()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kPersonalFile) != B_OK) {
		return;
	}

	std::string text;
	if (!ReadFile(path.Path(), &text))
		return;

	size_t start = 0;
	while (start < text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.size();
		std::string word = text.substr(start, end - start);
		if (!word.empty() && word[word.size() - 1] == '\r')
			word.erase(word.size() - 1);
		if (!word.empty())
			fDictionary.AddWord(word);
		start = end + 1;
	}
}


void
SpellService::AddPersonalWord(const char* word)
{
	if (word == NULL || word[0] == '\0')
		return;

	Dictionary();	// make sure it is loaded first
	fDictionary.AddWord(word);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK
		|| path.Append(kPersonalFile) != B_OK) {
		return;
	}
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_OPEN_AT_END);
	if (file.InitCheck() == B_OK) {
		BString line(word);
		line << "\n";
		file.Write(line.String(), (size_t)line.Length());
	}
}


bool
SpellService::IsProseFile(const char* fileName)
{
	static const char* const kExtensions[] = {
		".txt", ".text", ".md", ".markdown", ".mdown", ".mkd", ".rtf", ".rst",
		NULL
	};
	const char* dot = strrchr(fileName, '.');
	if (dot == NULL)
		return false;
	for (int i = 0; kExtensions[i] != NULL; i++) {
		if (strcasecmp(dot, kExtensions[i]) == 0)
			return true;
	}
	return false;
}
