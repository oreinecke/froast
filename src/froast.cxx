// Copyright (C) 2011 Oliver Schulz <oliver.schulz@tu-dortmund.de>

// This is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

#include <TROOT.h>
#include <TFile.h>
#include <THashList.h>

#include "../config.h"

#include "util.h"
#include "Selector.h"
#include "Settings.h"
#include "JSON.h"


/*!	\mainpage	Programme to evaluate CPG pulse shape data
	*
	* Syntax: froast COMMAND ... \n
	* Commands: 
	*	- settings [-j] INPUTFILE
	*	- map-single MAPPERS OUTPUTFILE INPUT_FILE
	*	- map-multi MAPPERS OUTPUTFILE_TAG INPUTFILES
	* - reduce MAPPERS OUTPUTFILE INPUTFILES
	*	- tabulate ROOT_FILE/TREENAME VAREXP [SELECTION [NENTRIES [STARTENTRY]]]
	*
	* With the settings option the settings from processed ROOT files or .rootrc can be read
	* out and with the -j option converted to json (output on the screen). \n
	* With the map-single option one input file can be processed. See froast::Selector::mapSingle for
	* more information on the input parameters. \n
	* With the map-multi option several input files can be processed. See froast::Selector::mapMulti for
	* more information on the input parameters. \n
	* With the reduce option several input files can be combined. See froast::Selector::reduce for
	* more information on the input parameters. \n
*/

using namespace std;
using namespace froast;



int settings(int argc, char *argv[], char *envp[]) {
	
	
	int nargs = argc-1;
	char **args = argv+1;

	bool jsonOutput = false;
	if ((nargs >= 1) && (string(args[0]) == "-j")) {
		--nargs; ++args;
		jsonOutput = true;
	}

	if (nargs == 0) {
		if (jsonOutput) {
			THashList *nested = Settings::global().exportNested();
			JSON::write(cout, nested) << endl;
			delete nested;
		} else Settings::global().write(cout);
		return 0;
	} else if (nargs == 1) {
		Settings settings;
		TString inFileName(args[0]);
		TString objSpecSep(".root/");
		if (inFileName.EndsWith(".root") || inFileName.Contains(objSpecSep)) {
			TString rootFileName = inFileName;
			TString settingsName = "";
			int idx = inFileName.Index(objSpecSep);
			if (idx >= 0) {
				rootFileName = inFileName(0, idx + objSpecSep.Length() - 1);
				settingsName = inFileName(idx + objSpecSep.Length(), inFileName.Length() - idx - objSpecSep.Length());
			}
			TFile inFile(rootFileName.Data(), "read");
			if (settingsName.Length() > 0) settings.read(&inFile, settingsName.Data());
			else settings.read(&inFile);
		} else if (inFileName.EndsWith(".rootrc")) {
			settings.read(inFileName);
		} else if (inFileName=="-" && jsonOutput) {
			// yes this isn't super-safe for instance on ./stdin but who would name a file 'stdin'????
			settings.read(TString("/dev/stdin"));
		} else if (inFileName.EndsWith(".json") || inFileName=="-" && !jsonOutput) {
			ifstream in(inFileName=="-"?"/dev/stdin":inFileName.Data());
			THashList *nested = JSON::read(in);
			settings.importNested(nested);
			delete nested;
		} else {
			cerr << "Unknown file extension, can't read settings from \"" << inFileName << "\"." << endl;
			return 1;
		}
		if (jsonOutput) {
			THashList *nested = settings.exportNested();
			JSON::write(cout, nested) << endl;
			delete nested;
		} else settings.write(cout);
		return 0;
	} else {
		cerr << "Syntax: " << args[0] << " [-j] ROOT_FILE SETTINGS" << endl;
		return 1;
	}
}


int map_single(int argc, char *argv[], char *envp[]) {
	if (argc != 4) {
		cerr << "Syntax: " << argv[0] << " MAPPERS OUTPUT_FILE INPUT_FILE" << endl;
		return 1;
	}
	string mappers = argv[1];
	string outFileName = argv[2];
	string inFileName = argv[3];
	Selector::mapSingle(inFileName, mappers, outFileName);

	return 0;
}


int map_multi(int argc, char *argv[], char *envp[]) {
	const size_t firstInputArg = 3;
	if (argc <= firstInputArg) {
		cerr << "Syntax: " << argv[0] << " MAPPERS TAG [INPUT]..." << endl;
		return 1;
	}
	string mappers = argv[1];
	string tag = argv[2];
	for (size_t arg = firstInputArg; arg < argc; ++arg) {
		// -> inputfilename(s), mappers, filename extension tag, bool noRecompile
		// By default the selector is compiled (++ ROOT compile option).
		// If there are more than one inputfile, the selector is not recompiled
		Selector::mapMulti(argv[arg], mappers, tag, arg > firstInputArg);
	}

	return 0;
}

