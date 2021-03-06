/* An interface to the MBROLA synthesizer.
 *
 * Copyright (C) 2013-2014 Reece H. Dunn
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

#include "../synthesizer/synth.hpp"
#include <stdexcept>

namespace tts = cainteoir::tts;
namespace ipa = cainteoir::ipa;
namespace rdf = cainteoir::rdf;
namespace rql = cainteoir::rdf::query;
namespace css = cainteoir::css;

#if defined(HAVE_MBROLA)

#include <cainteoir/path.hpp>

#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

struct procstat_t
{
	enum status_t
	{
		running,  // R (running)
		idle,     // S (sleeping)   -- interruptible
		sleeping, // D (disk sleep) -- uninterruptible
		zombie,   // Z (zombie)
		stopped,  // T (traced/stopped on a signal)
		paging,   // W (paging)
	};

	procstat_t() : fd(-1) {}
	~procstat_t() { close(fd); }

	void open(pid_t pid);

	status_t stat() const;
private:
	int fd;
};

void procstat_t::open(pid_t pid)
{
	if (fd != -1) close(fd);

	char filename[256];
	snprintf(filename, sizeof(filename), "/proc/%d/stat", pid);
	fd = ::open(filename, O_RDONLY);
	if (fd == -1) throw std::runtime_error(strerror(errno));
}

procstat_t::status_t procstat_t::stat() const
{
	char buffer[120];
	if (lseek(fd, 0, SEEK_SET) != 0)
		throw std::runtime_error(strerror(errno));
	if (read(fd, buffer, sizeof(buffer)) != sizeof(buffer))
		throw std::runtime_error(strerror(errno));

	char *s = (char *)memchr(buffer, ')', sizeof(buffer));
	if (!s || (s - buffer) >= sizeof(buffer) - 1 || s[1] != ' ')
		throw std::runtime_error("unable to determine the state of the MBROLA process");
	switch (s[2])
	{
	case 'R': return running;
	case 'S': return idle;
	case 'D': return sleeping;
	case 'Z': return zombie;
	case 'T': return stopped;
	case 'W': return paging;
	default:
		throw std::runtime_error("unable to determine the state of the MBROLA process");
	}
}

struct pipe_t
{
	enum fd_t
	{
		read_fd  = 0,
		write_fd = 1,
	};

	pipe_t()
		: fds{ -1, -1 }
	{
		if (pipe(fds) == -1)
			throw std::runtime_error(strerror(errno));
	}

	~pipe_t()
	{
		::close(fds[read_fd]);
		::close(fds[write_fd]);
	}

	void dup(fd_t type, int fd)
	{
		if (dup2(fds[type], fd) == -1)
			throw std::runtime_error(strerror(errno));
		close(type == read_fd ? write_fd : read_fd);
	}

	void close(fd_t type)
	{
		::close(fds[type]);
		fds[type] = -1;
	}

	ssize_t read(void *buf, size_t count, const procstat_t &proc, timeval timeout);

	ssize_t read(void *buf, size_t count, const procstat_t &proc)
	{
		timeval timeout = { 60, 0 };
		return read(buf, count, proc, timeout);
	}

	FILE *open(const char *mode)
	{
		FILE *ret = fdopen(fds[(mode[0] == 'r') ? read_fd : write_fd], mode);
		if (!ret) throw std::runtime_error(strerror(errno));
		fds[(mode[0] == 'r') ? read_fd : write_fd] = -1;
		return ret;
	}

	void set_flags(fd_t type, int flags)
	{
		fcntl(fds[type], F_SETFL, fcntl(fds[type], F_GETFL) | flags);
	}

	void clear_flags(fd_t type, int flags)
	{
		fcntl(fds[type], F_SETFL, fcntl(fds[type], F_GETFL) & ~flags);
	}
private:
	int fds[2];
};

ssize_t pipe_t::read(void *buf, size_t count, const procstat_t &proc, timeval timeout)
{
	for (;;)
	{
		ssize_t ret = ::read(fds[read_fd], buf, count);
		if (ret < 0) switch (errno)
		{
		case EINTR:
			continue; // normal... try again
		case EAGAIN:
			{
				if (proc.stat() == procstat_t::idle)
					return 0; // idle... no data

				fd_set rset;
				FD_ZERO(&rset);
				FD_SET(fds[read_fd], &rset);
				int ret = select(fds[read_fd] + 1, &rset, nullptr, nullptr, &timeout);
				if (ret < 0) // error
					throw std::runtime_error(strerror(errno));
				if (ret == 0)
					return 0; // timeout... no data
			}
			break;
		default:
			throw std::runtime_error(strerror(errno));
		}
		else
			return ret;
	}
}

struct mbrola_synthesizer : public tts::synthesizer
{
	mbrola_synthesizer(const char *database,
	                   std::shared_ptr<tts::prosody_writer> aWriter,
	                   const char *aVolumeScale,
	                   const std::vector<tts::unit_t> &aUnits,
	                   cainteoir::range<const tts::phoneme_units *> aPhonemes);

	~mbrola_synthesizer();

	/** @name audio_info */
	//@{

	int channels() const { return 1; }

	int frequency() const { return sample_rate; }

	const rdf::uri &format() const { return sample_format; }

	//@}
	/** @name tts::synthesizer */
	//@{

	void bind(const std::shared_ptr<tts::prosody_reader> &aProsody);

	bool synthesize(cainteoir::audio *out);

	//@}
