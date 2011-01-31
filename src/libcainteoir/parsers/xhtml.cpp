/* XHTML Document Parser.
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

namespace rdf = cainteoir::rdf;
namespace xml = cainteoir::xmldom;

void parseXHtmlHead(const xml::node &html, const rdf::uri &subject, cainteoir::document_events &events, std::string &title)
{
	for (xml::node node = html.firstChild(); node.isValid(); node.next())
	{
		if (node.type() == XML_ELEMENT_NODE)
		{
			if (node == rdf::xhtml("title"))
				title = node.content()->str();
		}
	}
}

void parseXHtmlBody(const xml::node &html, const rdf::uri &subject, cainteoir::document_events &events)
{
	for (xml::node node = html.firstChild(); node.isValid(); node.next())
	{
		if (node.type() == XML_ELEMENT_NODE)
		{
			if (node == rdf::xhtml("p") || node == rdf::xhtml("div"))
			{
				events.text(node.content());
			}
			else if (node == rdf::xhtml("h1") ||
			         node == rdf::xhtml("h2") ||
			         node == rdf::xhtml("h3") ||
			         node == rdf::xhtml("h4") ||
			         node == rdf::xhtml("h5") ||
			         node == rdf::xhtml("h6"))
			{
				events.text(node.content());
			}
		}
	}
}

void cainteoir::parseXHtmlDocument(const xml::node &html, const rdf::uri &subject, cainteoir::document_events &events)
{
	if (html != rdf::xhtml("html"))
		throw std::runtime_error("XHTML document is not of a recognised format.");

	std::string title;

	for (xml::node node = html.firstChild(); node.isValid(); node.next())
	{
		if (node.type() == XML_ELEMENT_NODE)
		{
			if (node == rdf::xhtml("head"))
				parseXHtmlHead(node, subject, events, title);
			else if (node == rdf::xhtml("body"))
				parseXHtmlBody(node, subject, events);
		}
	}

	if (!title.empty())
		events.metadata(rdf::statement(subject, rdf::dc("title"), rdf::literal(title)));

	for (xml::attribute attr = html.firstAttribute(); attr.isValid(); attr.next())
	{
		if (attr == rdf::xml("lang") || attr == rdf::uri(std::string(), "lang"))
			events.metadata(rdf::statement(subject, rdf::dc("language"), rdf::literal(attr.content())));
	}
}
