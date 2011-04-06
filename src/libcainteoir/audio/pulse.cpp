/* PulseAudio Support.
 *
 * Copyright (C) 2010 Reece H. Dunn
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

#include <cainteoir/audio.hpp>
#include <pulse/simple.h>
#include <string.h>
#include <stdio.h>

class pulse_audio : public cainteoir::audio
{
	pa_simple *pa;
	pa_sample_spec ss;
	const char *m_device;
public:
	pulse_audio(const char *device, cainteoir::audio_format format, int channels, int frequency)
		: pa(NULL)
		, m_device(device)
	{
		switch (format)
		{
		case cainteoir::S16_LE:
			ss.format = PA_SAMPLE_S16LE;
			break;
		case cainteoir::FLOAT32_LE:
			ss.format = PA_SAMPLE_FLOAT32LE;
			break;
		}

		ss.channels = channels;
		ss.rate = frequency;
	}

	~pulse_audio()
	{
		close();
	}

	void open()
	{
		if (!pa)
			pa = pa_simple_new(m_device, "Cainteoir Text-to-Speech", PA_STREAM_PLAYBACK, NULL, "Music", &ss, NULL, NULL, NULL);
	}

	void close()
	{
		if (pa)
		{
			pa_simple_free(pa);
			pa = NULL;
		}
	}

	uint32_t write(const char *data, uint32_t len)
	{
		return pa_simple_write(pa, data, len, NULL);
	}
};

std::auto_ptr<cainteoir::audio> cainteoir::create_pulseaudio_device(const char *device, cainteoir::audio_format format, int channels, int frequency, float quality, const rdf::graph &aMetadata, const rdf::uri &aDocument)
{
	return std::auto_ptr<cainteoir::audio>(new pulse_audio(device, format, channels, frequency));
}

