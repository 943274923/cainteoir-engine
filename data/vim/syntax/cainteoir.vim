" Vim syntax file
" Language:	Cainteoir Text-to-Speech Data Files
" Filenames:    *.phon, *.features, *.dict, *.ptp, *.voicedef, *.langdef
" Maintainer:	Reece H. Dunn <msclrhd@gmail.com>
" Last Change:	2015 Mar 9

" Quit when a (custom) syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

" Phoneme Features

syn keyword	cainteoirFeatUnused		ctl orl
syn keyword	cainteoirFeatConsonant		con vls vcd mrm
syn keyword	cainteoirFeatConsonant		blb lbd dnt alv pla rfx pal vel uvl phr glt
syn keyword	cainteoirFeatConsonant		alp lbp lbv epg
syn keyword	cainteoirFeatConsonant		nas stp frc sib apr trl flp lat clk imp
syn keyword	cainteoirFeatMain		dcz
syn keyword	cainteoirFeatDiacritic		ejc
syn keyword	cainteoirFeatVowel		vwl
syn keyword	cainteoirFeatVowel		unr rnd
syn keyword	cainteoirFeatVowel		fnt cnt bck
syn keyword	cainteoirFeatVowel		hgh smh umd mid lmd sml low
syn keyword	cainteoirFeatDiacritic		tie
syn keyword	cainteoirFeatSuprasegmental	pau sbr lnk fbr ibr glr glf ust dst
syn keyword	cainteoirFeatSuprasegmental	st1 st2 st3
syn keyword	cainteoirFeatSuprasegmental	est hlg lng
syn keyword	cainteoirFeatSuprasegmental	ts1 ts2 ts3 ts4 ts5
syn keyword	cainteoirFeatSuprasegmental	tm1 tm2 tm3 tm4 tm5
syn keyword	cainteoirFeatSuprasegmental	te1 te2 te3 te4 te5
syn keyword	cainteoirFeatDiacritic		syl nsy
syn keyword	cainteoirFeatDiacritic		unx
syn keyword	cainteoirFeatDiacritic		brv slv stv crv
syn keyword	cainteoirFeatDiacritic		dzd apc lmn lgl adv ret czd mcz rsd lwr
syn keyword	cainteoirFeatDiacritic		mrd lrd vfz nzd rzd atr rtr

syn match	cainteoirEscape			'\\.'
syn match	cainteoirUnicodeEscape		'\\u[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]'

syn match	cainteoirNumberSpecifier	"_[0-9]x"
syn match	cainteoirNumberSpecifier	"_10\^[0-9]*"
syn match	cainteoirNumberSpecifier	"_andDD"
syn match	cainteoirNumberSpecifier	"_DandDD"

syn region	cainteoirDirective		start='^\.[a-z]' end='$' contains=cainteoirComment,cainteoirString

syn region	cainteoirString			start="\"" end="\""

syn region	cainteoirComment		start="#" end="$" keepend contains=@Spell

syn region	cainteoirTranscription		start="/" end="/" contains=cainteoirFeatUnused,cainteoirFeatMain,cainteoirFeatConsonant,cainteoirFeatVowel,cainteoirFeatSuprasegmental,cainteoirFeatDiacritic

" Define the default highlighting.
" Only used when an item doesn't have highlighting yet

hi def link cainteoirComment			Comment
hi def link cainteoirTranscription		String
hi def link cainteoirFeatUnused			Constant
hi def link cainteoirFeatConsonant		cainteoirFeatMain
hi def link cainteoirFeatVowel			cainteoirFeatMain
hi def link cainteoirFeatMain			Type
hi def link cainteoirFeatSuprasegmental		Identifier
hi def link cainteoirFeatDiacritic		Identifier
hi def link cainteoirUnicodeEscape		Special
hi def link cainteoirEscape			Special
hi def link cainteoirDirective			PreProc
hi def link cainteoirString			String
hi def link cainteoirNumberSpecifier		Statement

let b:current_syntax = "cainteoir"
" vim: ts=8
