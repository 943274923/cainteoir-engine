/* Word Stream.
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
#include <stack>

namespace tts = cainteoir::tts;

struct punctuation_t
{
	ucd::codepoint_t codepoint;
	tts::event_type event;
};

std::initializer_list<punctuation_t> punctuation =
{
	{ 0x000021, tts::exclamation }, // EXCLAMATION MARK
	{ 0x00002C, tts::comma       }, // COMMA
	{ 0x00002E, tts::full_stop   }, // FULL STOP
	{ 0x00003A, tts::colon       }, // COLON
	{ 0x00003B, tts::semicolon   }, // SEMICOLON
	{ 0x00003F, tts::question    }, // QUESTION MARK
	{ 0x00037E, tts::question    }, // GREEK QUESTION MARK
	{ 0x000387, tts::semicolon   }, // GREEK ANO TELEIA
	{ 0x00055B, tts::full_stop   }, // ARMENIAN EMPHASIS MARK
	{ 0x00055C, tts::exclamation }, // ARMENIAN EXCLAMATION MARK
	{ 0x00055D, tts::comma       }, // ARMENIAN COMMA
	{ 0x00055E, tts::question    }, // ARMENIAN QUESTION MARK
	{ 0x000589, tts::full_stop   }, // ARMENIAN FULL STOP
	{ 0x00060C, tts::comma       }, // ARABIC COMMA
	{ 0x00061B, tts::semicolon   }, // ARABIC SEMICOLON
	{ 0x00061F, tts::question    }, // ARABIC QUESTION MARK
	{ 0x0006D4, tts::full_stop   }, // ARABIC FULL STOP
	{ 0x0007F8, tts::comma       }, // NKO COMMA
	{ 0x0007F9, tts::exclamation }, // NKO EXCLAMATION MARK
	{ 0x000EAF, tts::ellipsis    }, // LAO ELLIPSIS
	{ 0x000F0D, tts::full_stop   }, // TIBETAN MARK SHAD
	{ 0x000F0E, tts::paragraph   }, // TIBETAN MARK NYIS SHAD
	{ 0x000964, tts::full_stop   }, // DEVANAGARI DANDA
	{ 0x0010FB, tts::paragraph   }, // GEORGIAN PARAGRAPH SEPARATOR
	{ 0x001362, tts::full_stop   }, // ETHIOPIC FULL STOP
	{ 0x001363, tts::comma       }, // ETHIOPIC COMMA
	{ 0x001364, tts::semicolon   }, // ETHIOPIC SEMICOLON
	{ 0x001365, tts::colon       }, // ETHIOPIC COLON
	{ 0x001366, tts::colon       }, // ETHIOPIC PREFACE COLON
	{ 0x001367, tts::question    }, // ETHIOPIC QUESTION MARK
	{ 0x001368, tts::paragraph   }, // ETHIOPIC PARAGRAPH SEPARATOR
	{ 0x00166E, tts::full_stop   }, // CANADIAN SYLLABICS FULL STOP
	{ 0x001801, tts::ellipsis    }, // MONGOLIAN ELLIPSIS
	{ 0x001802, tts::comma       }, // MONGOLIAN COMMA
	{ 0x001803, tts::full_stop   }, // MONGOLIAN FULL STOP
	{ 0x001804, tts::colon       }, // MONGOLIAN COLON
	{ 0x001808, tts::comma       }, // MONGOLIAN MANCHU COMMA
	{ 0x001809, tts::full_stop   }, // MONGOLIAN MANCHU FULL STOP
	{ 0x001944, tts::exclamation }, // LIMBU EXCLAMATION MARK
	{ 0x001945, tts::question    }, // LIMBU QUESTION MARK
	{ 0x002026, tts::ellipsis    }, // HORIZONTAL ELLIPSIS
	{ 0x00204F, tts::semicolon   }, // REVERSED SEMICOLON
	{ 0x0022EE, tts::ellipsis    }, // VERTICAL ELLIPSIS
	{ 0x002CF9, tts::full_stop   }, // COPTIC OLD NUBIAN FULL STOP
	{ 0x002CFA, tts::question    }, // COPTIC OLD NUBIAN DIRECT QUESTION MARK
	{ 0x002CFB, tts::question    }, // COPTIC OLD NUBIAN INDIRECT QUESTION MARK
	{ 0x002CFE, tts::full_stop   }, // COPTIC FULL STOP
	{ 0x003001, tts::comma       }, // IDEOGRAPHIC COMMA
	{ 0x003002, tts::full_stop   }, // IDEOGRAPHIC FULL STOP
	{ 0x00A4FE, tts::comma       }, // LISU PUNCTUATION COMMA
	{ 0x00A4FF, tts::full_stop   }, // LISU PUNCTUATION FULL STOP
	{ 0x00A60D, tts::comma       }, // VAI COMMA
	{ 0x00A60E, tts::full_stop   }, // VAI FULL STOP
	{ 0x00A60F, tts::question    }, // VAI QUESTION MARK
	{ 0x00A6F3, tts::full_stop   }, // BAMUM FULL STOP
	{ 0x00A6F4, tts::colon       }, // BAMUM COLON
	{ 0x00A6F5, tts::comma       }, // BAMUM COMMA
	{ 0x00A6F6, tts::semicolon   }, // BAMUM SEMICOLON
	{ 0x00A6F7, tts::question    }, // BAMUM QUESTION MARK
	{ 0x00AAF1, tts::question    }, // MEETEI QUESTION MARK
	{ 0x00FE10, tts::comma       }, // PRESENTATION FORM FOR VERTICAL COMMA
	{ 0x00FE11, tts::comma       }, // PRESENTATION FORM FOR VERTICAL IDEOGRAPHIC COMMA
	{ 0x00FE12, tts::full_stop   }, // PRESENTATION FORM FOR VERTICAL IDEOGRAPHIC FULL STOP
	{ 0x00FE13, tts::colon       }, // PRESENTATION FORM FOR VERTICAL COLON
	{ 0x00FE14, tts::semicolon   }, // PRESENTATION FORM FOR VERTICAL SEMICOLON
	{ 0x00FE15, tts::exclamation }, // PRESENTATION FORM FOR VERTICAL EXCLAMATION MARK
	{ 0x00FE16, tts::question    }, // PRESENTATION FORM FOR VERTICAL QUESTION MARK
	{ 0x00FE19, tts::ellipsis    }, // PRESENTATION FORM FOR VERTTICAL HORIZONTAL ELLIPSIS
	{ 0x00FE50, tts::comma       }, // SMALL COMMA
	{ 0x00FE51, tts::comma       }, // SMALL IDEOGRAPHIC COMMA
	{ 0x00FE52, tts::full_stop   }, // SMALL FULL STOP
	{ 0x00FE54, tts::semicolon   }, // SMALL SEMICOLON
	{ 0x00FE55, tts::colon       }, // SMALL COLON
	{ 0x00FE56, tts::question    }, // SMALL QUESTION MARK
	{ 0x00FE57, tts::exclamation }, // SMALL EXCLAMATION MARK
	{ 0x00FF01, tts::exclamation }, // FULLWIDTH EXCLAMATION MARK
	{ 0x00FF0C, tts::comma       }, // FULLWIDTH COMMA
	{ 0x00FF0E, tts::full_stop   }, // FULLWIDTH FULL STOP
	{ 0x00FF1A, tts::colon       }, // FULLWIDTH COLON
	{ 0x00FF1B, tts::semicolon   }, // FULLWIDTH SEMICOLON
	{ 0x00FF1F, tts::question    }, // FULLWIDTH QUESTION MARK
	{ 0x00FF61, tts::full_stop   }, // HALFWIDTH IDEOGRAPHIC FULL STOP
	{ 0x00FF64, tts::comma       }, // HALFWIDTH IDEOGRAPHIC COMMA
	{ 0x011143, tts::question    }, // CHAKMA QUESTION MARK
	{ 0x012471, tts::colon       }, // CUNEIFORM PUNCTUATION SIGN VERTICAL COLON
	{ 0x012472, tts::colon       }, // CUNEIFORM PUNCTUATION SIGN DIAGONAL COLON
};

static tts::event_type punctuation_type(ucd::codepoint_t cp)
{
	for (auto &item : punctuation)
	{
		if (cp == item.codepoint)
			return item.event;
		if (cp < item.codepoint)
			return tts::punctuation;
	}
	return tts::punctuation;
}

struct number_words
{
	ucd::script script;
	uint16_t     max_group;
	const char **groups;
	const char  *hundred;
	const char  *group_separator;
	const char  *hundreds_separator;
	const char  *ties[8];
	const char  *zero_to_nineteen[20];
};

// short scale (US, Canada and Modern British)
static const char *en_GB_groups[] =
{
	/*  0 */ NULL,
	/*  3 */ "thousand",
	/*  6 */ "million",
	/*  9 */ "billion",
	/* 12 */ "trillion",
	/* 15 */ "quadrillion",
	/* 18 */ "quintillion",
	/* 21 */ "sextillion",
	/* 24 */ "septillion",
	/* 27 */ "octillion",
	/* 30 */ "nonillion",
};

