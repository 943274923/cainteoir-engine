/* Multi-type Object API.
 *
 * Copyright (C) 2015 Reece H. Dunn
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

#include <cainteoir/object.hpp>

cainteoir::object::object(const std::shared_ptr<cainteoir::buffer> &aValue)
	: mType(cainteoir::object_type::buffer)
{
	new (&mBufferVal)buffer_t(aValue);
}

cainteoir::object::object(const cainteoir::range<uint32_t> &aValue)
	: mType(cainteoir::object_type::range)
{
	new (&mRangeVal)range_t(aValue);
}

cainteoir::object::object(const object_type aType)
	: mType(aType)
{
	switch (aType)
	{
	case object_type::buffer:
		new (&mBufferVal)buffer_t();
		break;
	case object_type::range:
		new (&mRangeVal)range_t(0, 0);
		break;
	case object_type::dictionary:
		new (&mDictionaryVal)dictionary_t(std::make_shared<dictionary_t::element_type>());
		break;
	case object_type::phoneme:
		mPhonemeVal = {};
		break;
	default:
		mStringVal = {};
		break;
	}
}

cainteoir::object::object(const object &o)
	: mType(o.mType)
{
	switch (o.mType)
	{
	case object_type::buffer:
		new (&mBufferVal)buffer_t(o.mBufferVal);
		break;
	case object_type::range:
		new (&mRangeVal)range_t(o.mRangeVal);
		break;
	case object_type::dictionary:
		new (&mDictionaryVal)dictionary_t(o.mDictionaryVal);
		break;
	case object_type::phoneme:
		mPhonemeVal = o.mPhonemeVal;
		break;
	default:
		mStringVal = o.mStringVal;
		break;
	}
}

cainteoir::object::~object()
{
	clear();
}

cainteoir::object &
cainteoir::object::operator=(const object &o)
{
	clear();

	mType = o.mType;
	switch (o.mType)
	{
	case object_type::buffer:
		new (&mBufferVal)buffer_t(o.mBufferVal);
		break;
	case object_type::range:
		new (&mRangeVal)range_t(o.mRangeVal);
		break;
	case object_type::dictionary:
		new (&mDictionaryVal)dictionary_t(o.mDictionaryVal);
		break;
	case object_type::phoneme:
		mPhonemeVal = o.mPhonemeVal;
		break;
	default:
		mStringVal = o.mStringVal;
		break;
	}
	return *this;
}

const cainteoir::object *
cainteoir::object::get(const char *aKey) const
{
	if (mType == object_type::dictionary)
	{
		auto match = mDictionaryVal->find(aKey);
		if (match != mDictionaryVal->end())
			return &(*match).second;
	}
	return nullptr;
}

bool
cainteoir::object::put(const char *aKey, const object &aValue)
{
	if (mType == object_type::dictionary)
	{
		(*mDictionaryVal)[aKey] = aValue;
		return true;
	}
	return false;
}

void
cainteoir::object::clear()
{
	switch (mType)
	{
	case object_type::buffer:
		(&mBufferVal)->~buffer_t();
		break;
	case object_type::range:
		(&mRangeVal)->~range_t();
		break;
	case object_type::dictionary:
		(&mDictionaryVal)->~dictionary_t();
		break;
	default:
		break;
	}
}