private:
	void flush();

	enum state_t
	{
		need_data,
		have_data,
		write_data,
		read_errors,
		read_error_line,
	};

	pid_t pid;
	procstat_t proc;
	FILE *pho;
	pipe_t audio;
	pipe_t error;
	state_t state;

	char mMessage[512];
	ssize_t mRead;
	ssize_t mAvailable;
	char *mCurrentMessage;

	int sample_rate;
	rdf::uri sample_format;

	std::shared_ptr<tts::prosody_reader> prosody;
	std::shared_ptr<tts::prosody_writer> writer;

	const std::vector<tts::unit_t> &mUnits;
	cainteoir::range<const tts::phoneme_units *> mPhonemes;
};

mbrola_synthesizer::mbrola_synthesizer(const char *aDatabase,
                                       std::shared_ptr<tts::prosody_writer> aWriter,
                                       const char *aVolumeScale,
                                       const std::vector<tts::unit_t> &aUnits,
                                       cainteoir::range<const tts::phoneme_units *> aPhonemes)
	: pho(nullptr)
	, state(need_data)
	, sample_rate(0)
	, sample_format(rdf::tts("s16le"))
	, writer(aWriter)
	, mUnits(aUnits)
	, mPhonemes(aPhonemes)
	, mCurrentMessage(mMessage)
{
	pipe_t input;

	pid = fork();
	if (pid == -1) throw std::runtime_error(strerror(errno));
	if (pid == 0)
	{
		input.dup(pipe_t::read_fd, STDIN_FILENO);
		audio.dup(pipe_t::write_fd, STDOUT_FILENO);
		error.dup(pipe_t::write_fd, STDERR_FILENO);

		execlp("mbrola", "mbrola",
		       "-e",               // ignore fatal errors on unknown diphones
		       "-v", aVolumeScale, // volume scale factor
		       aDatabase,          // voice file database
		       "-",                // pho file input (stdin)
		       "-.wav",            // audio output (stdout)
                       nullptr);
		_exit(1);
	}

	proc.open(pid);

	input.close(pipe_t::read_fd);
	audio.close(pipe_t::write_fd);
	error.close(pipe_t::write_fd);

	input.set_flags(pipe_t::write_fd, O_NONBLOCK);
	audio.set_flags(pipe_t::read_fd,  O_NONBLOCK);
	error.set_flags(pipe_t::read_fd,  O_NONBLOCK);

	pho = input.open("w");
	writer->reset(pho);

	flush();
	uint8_t header[44];
	if (audio.read(header, sizeof(header), proc) != sizeof(header))
		throw std::runtime_error("MBROLA did not return a WAV header.");

	if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVEfmt ", 8) != 0)
		throw std::runtime_error("MBROLA did not return a WAV header.");

	sample_rate = header[24] + (header[25] << 8) + (header[26] << 16) + (header[27] << 24);
}

mbrola_synthesizer::~mbrola_synthesizer()
{
	fclose(pho);
}

void mbrola_synthesizer::bind(const std::shared_ptr<tts::prosody_reader> &aProsody)
{
	prosody = tts::create_unit_reader(aProsody, mUnits, mPhonemes);
}

bool mbrola_synthesizer::synthesize(cainteoir::audio *out)
{
	if (!prosody) return false;

	short data[1024];
	ssize_t read;

	while (true) switch (state)
	{
	case need_data:
	case have_data:
		if (!prosody->read())
		{
			if (state == need_data)
				return false;
			flush();
			state = write_data;
			continue;
		}

		if (writer->write(*prosody))
			state = have_data;

		if (prosody->first.phoneme1 == ipa::intonation_break &&
		    prosody->first.phoneme2 == ipa::unspecified)
		{
			if (state == have_data)
			{
				flush();
				state = write_data;
			}
		}
		break;
	case write_data:
		read = audio.read(data, sizeof(data), proc);
		if (read > 0)
		{
			if (out)
				out->write((const char *)data, read);
			state = read_errors;
		}
		else
		{
			state = need_data;
			return true;
		}
		break;
	case read_errors:
		mAvailable = sizeof(mMessage) - (mCurrentMessage - mMessage);
		mRead = error.read(mCurrentMessage, mAvailable, proc, { 0, 50 });
		state = (mRead > 0) ? read_error_line : write_data;
		break;
	case read_error_line:
		if (mRead > 0)
		{
			char *eol = (char *)memchr(mCurrentMessage, '\n', mAvailable);
			char *msg = nullptr;
			if (eol != nullptr)
			{
				msg = mCurrentMessage;
				*eol = '\0';

				mRead -= strlen(mCurrentMessage) + 1;
				mCurrentMessage = eol + 1;
				mAvailable = sizeof(mMessage) - (mCurrentMessage - mMessage);

				char *warn    = strstr(msg, "Warning: ");
				char *unknown = strstr(msg, " unkown, replaced with "); // typo present in mbrola
				if (warn != nullptr && unknown != nullptr)
				{
					*unknown = '\0';
					char error_msg[128];
					snprintf(error_msg, sizeof(error_msg), "unknown diphone `%s`, replaced with `%s`", msg + 9, unknown + 23);
					throw tts::unknown_diphone(error_msg);
				}
			}
			else if (mRead != 0)
			{
				strncpy(mCurrentMessage, mMessage, mRead);
				mCurrentMessage = mMessage + mRead;
				state = read_errors;
			}
		}
		else
		{
			mCurrentMessage = mMessage;
			state = read_errors;
		}
		break;
	default:
		state = need_data;
		break;
	}
}