static const number_words en_GB =
{
	ucd::Latn,
	11,
	en_GB_groups,
	"hundred",
	"and",
	"and",
	{
		"twenty",
		"thirty",
		"forty",
		"fifty",
		"sixty",
		"seventy",
		"eighty",
		"ninety",
	},
	{
		"zero",
		"one",
		"two",
		"three",
		"four",
		"five",
		"six",
		"seven",
		"eight",
		"nine",
		"ten",
		"eleven",
		"twelve",
		"thirteen",
		"fourteen",
		"fifteen",
		"sixteen",
		"seventeen",
		"eighteen",
		"nineteen",
	}
};

struct number_block
{
	uint16_t rank;
	uint16_t value;
};

static std::stack<number_block> parse_number(const tts::text_event &number, uint16_t max_block)
{
	std::stack<number_block> blocks;
	uint16_t value = 0; // The numeric value of the current group of 3 digits.
	uint16_t scale = 1; // The scale factor to apply to the current digit.
	uint16_t n     = 0; // The current group order -- use 10^3n for thousands, millions, etc.
	for (char c : cainteoir::reverse(*number.text))
	{
		if (c >= '0' && c <= '9')
		{
			value += ((c - '0') * scale);
			scale *= 10;
			if (scale == 1000)
			{
				blocks.push({ n, value });
				scale = 1;
				value = 0;
				++n;
				if (n == max_block)
				{
					fprintf(stdout, "error: number exceeded maximum block count %d.\n", n);
					return blocks;
				}
			}
		}
		else
			fprintf(stdout, "error: unsupported number character '%c'.\n", c);
	}
	if (scale != 1)
		blocks.push({ n, value });
	return blocks;
}

