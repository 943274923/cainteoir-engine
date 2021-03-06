/* MBROLA pho file.
 *
 * Copyright (C) 2014 Reece H. Dunn
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
#include "i18n.h"
#include "compatibility.hpp"

#include <cainteoir/synthesizer.hpp>

namespace tts = cainteoir::tts;
namespace ipa = cainteoir::ipa;
namespace css = cainteoir::css;

bool
tts::write_diphone(const tts::prosody &aProsody,
                   const std::shared_ptr<tts::phoneme_writer> &aPhonemeSet,
                   FILE *aOutput)
{
	if (!aOutput) return false;

	constexpr auto mask = ~(ipa::stress | ipa::tone_start | ipa::tone_middle | ipa::tone_end);

	if (!aPhonemeSet->write(aProsody.first.phoneme1.get(mask)))
		return false;

	if (aProsody.first.phoneme2 != ipa::unspecified)
	{
		if (!aPhonemeSet->write(aProsody.first.phoneme2.get(mask)))
			return false;
	}

	aPhonemeSet->flush();

	if (aProsody.second.phoneme1 != ipa::unspecified)
	{
		fputc('-', aOutput);
		if (!aPhonemeSet->write(aProsody.second.phoneme1.get(mask)))
			return false;

		if (aProsody.second.phoneme2 != ipa::unspecified)
		{
			if (!aPhonemeSet->write(aProsody.second.phoneme2.get(mask)))
				return false;
		}

		aPhonemeSet->flush();
	}
}

static float parse_number(const char * &current, const char *end)
{
	int value = 0; // the value of the number
	int divisor = 1; // the number to divide by to convert value to a fraction
	bool is_fraction = false;
	while (current != end) switch (*current)
	{
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		value *= 10;
		value += (*current - '0');
		if (is_fraction)
			divisor *= 10;
		++current;
		break;
	case '.':
		if (is_fraction)
			return float(value) / divisor;
		is_fraction = true;
		++current;
		break;
	default:
		return float(value) / divisor;
	}
	return float(value) / divisor;
}

struct pho_reader : public tts::prosody_reader
{
	pho_reader(const std::shared_ptr<tts::phoneme_parser> &aPhonemeSet,
	           const std::shared_ptr<cainteoir::buffer> &aBuffer);

	bool read();
private:
	bool skip_whitespace();

	std::shared_ptr<tts::phoneme_parser> mPhonemeSet;

	std::shared_ptr<cainteoir::buffer> mBuffer;
	const char *mCurrent;
	const char *mEnd;
};

pho_reader::pho_reader(const std::shared_ptr<tts::phoneme_parser> &aPhonemeSet,
                       const std::shared_ptr<cainteoir::buffer> &aBuffer)
	: mPhonemeSet(aPhonemeSet)
	, mBuffer(aBuffer)
	, mCurrent(aBuffer->begin())
	, mEnd(aBuffer->end())
{
}

bool pho_reader::read()
{
	while (mCurrent != mEnd) switch (*mCurrent)
	{
	case ';':
		while (mCurrent != mEnd && *mCurrent != '\n') ++mCurrent;
		break;
	case '\r': case '\n':
		++mCurrent;
		break;
	default:
		if (!mPhonemeSet->parse(mCurrent, mEnd, first.phoneme1))
			return false;
		switch (*mCurrent)
		{
		case ' ': case '\t':
			if (!mPhonemeSet->parse(mEnd, mEnd, first.phoneme2))
				first.phoneme2 = ipa::unspecified;
			second.phoneme1 = ipa::unspecified;
			second.phoneme2 = ipa::unspecified;
			break;
		case '-': // diphone
			++mCurrent;
			if (!mPhonemeSet->parse(mEnd, mEnd, first.phoneme2))
				first.phoneme2 = ipa::unspecified;
			if (!mPhonemeSet->parse(mCurrent, mEnd, second.phoneme1))
				return false;
			switch (*mCurrent)
			{
			case ' ': case '\t':
				if (!mPhonemeSet->parse(mEnd, mEnd, second.phoneme2))
					second.phoneme2 = ipa::unspecified;
				break;
			default: // diphthong or affricate
				if (!mPhonemeSet->parse(mCurrent, mEnd, second.phoneme2))
					second.phoneme2 = ipa::unspecified;
				break;
			}
			break;
		default: // diphthong or affricate
			if (!mPhonemeSet->parse(mCurrent, mEnd, first.phoneme2))
				first.phoneme2 = ipa::unspecified;
			second.phoneme1 = ipa::unspecified;
			second.phoneme2 = ipa::unspecified;
			break;
		}

		if (!skip_whitespace())
		{
			first.duration = {};
			return true;
		}

		first.duration = css::time(parse_number(mCurrent, mEnd), css::time::milliseconds);
		if (*mCurrent == '-')
		{
			++mCurrent;
			second.duration = css::time(parse_number(mCurrent, mEnd), css::time::milliseconds);
		}
		else
			second.duration = {};

		envelope.clear();
		while (skip_whitespace())
		{
			float offset = parse_number(mCurrent, mEnd);

			if (!skip_whitespace())
				throw tts::phoneme_error("expected whitespace after the offset");

			float pitch = parse_number(mCurrent, mEnd);

			envelope.push_back({ int(offset), { pitch, css::frequency::hertz }});
		}

		return true;
	}
	return false;
}

bool pho_reader::skip_whitespace()
{
	if (*mCurrent == ' ' || *mCurrent == '\t')
	{
		++mCurrent;
		while (*mCurrent == ' ' || *mCurrent == '\t')
			++mCurrent;

		if (mCurrent == mEnd || *mCurrent == '\r' || *mCurrent == '\n' || *mCurrent == ';')
			return false;

		return true;
	}
	return false;
}

std::shared_ptr<tts::prosody_reader>
tts::createPhoReader(const std::shared_ptr<phoneme_parser> &aPhonemeSet,
                     const std::shared_ptr<cainteoir::buffer> &aBuffer)
{
	if (!aBuffer.get()) return {};
	return std::make_shared<pho_reader>(aPhonemeSet, aBuffer);
}

struct pho_writer : public tts::prosody_writer
{
	pho_writer(const std::shared_ptr<tts::phoneme_writer> &aPhonemeSet);

	void reset(FILE *aOutput);

	bool write(const tts::prosody &aProsody);
private:
	std::shared_ptr<tts::phoneme_writer> mPhonemeSet;
	FILE *mOutput;
};

pho_writer::pho_writer(const std::shared_ptr<tts::phoneme_writer> &aPhonemeSet)
	: mPhonemeSet(aPhonemeSet)
	, mOutput(nullptr)
{
}

void pho_writer::reset(FILE *aOutput)
{
	mPhonemeSet->reset(aOutput);
	mOutput = aOutput;
}

bool pho_writer::write(const tts::prosody &aProsody)
{
	fflush(mOutput);
	if (!tts::write_diphone(aProsody, mPhonemeSet, mOutput))
		return false;

	if (aProsody.first.duration.units() != css::time::inherit)
	{
		fprintf(mOutput, " %G", aProsody.first.duration.as(css::time::milliseconds).value());
		if (aProsody.second.duration.units() != css::time::inherit)
			fprintf(mOutput, "-%G", aProsody.second.duration.as(css::time::milliseconds).value());
	}

	for (auto &entry : aProsody.envelope)
		fprintf(mOutput, " %d %G", entry.offset, entry.pitch.as(css::frequency::hertz).value());

	fprintf(mOutput, "\n");
	return true;
}

std::shared_ptr<tts::prosody_writer>
tts::createPhoWriter(const std::shared_ptr<phoneme_writer> &aPhonemeSet)
{
	return std::make_shared<pho_writer>(aPhonemeSet);
}
