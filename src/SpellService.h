/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef SPELL_SERVICE_H
#define SPELL_SERVICE_H

#include <String.h>

#include <vector>

#include "Spell.h"


// Finds Hunspell dictionaries (<id>.aff + <id>.dic), loads the selected one
// and keeps the personal word list. Dictionaries are looked up in, first
// match wins:
//   ~/config/non-packaged/data/Inkwell/dictionaries   (add your own here)
//   /boot/system/data/Inkwell/dictionaries            (installed package)
//   <app folder>/dictionaries and <app folder>/../dictionaries (development)
class SpellService {
public:
	struct Language {
		BString	id;			// file name without extension, e.g. "en_US"
		BString	affixPath;
		BString	wordListPath;
	};

	static	SpellService&		Instance();

			const std::vector<Language>& Languages() const
									{ return fLanguages; }
			const BString&		CurrentLanguage() const { return fCurrent; }
			bool				SetLanguage(const char* id);

			bool				IsEnabled() const { return fEnabled; }
			void				SetEnabled(bool enabled);

	// NULL if no dictionary could be loaded.
			const spell::Dictionary* Dictionary() const;

			void				AddPersonalWord(const char* word);

	// Only prose is checked: .txt, .md, .rtf, ... (not source code).
	static	bool				IsProseFile(const char* fileName);

private:
								SpellService();

			void				_FindLanguages();
			void				_LoadPersonalWords();

			std::vector<Language> fLanguages;
			BString				fCurrent;
			bool				fEnabled;
			mutable spell::Dictionary fDictionary;
			mutable bool		fLoaded;
};

#endif	// SPELL_SERVICE_H