struct word_builder
{
	word_builder(std::queue<tts::text_event> &aEvents)
		: mEvents(aEvents)
	{
	}

	void push(ucd::script aScript, const tts::text_event &aEvent, const char *aWord)
	{
		mEvents.push({ std::make_shared<cainteoir::buffer>(aWord),
		               tts::word_lowercase,
		               aScript,
		               aEvent.range,
		               0 });
	}
private:
	std::queue<tts::text_event> &mEvents;
};

static void parse_cardinal_number(word_builder events, const tts::text_event &number, const number_words &words)
{
	std::stack<number_block> blocks = parse_number(number, words.max_group);
	bool need_zero  = true;
	bool need_and   = false;
	while (!blocks.empty())
	{
		auto item = blocks.top();
		blocks.pop();

		bool need_group = item.value != 0;

		if (item.value >= 100)
		{
			events.push(words.script, number, words.zero_to_nineteen[item.value / 100]);
			events.push(words.script, number, words.hundred);
			item.value %= 100;
			need_zero = false;
			need_and  = false;

			if (item.value != 0 && words.hundreds_separator)
				events.push(words.script, number, words.hundreds_separator);
		}

		if (need_and && item.rank == 0)
		{
			if (item.value != 0)
				events.push(words.script, number, words.group_separator);
			need_and = false;
		}

		if (item.value >= 20)
		{
			events.push(words.script, number, words.ties[(item.value - 20) / 10]);
			item.value %= 10;
			need_zero = false;
		}

		if (item.value == 0)
		{
			if (item.rank == 0 && need_zero)
				events.push(words.script, number, words.zero_to_nineteen[item.value]);
		}
		else
		{
			events.push(words.script, number, words.zero_to_nineteen[item.value]);
			need_zero = false;
		}

		if (item.rank > 0 && need_group)
		{
			events.push(words.script, number, words.groups[item.rank]);
			need_and = true;
		}
	}
}

bool tts::word_stream::read()
{
	if (mEntries.empty())
	{
		while (mEntries.empty() && mReader.read())
		{
			auto &event = mReader.event();
			switch (event.type)
			{
			case tts::word_uppercase:
			case tts::word_lowercase:
			case tts::word_capitalized:
			case tts::word_mixedcase:
			case tts::word_script:
				mEntries.push(event);
				break;
			case tts::paragraph:
				mEntries.push(event);
				break;
			case tts::number:
				parse_cardinal_number(mEntries, event, en_GB);
				break;
			case tts::punctuation:
			case tts::symbol:
				{
					tts::event_type type = punctuation_type(event.codepoint);
					if (type != tts::punctuation)
						mEntries.push({ event.text, type, event.script, event.range, event.codepoint });
				}
				break;
			}
		}

		// Don't pop the first matching item:
		return !mEntries.empty();
	}

	mEntries.pop();
	if (mEntries.empty())
		return read();

	return true;
}