void mbrola_synthesizer::flush()
{
	fputs("#\n", pho);
	fflush(pho);
}

struct mbrola_voice : public tts::voice
{
	mbrola_voice(const std::shared_ptr<cainteoir::buffer> &aData,
	             const rdf::graph &aMetadata,
	             const rdf::uri &aVoice);

	std::shared_ptr<tts::synthesizer> synthesizer();

	std::shared_ptr<tts::duration_model> durations() { return mDurations; }

	std::shared_ptr<tts::pitch_model> pitch_model() { return mPitchModel; }

	std::shared_ptr<tts::prosody_reader>
	unit_reader(const std::shared_ptr<tts::prosody_reader> &aProsody);

	std::shared_ptr<tts::prosody_writer> unit_writer() { return mWriter; }

	cainteoir::range<const tts::phoneme_units *> phones() { return mPhonemes; };
private:
	char mDatabase[256];
	std::string mVolumeScale;
	std::shared_ptr<tts::prosody_writer> mWriter;
	std::shared_ptr<cainteoir::buffer> mData;
	std::shared_ptr<tts::duration_model> mDurations;

	std::vector<tts::unit_t> mUnits;
	cainteoir::range<const tts::phoneme_units *> mPhonemes;
	std::shared_ptr<tts::pitch_model> mPitchModel;
};

mbrola_voice::mbrola_voice(const std::shared_ptr<cainteoir::buffer> &aData,
                           const rdf::graph &aMetadata,
                           const rdf::uri &aVoice)
	: mData(aData)
	, mPhonemes(nullptr, nullptr)
{
	snprintf(mDatabase, sizeof(mDatabase), MBROLA_DIR "/%s/%s", aVoice.ref.c_str(), aVoice.ref.c_str());

	const auto voice = rql::select(aMetadata, rql::subject == aVoice);
	mVolumeScale = rql::select_value<std::string>(voice, rql::predicate == rdf::tts("volumeScale"));

	auto phonemes = tts::create_unit_writer(mUnits, "_");
	mWriter = tts::createPhoWriter(phonemes);

	cainteoir::native_endian_buffer data(aData);
	data.seek(tts::VOICEDB_HEADER_SIZE);
	while (!data.eof()) switch (data.magic())
	{
	case tts::STRING_TABLE_MAGIC:
		data.seek(data.u32());
		break;
	case tts::DURATION_TABLE_MAGIC:
		mDurations = tts::createDurationModel(data);
		break;
	case tts::PHONEME_UNIT_TABLE_MAGIC:
		read_phoneme_units(data, mUnits);
		break;
	case tts::PHONEME_TABLE_MAGIC:
		mPhonemes = data.array<tts::phoneme_units>();
		break;
	case tts::PITCH_DATA_MAGIC:
		{
			css::frequency base{ data.f16_16(), css::frequency::hertz };
			css::frequency step{ data.f16_16(), css::frequency::hertz };
			css::frequency sdev{ data.f16_16(), css::frequency::hertz };
			mPitchModel = std::make_shared<tts::pitch_model>(base, step, sdev);
		}
		break;
	default:
		throw std::runtime_error("unsupported section in MBROLA voice file");
	}
}

std::shared_ptr<tts::synthesizer> mbrola_voice::synthesizer()
{
	return std::make_shared<mbrola_synthesizer>(mDatabase, mWriter, mVolumeScale.c_str(), mUnits, mPhonemes);
}

std::shared_ptr<tts::prosody_reader>
mbrola_voice::unit_reader(const std::shared_ptr<tts::prosody_reader> &aProsody)
{
	return tts::create_unit_reader(aProsody, mUnits, mPhonemes);
}

std::shared_ptr<tts::voice>
tts::create_mbrola_voice(const std::shared_ptr<cainteoir::buffer> &aData,
                         const rdf::graph &aMetadata,
                         const rdf::uri &aVoice)
{
	if (aVoice.ref.size() != 3) return {};
	return std::make_shared<mbrola_voice>(aData, aMetadata, aVoice);
}

#else

std::shared_ptr<tts::voice>
tts::create_mbrola_voice(const std::shared_ptr<cainteoir::buffer> &aData,
                         const rdf::graph &aMetadata,
                         const rdf::uri &aVoice)
{
	return {};
}

#endif
