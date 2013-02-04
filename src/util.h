// Copyright (C) 2011 Oliver Schulz <oliver.schulz@tu-dortmund.de>

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.


#ifndef FROAST_UTIL_H
#define FROAST_UTIL_H

#include <stdexcept>
#include <string>
#include <iostream>
#include <limits>
#include <stdint.h>


class THashList;
class TDirectory;


namespace froast {



class Settings {
public:
	static bool get(const char* name, bool dflt, bool saveDflt = true);
	static int32_t get(const char* name, int32_t dflt, bool saveDflt = true);
	static double get(const char* name, double dflt, bool saveDflt = true);
	static const char* get(const char* name, const char* dflt, bool saveDflt = true);

	static std::ostream& write(THashList *settings, std::ostream &out);

	static std::ostream& write(std::ostream &out);

	static void writeToGDirectory();

	static THashList* getFrom(TDirectory *tdir);

	static THashList* getCurrent();

	static std::string toString();
};



class Util {
public:
	inline static double floatNaN() { return std::numeric_limits<float>::quiet_NaN(); }
	inline static double doubleNaN() { return std::numeric_limits<double>::quiet_NaN(); }
};


} // namespace froast


#ifdef __CINT__
#pragma link C++ class froast::Settings-;
#pragma link C++ class froast::Util-;
#endif


#endif // FROAST_UTIL_H
