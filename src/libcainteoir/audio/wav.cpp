/* WAVE Audio File.
 *
 * Copyright (C) 2010-2015 Reece H. Dunn
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
#include "i18n.h"

#include <cainteoir/audio.hpp>
#include <stdexcept>
#include <string.h>
#include <stdio.h>

namespace rdf = cainteoir::rdf;
namespace rql = cainteoir::rdf::query;

struct WaveHeader
{
	// RIFF header
	char id[4];
	uint32_t size;
	char format[4];
	// fmt header
	char fmt_id[4];
	uint32_t fmt_size;
	uint16_t type;
	uint16_t channels;
	uint32_t frequency;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t sample_size;
	// data header
	char data_id[4];
	uint32_t data_size;
};

#define WAVE_FORMAT_PCM 0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003

class wav_audio : public cainteoir::audio
{
	FILE *m_file;
	WaveHeader m_header;
	const rdf::uri mFormat;
public:
	wav_audio(FILE *f, const rdf::uri &format, int channels, int frequency)
		: m_file(f)
		, mFormat(format)
	{
		WaveHeader header = {
			{ 'R', 'I', 'F', 'F' }, 0x7FFFFFFF, { 'W', 'A', 'V', 'E' },
			{ 'f', 'm', 't', ' ' }, 16, 0, 0, 0, 0, 0, 0,
			{ 'd', 'a', 't', 'a' }, 0x7FFFFFFF - sizeof(WaveHeader)
		};

		header.channels = channels;
		header.frequency = frequency;

		if (format == rdf::tts("s8"))
		{
			header.sample_size = 8;
			header.type = WAVE_FORMAT_PCM;
		}
		else if (format == rdf::tts("s16le"))
		{
			header.sample_size = 16;
			header.type = WAVE_FORMAT_PCM;
		}
		else if (format == rdf::tts("float32le"))
		{
			header.sample_size = 32;
			header.type = WAVE_FORMAT_IEEE_FLOAT;
		}
		else
			throw std::runtime_error(i18n("unsupported audio format."));

		header.byte_rate = header.frequency * header.channels * (header.sample_size / 8);
		header.block_align = header.channels * (header.sample_size / 8);

		m_header = header;
	}

	~wav_audio()
	{
		close();
	}

	void open()
	{
		fwrite(&m_header, 1, sizeof(m_header), m_file);
		m_header.size = sizeof(WaveHeader);
		m_header.data_size = 0;
	}

	void close()
	{
		if (!m_file || m_file == stdout)
			return;

		fseek(m_file, 0, SEEK_SET);
		fwrite(&m_header, 1, sizeof(m_header), m_file);

		fclose(m_file);
		m_file = nullptr;
	}

	uint32_t write(const char *data, uint32_t len)
	{
		m_header.size += len;
		m_header.data_size += len;

		return fwrite(data, 1, len, m_file);
	}

	int channels() const { return m_header.channels; }

	int frequency() const { return m_header.frequency; }

	const rdf::uri &format() const { return mFormat; }
};

std::shared_ptr<cainteoir::audio>
cainteoir::create_wav_file(const char *aFileName,
                           const rdf::uri &aFormat,
                           int aChannels,
                           int aFrequency)
{
	FILE *file = aFileName ? fopen(aFileName, "wb") : stdout;
	if (!file) throw std::runtime_error(strerror(errno));
	return std::make_shared<wav_audio>(file, aFormat, aChannels, aFrequency);
}

std::shared_ptr<cainteoir::audio>
cainteoir::create_wav_file(const char *aFileName,
                           const rdf::graph &aVoiceMetadata,
                           const rdf::uri &aVoice)
{
	rql::results data = rql::select(aVoiceMetadata, rql::subject == aVoice);
	int channels  = rql::select_value<int>(data, rql::predicate == rdf::tts("channels"));
	int frequency = rql::select_value<int>(data, rql::predicate == rdf::tts("frequency"));
	const rdf::uri &format = rql::object(rql::select(data, rql::predicate == rdf::tts("audio-format")).front());

	return create_wav_file(aFileName, format, channels, frequency);
}