int reduce(int argc, char *argv[], char *envp[]) {
	const size_t firstInputArg = 3;
	if (argc <= firstInputArg) {
		cerr << "Syntax: " << argv[0] << " MAPPERS OUTPUT_FILE [INPUT]..." << endl;
		return 1;
	}
	string mappers = argv[1];
	string outFileName = argv[2];
	string inFiles = argv[firstInputArg];
	for (size_t arg = firstInputArg+1; arg < argc; ++arg) {
		inFiles+=" ";
		inFiles+=argv[arg];
	}
	Selector::reduce(inFiles, mappers, outFileName);
	return 0;
}

int tabulate(int argc, char *argv[], char *envp[]) {
	int nargs = argc-1;
	char **args = argv+1;

	if (nargs < 2) {
		cerr << "Syntax: " << argv[0] << " [-j] ROOT_FILE/TREENAME VAREXP [SELECTION [NENTRIES [STARTENTRY]]]" << endl;
		return 1;
	}

	TString input(args[0]);
	ssize_t splitPos = input.Last('/');
	if ((splitPos < 0) || (splitPos >= input.Length()-1)) {
		cerr << "Error: No tree name specified."  << endl;
		return 1;
	}
	TString inFileName = input(0, splitPos);
	TString treeName = input(splitPos + 1, input.Length() - splitPos - 1);
	TChain chain(treeName);

	if (inFileName.First('*')>=inFileName.Last('/'))
		chain.Add(inFileName);
	// this should assist the TChain with its retarded shell expansion
	else {
		TObjArray &files=*chain.GetListOfFiles();
		chain.Add((TString)inFileName(0,splitPos=inFileName.First('/')));
		int length=files.GetEntries();
		inFileName.Remove(0,splitPos+1);
		while (inFileName.Length()>0 && length>0) {
			splitPos=inFileName.First('/');
			if (splitPos<0) splitPos=inFileName.Length();
			for (int entry=0;entry<length;entry++)
				chain.Add(TString(files[entry]->GetTitle())+"/"+inFileName(0,splitPos));
			files.RemoveRange(0,length-1);
			files.Compress();
			length=files.GetEntries();
			inFileName.Remove(0,splitPos+1);
		}
		// it seems that the operation above messes up chain's file list, so let's just rebuild it here:
		TObjArray a(*(TObjArray*)files.Clone()); chain.Reset();
		for (int entry=0;entry<length;entry++) chain.Add(a[entry]->GetTitle());
	}


	TString varexp(args[1]);
	const TString selection = (nargs > 2) ? args[2] : "";
	ssize_t nEntries = (nargs > 3) ? atol(args[3]) : -1;
	ssize_t startEntry = (nargs > 4) ? atol(args[4]) : 0;
	Selector::tabulate(&chain, cout, varexp, selection, nEntries, startEntry);
	
	return 0;
}


int main(int argc, char *argv[], char *envp[]) {
	try {
		// Have to tell ROOT to load vector dlls, otherwise ROOT will produce
		// "is not of a class known to ROOT" errors on creation of STL vector
		// branches:
		gROOT->ProcessLine("#include <vector>");
		
		gSystem->SetProgname(PACKAGE_TARNAME);
		
		string progName(argv[0]);

		if (argc < 2) {
			cerr << "Syntax: " << progName << " COMMAND ..." << endl << endl;
			cerr << "Commands: " << endl;
			cerr << "  settings" << endl;
			cerr << "  map-single" << endl;
			cerr << "  map-multi" << endl;
			cerr << "  reduce" << endl;
			cerr << "  tabulate" << endl;
			return 1;
		}
		
		string cmd(argv[1]);

		int cmd_argc = argc - 1;
		char **cmd_argv = argv + 1;

		if (cmd == "settings") return settings(cmd_argc, cmd_argv, envp);
		else if (cmd == "map-single") return map_single(cmd_argc, cmd_argv, envp);
		else if (cmd == "map-multi") return map_multi(cmd_argc, cmd_argv, envp);
		else if (cmd == "reduce") return reduce(cmd_argc, cmd_argv, envp);
		else if (cmd == "tabulate") return tabulate(cmd_argc, cmd_argv, envp);
		else {
			cerr << "ERROR: " << progName << " does not support command \"" << cmd << "\"" << endl;
		}
	}
	catch(std::exception &e) {
		cerr << endl << endl << "Exception: " << e.what() << endl << endl;

		return 1;
	}
}
