/*
 * Copyright 2026 Daniel Dreichlinger
 * Distributed under the terms of the MIT License.
 */
#ifndef SPELL_H
#define SPELL_H

// Platform independent spell checking with Hunspell-format dictionaries
// (a .aff affix file plus a .dic word list). No Haiku headers, see tests/.
//
// Supported from the format: SET UTF-8, ICONV, ONLYINCOMPOUND, NOSUGGEST and
// PFX/SFX rules with single-character flags (conditions are matched on bytes,
// which is fine for the Latin-script dictionaries this is meant for).
// Not supported: compounding, two-level affixes, REP/MAP/PHONE suggestions.

#include <string>
#include <unordered_map>
#include <vector>


namespace spell {

struct Range {
	size_t	begin;	// byte offsets into the checked text, end is exclusive
	size_t	end;
};


class Dictionary {
public:
								Dictionary();

			bool				Load(const std::string& affixText,
									const std::string& wordListText);
			bool				IsLoaded() const { return !fWords.empty(); }

	// `word` is one UTF-8 word without punctuation (apostrophes allowed).
			bool				IsCorrect(const std::string& word) const;

	// Words one edit away from `word` that are correct, best guesses first.
			std::vector<std::string> Suggest(const std::string& word,
									size_t maxCount = 6) const;

	// Personal words: accepted like dictionary words (lowercase entries match
	// any capitalization, capitalized ones only when capitalized).
			void				AddWord(const std::string& word);

private:
	struct Condition {
		bool		any;
		bool		negate;
		std::string	chars;
	};

	struct Affix {
		char					flag;
		bool					cross;
		std::string				strip;
		std::string				add;
		std::vector<Condition>	condition;
	};

			bool				_Lookup(const std::string& word,
									bool forSuggestion) const;
			bool				_HasEntry(const std::string& stem,
									char flag1, char flag2,
									bool forSuggestion) const;
			bool				_CheckSuffix(const std::string& word,
									char prefixFlag, const Affix* prefix,
									bool forSuggestion) const;
			bool				_MatchesEnd(const Affix& rule,
									const std::string& stem) const;
			bool				_MatchesStart(const Affix& rule,
									const std::string& stem) const;
			std::string			_Convert(const std::string& word) const;

			std::unordered_map<std::string, std::string> fWords;	// -> flags
			std::vector<Affix>	fPrefixes;
			std::vector<Affix>	fSuffixes;
			std::vector<std::pair<std::string, std::string> > fIconv;
			std::string			fTry;
			char				fOnlyInCompound;
			char				fNoSuggest;
};


// Finds misspelled words in `text`. Skips what is not prose: words with
// digits or non-Latin letters, ALL CAPS and MixedCase words, URLs, e-mail
// addresses and paths. With `markdown`, code and link targets are skipped too.
std::vector<Range> FindMisspellings(const Dictionary& dictionary,
	const std::string& text, bool markdown);

// Replaces fenced/inline code and link targets with spaces (same length).
std::string MaskMarkdownCode(const std::string& text);

}	// namespace spell

#endif	// SPELL_H
