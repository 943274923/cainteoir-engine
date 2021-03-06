/* NCX Document Parser.
 *
 * Copyright (C) 2010-2014 Reece H. Dunn
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

#include "parsers.hpp"
#include <stdexcept>

namespace xml    = cainteoir::xml;
namespace xmlns  = cainteoir::xml::xmlns;
namespace rdf    = cainteoir::rdf;
namespace events = cainteoir::events;

namespace ncx
{
	static const xml::context::entry content_node   = {};
	static const xml::context::entry docAuthor_node = {};
	static const xml::context::entry docTitle_node  = {};
	static const xml::context::entry head_node      = {};
	static const xml::context::entry meta_node      = {};
	static const xml::context::entry navLabel_node  = {};
	static const xml::context::entry navMap_node    = {};
	static const xml::context::entry navPoint_node  = {};
	static const xml::context::entry ncx_node       = {};
	static const xml::context::entry text_node      = {};

	static const xml::context::entry content_attr = {};
	static const xml::context::entry name_attr    = {};
	static const xml::context::entry src_attr     = {};
}

static const std::initializer_list<const xml::context::entry_ref> ncx_nodes =
{
	{ "content",   &ncx::content_node },
	{ "docAuthor", &ncx::docAuthor_node },
	{ "docTitle",  &ncx::docTitle_node },
	{ "head",      &ncx::head_node },
	{ "meta",      &ncx::meta_node },
	{ "navLabel",  &ncx::navLabel_node },
	{ "navMap",    &ncx::navMap_node },
	{ "navPoint",  &ncx::navPoint_node },
	{ "ncx",       &ncx::ncx_node },
	{ "text",      &ncx::text_node },
};

static const std::initializer_list<const xml::context::entry_ref> ncx_attrs =
{
	{ "content", &ncx::content_attr },
	{ "name",    &ncx::name_attr },
	{ "src",     &ncx::src_attr },
};

struct ncx_document_reader : public cainteoir::document_reader
{
	ncx_document_reader(const std::shared_ptr<xml::reader> &aReader,
	                    const rdf::uri &aSubject,
	                    rdf::graph &aPrimaryMetadata,
	                    const std::string &aTitle,
	                    const cainteoir::path &aBaseUri);

	bool read(rdf::graph *aMetadata);

	void generate_reference(rdf::graph *aMetadata,
	                        int depth,
	                        const std::string &title,
	                        const rdf::uri &location);

	std::shared_ptr<xml::reader> reader;
	rdf::uri mSubject;
	rdf::uri mCurrentReference;
	std::string mTitle;
	cainteoir::path mBaseUri;
	int mDepth;
	bool mIsFirst;
};

bool ncx_document_reader::read(rdf::graph *aMetadata)
{
	if (!mTitle.empty())
	{
		generate_reference(aMetadata, 0, mTitle, mSubject);
		mTitle.clear();
	}

	const xml::context::entry *current = nullptr;
	const xml::context::entry *outer   = nullptr;

	content.reset();
	anchor = rdf::uri();

	while (reader->read()) switch (reader->nodeType())
	{
	case xml::reader::textNode:
	case xml::reader::cdataNode:
		if (current == &ncx::text_node)
		{
			if (outer == &ncx::navLabel_node)
				content = reader->nodeValue().buffer();
			current = outer = nullptr;
			if (!anchor.empty())
			{
				generate_reference(aMetadata, mDepth, content->str(),
				                   rdf::uri((mBaseUri / anchor.ns).str(), anchor.ref));
				content = {};
			}
		}
		break;
	case xml::reader::attribute:
		if (current == &ncx::content_node && reader->context() == &ncx::src_attr)
		{
			std::string src = reader->nodeValue().str();
			std::string::size_type ref = src.find('#');
			if (ref == std::string::npos)
				anchor = rdf::uri(src, std::string());
			else
				anchor = rdf::uri(src.substr(0, ref), src.substr(ref+1));
			if (content)
			{
				generate_reference(aMetadata, mDepth, content->str(),
				                   rdf::uri((mBaseUri / anchor.ns).str(), anchor.ref));
				anchor = {};
			}
		}
		break;
	case xml::reader::beginTagNode:
		current = reader->context();
		if (current == &ncx::navPoint_node)
			++mDepth;
		else if (current == &ncx::navLabel_node)
			outer = reader->context();
		break;
	case xml::reader::endTagNode:
		if (reader->context() == &ncx::navPoint_node)
			--mDepth;
		break;
	default:
		break;
	}

	if (!mIsFirst)
		aMetadata->statement(mCurrentReference, rdf::rdf("rest"), rdf::rdf("nil"));
	return false;
}

void ncx_document_reader::generate_reference(rdf::graph *aMetadata,
                                             int depth,
                                             const std::string &title,
                                             const rdf::uri &location)
{
	if (!aMetadata) return;

	if (mIsFirst)
	{
		const rdf::uri listing = aMetadata->genid();
		aMetadata->statement(mSubject, rdf::ref("listing"), listing);

		mCurrentReference = aMetadata->genid();
		aMetadata->statement(listing, rdf::rdf("type"), rdf::ref("Listing"));
		aMetadata->statement(listing, rdf::ref("type"), rdf::epv("toc"));
		aMetadata->statement(listing, rdf::ref("entries"), mCurrentReference);
		mIsFirst = false;
	}
	else
	{
		const rdf::uri next = aMetadata->genid();
		aMetadata->statement(mCurrentReference, rdf::rdf("rest"), next);
		mCurrentReference = next;
	}

	const rdf::uri entry = aMetadata->genid();
	aMetadata->statement(mCurrentReference, rdf::rdf("first"), entry);

	aMetadata->statement(entry, rdf::rdf("type"), rdf::ref("Entry"));
	aMetadata->statement(entry, rdf::ref("level"), rdf::literal(depth, rdf::xsd("integer")));
	aMetadata->statement(entry, rdf::ref("target"), location);
	aMetadata->statement(entry, rdf::dc("title"), rdf::literal(title));
}

ncx_document_reader::ncx_document_reader(const std::shared_ptr<xml::reader> &aReader,
                                         const rdf::uri &aSubject,
                                         rdf::graph &aPrimaryMetadata,
                                         const std::string &aTitle,
                                         const cainteoir::path &aBaseUri)
	: reader(aReader)
	, mSubject(aSubject)
	, mCurrentReference(aSubject)
	, mDepth(0)
	, mIsFirst(true)
	, mBaseUri(aBaseUri)
{
	reader->set_nodes(xmlns::ncx, ncx_nodes);
	reader->set_attrs(xmlns::ncx, ncx_attrs);

	const xml::context::entry *outer   = nullptr;
	const xml::context::entry *current = nullptr;

	std::string name;
	std::string content;
	bool in_header = true;

	while (in_header && reader->read()) switch (reader->nodeType())
	{
	case xml::reader::textNode:
	case xml::reader::cdataNode:
		if (current == &ncx::text_node)
		{
			std::string value = reader->nodeValue().str();
			if (outer == &ncx::docAuthor_node)
			{
				aPrimaryMetadata.statement(aSubject, rdf::dc("creator"), rdf::literal(value));
			}
			else if (outer == &ncx::docTitle_node)
			{
				mTitle = value;
				aPrimaryMetadata.statement(aSubject, rdf::dc("title"), rdf::literal(value));
			}
			current = outer = nullptr;
		}
		break;
	case xml::reader::attribute:
		if (current == &ncx::meta_node && outer == &ncx::head_node)
		{
			if (reader->context() == &ncx::name_attr)
				name = reader->nodeValue().str();
			else if (reader->context() == &ncx::content_attr)
				content = reader->nodeValue().str();
		}
		break;
	case xml::reader::beginTagNode:
		current = reader->context();
		if (reader->context() == &ncx::head_node)
			outer = reader->context();
		else if (reader->context() == &ncx::docAuthor_node)
			outer = reader->context();
		else if (reader->context() == &ncx::docTitle_node)
			outer = reader->context();
		else if (reader->context() == &ncx::navMap_node)
			in_header = false;
		break;
	case xml::reader::endTagNode:
		if (reader->context() == &ncx::meta_node)
		{
			if (name.find("dtb:") == 0)
			{
				std::string meta = name.substr(4);
				if (meta == "depth" || meta == "totalPageCount" || meta == "maxPageNumber")
					aPrimaryMetadata.statement(aSubject, rdf::dtb(meta), rdf::literal(content, rdf::xsd("int")));
				else
					aPrimaryMetadata.statement(aSubject, rdf::dtb(meta), rdf::literal(content));
			}
		}
		else if (reader->context() == &ncx::head_node)
			outer = nullptr;
		current = nullptr;
		break;
	default:
		break;
	}

	aPrimaryMetadata.statement(aSubject, rdf::tts("mimetype"), rdf::literal("application/x-dtbncx+xml"));
}

std::shared_ptr<cainteoir::document_reader>
cainteoir::createNcxReader(const std::shared_ptr<xml::reader> &aReader,
                           const rdf::uri &aSubject,
                           rdf::graph &aPrimaryMetadata,
                           const std::string &aTitle,
                           const cainteoir::path &aBaseUri)
{
	return std::make_shared<ncx_document_reader>(aReader, aSubject, aPrimaryMetadata, aTitle, aBaseUri);
}
