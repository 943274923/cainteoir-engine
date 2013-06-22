/* Metadata Extractor.
 *
 * Copyright (C) 2010-2013 Reece H. Dunn
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

#include <cainteoir/metadata.hpp>
#include <cainteoir/audio.hpp>
#include <cainteoir/document.hpp>
#include <cainteoir/stopwatch.hpp>
#include <stdexcept>
#include <iostream>
#include <cstdio>
#include <getopt.h>

namespace rdf = cainteoir::rdf;

enum args
{
	ARG_HELP = 'h',
	ARG_NTRIPLE = 300,
	ARG_TIME = 't',
	ARG_TURTLE = 301,
	ARG_VORBIS_COMMENTS = 302,
};

const char *options_short = "ht";

static struct option options[] =
{
	{ "ntriple", no_argument, 0, ARG_NTRIPLE },
	{ "turtle",  no_argument, 0, ARG_TURTLE },
	{ "vorbis",  no_argument, 0, ARG_VORBIS_COMMENTS },
	{ "help",    no_argument, 0, ARG_HELP },
	{ "time",    no_argument, 0, ARG_TIME },
	{ 0, 0, 0, 0 }
};

void help()
{
	fprintf(stdout, i18n("usage: metadata [OPTION..] document..\n"));
	fprintf(stdout, "\n");
	fprintf(stdout, i18n("Formats:\n"));
	fprintf(stdout, i18n(" --ntriple              Output RDF N-Triples statements\n"));
	fprintf(stdout, i18n(" --turtle               Output RDF Turtle statements\n"));
	fprintf(stdout, i18n(" --vorbis               Output VorbisComment entries\n"));
	fprintf(stdout, "\n");
	fprintf(stdout, i18n("General:\n"));
	fprintf(stdout, i18n(" -h, --help             This help text\n"));
	fprintf(stdout, i18n(" -t, --time             Time how long it takes to extract the metadata\n"));
	fprintf(stdout, "\n");
	fprintf(stdout, i18n("Report bugs to msclrhd@gmail.com\n"));
}

int main(int argc, char ** argv)
{
	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	try
	{
		enum
		{
			rdf_metadata,
			vorbis_comments,
		} output_type = rdf_metadata;

		rdf::formatter::format_type format = rdf::formatter::ntriple;
		bool print_time = false;

		while (1)
		{
			int option_index = 0;
			int c = getopt_long(argc, argv, options_short, options, &option_index);
			if (c == -1)
				break;

			switch (c)
			{
			case ARG_NTRIPLE:
				output_type = rdf_metadata;
				format = rdf::formatter::ntriple;
				break;
			case ARG_TURTLE:
				output_type = rdf_metadata;
				format = rdf::formatter::turtle;
				break;
			case ARG_VORBIS_COMMENTS:
				output_type = vorbis_comments;
				break;
			case ARG_HELP:
				help();
				return 0;
			case ARG_TIME:
				print_time = true;
				break;
			}
		}

		argc -= optind;
		argv += optind;

		cainteoir::stopwatch timer;
		rdf::graph metadata;

		if (argc == 0)
			cainteoir::createDocumentReader(nullptr, metadata, std::string());
		else for(int i = 0; i < argc; ++i)
		{
			auto reader = cainteoir::createDocumentReader(argv[i], metadata, std::string());
			if (!reader)
				fprintf(stderr, i18n("unsupported document format for file \"%s\"\n"), argv[i]);
		}

		if (print_time)
			fprintf(stderr, "Extraction Time: %G\n", timer.elapsed());

		if (!metadata.empty())
		{
			if (output_type == rdf_metadata)
			{
				(*rdf::create_formatter(std::cout, format))
					<< rdf::rdf
					<< rdf::rdfa
					<< rdf::rdfs
					<< rdf::xsd
					<< rdf::xml
					<< rdf::owl
					<< rdf::dc
					<< rdf::dcterms
					<< rdf::dcam
					<< rdf::epub
					<< rdf::opf
					<< rdf::ocf
					<< rdf::pkg
					<< rdf::media
					<< rdf::onix
					<< rdf::marc
					<< rdf::ncx
					<< rdf::dtb
					<< rdf::smil
					<< rdf::xhtml
					<< rdf::skos
					<< rdf::foaf
					<< rdf::tts
					<< rdf::iana
					<< rdf::subtag
					<< metadata;
			}
			else
			{
				const rdf::uri subject = rdf::uri(argv[0], std::string());
				for (auto &comment : cainteoir::vorbis_comments(metadata, subject))
					std::cout << comment.label << "=" << comment.value << std::endl;
			}
		}
	}
	catch (std::runtime_error &e)
	{
		fprintf(stderr, i18n("error: %s\n"), e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
