/* Phoneme Stream.
 *
 * Copyright (C) 2013 Reece H. Dunn
 *
 * This file is part of cainteoir-engine.
 *
 * cainteoir-engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cainteoir-engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cainteoir-engine.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "compatibility.hpp"

#include <cainteoir/text.hpp>

namespace tts = cainteoir::tts;

tts::phoneme_stream::phoneme_stream(const std::shared_ptr<document_reader> &aReader,
                                    const language::tag &aLocale,
                                    tts::word_stream::number_scale aScale,
                                    const std::shared_ptr<phoneme_reader> &aRules,
                                    const std::shared_ptr<dictionary_reader> &aExceptionDictionary)
	: mReader(aReader, aLocale, aScale)
	, mRules(aRules)
{
	if (aExceptionDictionary) while (aExceptionDictionary->read())
		mExceptionDictionary.add_entry(aExceptionDictionary->word, aExceptionDictionary->entry);
}

bool tts::phoneme_stream::read()
{
	while (mReader.read()) switch (mReader.event().type)
	{
	case tts::word_uppercase:
	case tts::word_lowercase:
	case tts::word_capitalized:
	case tts::word_mixedcase:
	case tts::word_script:
		if (pronounce(mReader.event().text, mReader.event().range))
			return true;
		break;
	default:
		mEvent = mReader.event();
		return true;
	}
	return false;
}

bool tts::phoneme_stream::pronounce(const std::shared_ptr<buffer> &aText, const range<uint32_t> &aRange)
{
	mEvent = { tts::phonemes, aRange, 0 };

	auto phonemes = mExceptionDictionary.pronounce(aText);
	if (!phonemes.empty())
	{
		for (const tts::phoneme &p : phonemes)
			mEvent.phonemes.push_back(p);
		return true;
	}

	if (mRules.get())
	{
		mRules->reset(aText);
		while (mRules->read())
			mEvent.phonemes.push_back(*mRules);
		return true;
	}

	fprintf(stderr, "error: cannot pronounce '%s'\n", aText->str().c_str());
	return false;
}

void tts::generate_phonemes(tts::phoneme_stream &reader,
                            FILE *out,
                            const char *phonemeset,
                            tts::stress_type stress,
                            const char *open,
                            const char *close,
                            const char *phrase)
{
	auto ipa = tts::createPhonemeWriter(phonemeset);
	ipa->reset(out);

	bool need_open  = true;
	bool need_space = false;
	while (reader.read())
	{
		auto &event = reader.event();
		switch (event.type)
		{
		default:
			if (!need_open)
			{
				fprintf(out, "%s", close ? close : "");
				need_open  = true;
				need_space = false;
			}
			if (event.type == tts::paragraph)
				fprintf(out, "\n\n");
			else
				fprintf(out, "%s%s", event.text->str().c_str(), phrase);
			break;
		case tts::phonemes:
			if (need_open)
			{
				if (open) fprintf(out, "%s", open);
				need_open  = false;
				need_space = false;
			}
			if (need_space)
				fprintf(out, " ");
			{
				auto phonemes = event.phonemes;
				tts::make_stressed(phonemes, stress);
				for (auto p : phonemes)
					ipa->write(p);
			}
			need_space = true;
			break;
		}
	}

	if (!need_open)
		fprintf(out, "%s\n", close ? close : "");
}
