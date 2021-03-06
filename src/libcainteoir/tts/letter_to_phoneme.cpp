/* Letter-to-Phoneme Rule Processor.
 *
 * Copyright (C) 2013-2016 Reece H. Dunn
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

#include <cainteoir/language.hpp>
#include "../synthesizer/synth.hpp"

namespace tts = cainteoir::tts;
namespace ipa = cainteoir::ipa;

struct ruleset : public tts::phoneme_reader
{
	ruleset(const std::shared_ptr<cainteoir::buffer> &aData,
	        const cainteoir::language::tag &aLocale);

	void reset(const std::shared_ptr<cainteoir::buffer> &aBuffer);

	bool read();
private:
	std::shared_ptr<cainteoir::buffer> mBuffer;
	const uint8_t *mStart;
	const uint8_t *mCurrent;
	const uint8_t *mEnd;
	const char *mPhonemeCurrent;
	const char *mPhonemeEnd;

	std::shared_ptr<cainteoir::buffer> mData;
	std::shared_ptr<tts::phoneme_parser> mPhonemeSet;
	cainteoir::native_endian_buffer mRules;
	uint32_t mRuleGroups[256];
	uint32_t mClassDefs[256];
	uint8_t  mConditionalFlags[256];
	uint8_t  mBoundary;

	ipa::phoneme mPreviousPhoneme;

	std::shared_ptr<tts::rewriter> mRewriter;

	bool match_features_next(const char *phonemes, const uint8_t *&rules);
	bool match_features_prev(const uint8_t *&rules);

	bool match_phonemes_next(const char *phonemes, const uint8_t *&rules);

	bool match_classdef(uint32_t offset, const uint8_t *&current);

	bool match_classdef_back(uint32_t offset, const uint8_t *&current);

	enum elision_rules_t
	{
		match_elision_rules,
		ignore_elision_rules,
	};

	std::pair<const uint8_t *, const char *>
	next_match(const uint8_t *current, elision_rules_t elision = match_elision_rules);
};

ruleset::ruleset(const std::shared_ptr<cainteoir::buffer> &aData,
                 const cainteoir::language::tag &aLocale)
	: mData(aData)
	, mRules((const uint8_t *)aData->begin(), (const uint8_t *)aData->end())
	, mRewriter(tts::createLexicalRewriteRules(aData))
{
	memset(mRuleGroups, 0, sizeof(mRuleGroups));
	memset(mClassDefs, 0, sizeof(mClassDefs));
	memset(mConditionalFlags, 0, sizeof(mConditionalFlags));

	mRules.seek(tts::LANGDB_HEADER_ID);
	const char *locale = mRules.pstr();
	const char *phonemeset = mRules.pstr();
	mBoundary = mRules.u8();

	mPhonemeSet = tts::createPhonemeParser(phonemeset);

	while (!mRules.eof()) switch (mRules.magic())
	{
	case tts::STRING_TABLE_MAGIC:
		mRules.seek(mRules.u32());
		break;
	case tts::CONDRULE_TABLE_MAGIC:
		for (auto n : cainteoir::range<uint16_t>(0, mRules.u16()))
		{
			uint8_t c = mRules.u8();
			uint8_t type = mRules.u8();
			const char *value = mRules.pstr();
			uint8_t set = (type & tts::LANGDB_CONDRULE_SET_MASK) == tts::LANGDB_CONDRULE_SET;
			switch (type & ~tts::LANGDB_CONDRULE_SET_MASK)
			{
			case tts::LANGDB_CONDRULE_LOCALE:
				if (cainteoir::language::issubtag(aLocale, cainteoir::language::make_lang(value)))
					mConditionalFlags[c] = set;
				break;
			}
		}
		break;
	case tts::CLASSDEF_TABLE_MAGIC:
		{
			uint16_t entries = mRules.u16();
			uint8_t  id = mRules.u8();
			uint32_t offset = mRules.offset();
			mRules.seek(offset + (entries * tts::CLASSDEF_TABLE_ENTRY_SIZE));
			mClassDefs[id] = offset;
		}
		break;
	case tts::LETTER_TO_PHONEME_TABLE_MAGIC:
		{
			uint16_t entries = mRules.u16();
			uint8_t  id = mRules.u8();
			uint32_t offset = mRules.offset();
			mRules.seek(offset + (entries * tts::LETTER_TO_PHONEME_TABLE_ENTRY_SIZE));
			mRuleGroups[id] = offset;
		}
		break;
	case tts::LEXICAL_REWRITE_RULE_TABLE_MAGIC:
		{
			uint16_t entries = mRules.u16();
			uint8_t  id = mRules.u8();
			uint32_t offset = mRules.offset();
			mRules.seek(offset + (entries * tts::LEXICAL_REWRITE_RULES_TABLE_ENTRY_SIZE));
		}
		break;
	case tts::DICTIONARY_TABLE_MAGIC:
		{
			uint16_t entries = mRules.u16();
			uint32_t offset = mRules.offset();
			mRules.seek(offset + (entries * tts::DICTIONARY_TABLE_ENTRY_SIZE));
		}
		break;
	default:
		throw std::runtime_error("unsupported section in the language file");
	}
}

void ruleset::reset(const std::shared_ptr<cainteoir::buffer> &aBuffer)
{
	mBuffer = mRewriter->rewrite(aBuffer);
	if (mBuffer.get())
	{
		mStart = mCurrent = (const uint8_t *)mBuffer->begin();
		mEnd = (const uint8_t *)mBuffer->end();
	}
	else
		mStart = mCurrent = mEnd = nullptr;
	mPhonemeCurrent = mPhonemeEnd = nullptr;
	(ipa::phoneme &)*this = ipa::unspecified;
}

bool ruleset::read()
{
	mPreviousPhoneme = *this;
	while (!mPhonemeSet->parse(mPhonemeCurrent, mPhonemeEnd, *this))
	{
		auto match = next_match(mCurrent);
		if (!match.first)
			return false;

		mCurrent        = match.first;
		mPhonemeCurrent = match.second;
		mPhonemeEnd     = match.second + strlen(match.second);
		mPhonemeSet->initialize();
	}
	return true;
}

bool ruleset::match_features_next(const char *phonemes, const uint8_t *&rules)
{
	const char *phonemes_end = phonemes + strlen(phonemes);
	ipa::phoneme p;
	char feature[4] = { 0, 0, 0, 0 };
	uint8_t feature_pos = 0xFF;
	while (true) switch (*rules)
	{
	case 0:
		return true;
	case '{':
		if (!mPhonemeSet->parse(phonemes, phonemes_end, p))
			return true;
		feature_pos = 0;
		++rules;
		break;
	case '}':
	case ',':
		if (feature_pos != 3)
			return false;
		if (!p.get(feature))
			return false;
		feature_pos = (*rules == '}') ? 0xFF : 0;
		++rules;
		break;
	default:
		if (feature_pos == 0xFF)
			return !mPhonemeSet->parse(phonemes, phonemes_end, p);
		if (feature_pos >= 4)
			return false;
		feature[feature_pos++] = *rules++;
		break;
	}
}

bool ruleset::match_features_prev(const uint8_t *&rules)
{
	char feature[4] = { 0, 0, 0, 0 };
	uint8_t feature_pos = 0xFF;
	while (true) switch (*rules)
	{
	case 0:
	case '(':
		return feature_pos == 0xFF;
	case '}':
		feature_pos = 0;
		++rules;
		break;
	case '{':
	case ',':
		if (feature_pos != 3)
			return false;
		if (!mPreviousPhoneme.get(feature))
			return false;
		feature_pos = (*rules == '{') ? 0xFF : 0;
		++rules;
		break;
	default:
		if (feature_pos == 0xFF || feature_pos >= 3)
			return false;
		feature[2 - feature_pos] = *rules++;
		++feature_pos;
		break;
	}
	return false;
}

bool ruleset::match_phonemes_next(const char *phonemes, const uint8_t *&rules)
{
	constexpr auto mask = ipa::main | ipa::diacritics | ipa::length;

	auto *phonemes_end = phonemes + strlen(phonemes);

	++rules;
	auto *end = rules;
	while (*end && *end != '/')
		++end;

	ipa::phoneme p1;
	ipa::phoneme p2;
	while (mPhonemeSet->parse((const char *&)rules, (const char *)end, p1))
	{
		if (!mPhonemeSet->parse(phonemes, phonemes_end, p2))
			return false;

		if (p1.get(mask) != p2.get(mask))
			return false;
	}

	if (rules == end && phonemes == phonemes_end)
	{
		++rules;
		return true;
	}

	return false;
}

bool ruleset::match_classdef(uint32_t offset, const uint8_t *&current)
{
	uint32_t prev_offset = mRules.offset();
	mRules.seek(offset);

	while (true)
	{
		offset = mRules.u32();
		if (offset == 0)
		{
			mRules.seek(prev_offset);
			return false;
		}

		const uint8_t *match = (const uint8_t *)mRules.pstr(offset);
		const uint8_t *check = current;
		while (*match && check < mEnd && *check == *match)
		{
			++match;
			++check;
		}

		if (!*match)
		{
			current = check;
			mRules.seek(prev_offset);
			return true;
		}
	}
}

bool ruleset::match_classdef_back(uint32_t offset, const uint8_t *&current)
{
	uint32_t prev_offset = mRules.offset();
	mRules.seek(offset);

	while (true)
	{
		offset = mRules.u32();
		if (offset == 0)
		{
			mRules.seek(prev_offset);
			return false;
		}

		const uint8_t *start = (const uint8_t *)mRules.pstr(offset);
		const uint8_t *match = start;
		while (*match) ++match;
		--match;

		const uint8_t *check = current;
		while (match >= start && check >= mStart && *check == *match)
		{
			--match;
			--check;
		}

		if (match < start)
		{
			current = check;
			mRules.seek(prev_offset);
			return true;
		}
	}
}

std::pair<const uint8_t *, const char *>
ruleset::next_match(const uint8_t *current, elision_rules_t elision)
{
	while (current != mEnd && *current == mBoundary)
		++current;

	if (current == mEnd) return { nullptr, nullptr };

	uint32_t offset = mRuleGroups[*current];
	if (offset == 0)
	{
		++current;
		throw tts::phoneme_error(i18n("unable to pronounce the text"));
	}

	mRules.seek(offset);

	static const uint8_t null_rule[] = { 0 };

	enum state_t
	{
		in_rule_group,
		context_match,
		left_match,
		right_match,
	};

	ipa::phoneme prev_phoneme;
	state_t state = in_rule_group;
	const uint8_t *rule     = null_rule;
	const uint8_t *context  = nullptr;
	const uint8_t *left     = nullptr;
	const uint8_t *right    = nullptr;
	const char    *phonemes = nullptr;
	const char    *phonemes_ctx = nullptr;
	const char    *phonemes_end = nullptr;
	std::pair<const uint8_t *, const char *> ctx;
	while (true) switch (*rule)
	{
	case 0:
		switch (state)
		{
		case in_rule_group:
			rule = (const uint8_t *)mRules.pstr();
			if (*rule == 0)
			{
				if (elision == ignore_elision_rules)
					return next_match(context, elision);
				++current;
				throw tts::phoneme_error(i18n("unable to pronounce the text"));
			}

			phonemes = mRules.pstr();
			phonemes_end = phonemes + strlen(phonemes);
			context = left = right = current;
			state = context_match;
			break;
		default:
			if (elision == ignore_elision_rules && *phonemes == 0)
				return next_match(context, elision);
			else
				return { context, phonemes };
			break;
		}
		break;
	case '@':
		++rule;
		if (!mConditionalFlags[*rule++])
		{
			state = in_rule_group;
			rule = null_rule;
		}
		break;
	case '!':
		++rule;
		if (mConditionalFlags[*rule++])
		{
			state = in_rule_group;
			rule = null_rule;
		}
		break;
	case '(':
		right = context;
		state = right_match;
		++rule;
		break;
	case ')':
		state = left_match;
		++rule;
		--left;
		break;
	case '_':
		switch (state)
		{
		case left_match:
			if (left == mStart - 1)
				++rule;
			else if (*left == mBoundary)
			{
				++rule;
				--left;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		case right_match:
			if (right == mEnd)
				++rule;
			else if (*right == mBoundary)
			{
				++rule;
				++right;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		default:
			state = in_rule_group;
			rule = null_rule;
			break;
		}
		break;
	case '{':
		switch (state)
		{
		case right_match:
			prev_phoneme = mPreviousPhoneme;
			phonemes_ctx = phonemes;
			mPhonemeSet->parse(phonemes_ctx, phonemes_end, mPreviousPhoneme);
			offset = mRules.offset();
			ctx = next_match(right, ignore_elision_rules);
			mRules.seek(offset);
			mPreviousPhoneme = prev_phoneme;
			if (ctx.first != nullptr && match_features_next(ctx.second, rule))
			{
				right = ctx.first;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		default:
			state = in_rule_group;
			rule = null_rule;
			break;
		}
		break;
	case '}':
		switch (state)
		{
		case left_match:
			if (!match_features_prev(rule))
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		default:
			state = in_rule_group;
			rule = null_rule;
			break;
		}
		break;
	case '/':
		switch (state)
		{
		case right_match:
			prev_phoneme = mPreviousPhoneme;
			phonemes_ctx = phonemes;
			mPhonemeSet->parse(phonemes_ctx, phonemes_end, mPreviousPhoneme);
			offset = mRules.offset();
			ctx = next_match(right, ignore_elision_rules);
			mRules.seek(offset);
			mPreviousPhoneme = prev_phoneme;
			if (ctx.first != nullptr && match_phonemes_next(ctx.second, rule))
			{
				right = ctx.first;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		default:
			state = in_rule_group;
			rule = null_rule;
			break;
		}
		break;
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
	case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S':
	case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
		offset = mClassDefs[*rule];
		if (offset == 0)
		{
			state = in_rule_group;
			rule = null_rule;
		}
		else switch (state)
		{
		case context_match:
			if (match_classdef(offset, context))
			{
				++rule;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		case left_match:
			if (match_classdef_back(offset, left))
			{
				++rule;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		case right_match:
			if (match_classdef(offset, right))
			{
				++rule;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		}
		break;
	default:
		switch (state)
		{
		case context_match:
			if (context < mEnd && *context == *rule)
			{
				++rule;
				++context;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		case left_match:
			if (left >= mStart && *left == *rule)
			{
				++rule;
				--left;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		case right_match:
			if (right < mEnd && *right == *rule)
			{
				++rule;
				++right;
			}
			else
			{
				state = in_rule_group;
				rule = null_rule;
			}
			break;
		}
		break;
	}
}

std::shared_ptr<tts::phoneme_reader>
tts::createPronunciationRules(const char *aRuleSetPath,
                              const cainteoir::language::tag &aLocale)
{
	if (!aRuleSetPath) return {};

	auto data = cainteoir::make_file_buffer(aRuleSetPath);
	if (!data) return {};

	const char *header = data->begin();

	if (strncmp(header, "LANGDB", 6) != 0 || *(const uint16_t *)(header + 6) != 0x3031)
		return {};

	return std::make_shared<ruleset>(data, aLocale);
}
