/* ZIP File Reader API.
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

#include "parsers.hpp"

#include <zlib.h>
#include <stdexcept>

std::tr1::shared_ptr<cainteoir::buffer> inflateBuffer(const cainteoir::buffer &compressed, uint32_t uncompressed, int window)
{
	std::tr1::shared_ptr<cainteoir::buffer> data(new cainteoir::data_buffer(uncompressed));

	z_stream strm = {0};
	int ret = inflateInit2(&strm, window);
	if (ret != Z_OK)
		goto err;

	strm.avail_in = compressed.size();
	strm.next_in = (Bytef *)compressed.begin();

	strm.avail_out = uncompressed;
	strm.next_out = (Bytef *)data->begin();

	ret = inflate(&strm, Z_SYNC_FLUSH);
	(void)inflateEnd(&strm);

	if (ret != Z_STREAM_END)
		goto err;

	return data;

err:
	switch (ret)
	{
	case Z_STREAM_ERROR:
		throw std::runtime_error("decompression failed (invalid compression level)");
	case Z_DATA_ERROR:
		throw std::runtime_error("decompression failed (invalid or incomplete deflate data)");
	case Z_MEM_ERROR:
		throw std::runtime_error("decompression failed (out of memory)");
	case Z_VERSION_ERROR:
		throw std::runtime_error("decompression failed (zlib version mismatch)");
	default:
		throw std::runtime_error("decompression failed (unspecified error)");
	}
}

std::tr1::shared_ptr<cainteoir::buffer> cainteoir::strm_copy(const cainteoir::buffer &compressed)
{
	std::tr1::shared_ptr<cainteoir::buffer> data(new cainteoir::data_buffer(compressed.size()));
	memcpy((char *)data->begin(), compressed.begin(), compressed.size());

	return data;
}

std::tr1::shared_ptr<cainteoir::buffer> cainteoir::strm_inflate(const cainteoir::buffer &compressed, uint32_t uncompressed)
{
	return inflateBuffer(compressed, uncompressed, -MAX_WBITS);
}

std::tr1::shared_ptr<cainteoir::buffer> cainteoir::strm_gzip_decompress(const cainteoir::buffer &compressed)
{
	uint32_t uncompressed = *((uint32_t *)(compressed.end() - 4));
	return inflateBuffer(compressed, uncompressed, 16 + MAX_WBITS);
}
