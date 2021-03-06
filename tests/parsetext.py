#!/usr/bin/python

# Copyright (C) 2012-2014 Reece H. Dunn
#
# This file is part of cainteoir-engine.
#
# cainteoir-engine is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cainteoir-engine is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cainteoir-engine.  If not, see <http://www.gnu.org/licenses/>.

import harness
import sys

if __name__ == '__main__':
	test = harness.TestSuite('parsetext', sys.argv)
	test.run({'name': 'Parse Text', 'groups': [
		{'name': 'Latin', 'type': 'parsetext', 'tests': [
			{'test': 'parse-text/latin/words.txt', 'result': 'parse-text/latin/words.pt'},
			{'test': 'parse-text/latin/numbers.txt', 'result': 'parse-text/latin/numbers.pt'},
			{'test': 'parse-text/latin/punctuation.txt', 'result': 'parse-text/latin/punctuation.pt'},
			{'test': 'parse-text/latin/hyphens.txt', 'result': 'parse-text/latin/hyphens.pt'},
			{'test': 'parse-text/latin/shy-hyphens.txt', 'result': 'parse-text/latin/shy-hyphens.pt'},
			{'test': 'parse-text/latin/quotes.txt', 'result': 'parse-text/latin/quotes.pt'},
			{'test': 'parse-text/latin/symbols.txt', 'result': 'parse-text/latin/symbols.pt'},
			{'test': 'parse-text/latin/paragraph.txt', 'result': 'parse-text/latin/paragraph.pt'},
			{'test': 'parse-text/latin/match-limit.txt', 'result': 'parse-text/latin/match-limit.pt'},
			{'test': 'parse-text/latin/no-newline-at-end.txt', 'result': 'parse-text/latin/no-newline-at-end.pt'},
			{'test': 'parse-text/latin/accented.txt', 'result': 'parse-text/latin/accented.pt'},
		]},
		{'name': 'Japanese', 'type': 'parsetext', 'tests': [
			{'test': 'parse-text/japanese/words.txt', 'result': 'parse-text/japanese/words.pt'},
			{'test': 'parse-text/japanese/mixed-script.txt', 'result': 'parse-text/japanese/mixed-script.pt'},
		]},
		{'name': 'HTML', 'type': 'parsetext', 'tests': [
			{'test': 'parse-text/html/block-first-empty.html', 'result': 'parse-text/html/block-first-empty.pt'},
			{'test': 'parse-text/html/block-nested.html', 'result': 'parse-text/html/block-nested.pt'},
			{'test': 'parse-text/html/paragraph.html', 'result': 'parse-text/html/paragraph.pt'},
			{'test': 'parse-text/html/paragraph-singleword.html', 'result': 'parse-text/html/paragraph-singleword.pt'},
			{'test': 'parse-text/html/msword.html', 'result': 'parse-text/html/msword.pt'},
			{'test': 'parse-text/html/split-word.html', 'result': 'parse-text/html/split-word.pt'},
			{'test': 'parse-text/html/ordinal.html', 'result': 'parse-text/html/ordinal.pt'},
			{'test': 'parse-text/html/br-after-full-stop.html', 'result': 'parse-text/html/br-after-full-stop.pt'},
			{'test': 'parse-text/html/ol.html', 'result': 'parse-text/html/ol.pt'},
		]},
	]})
	test.summary()
