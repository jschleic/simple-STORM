/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*      Copyright 2010-2011 by Joachim Schleicher                       */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/
 

#ifndef UTIL_HXX
#define UTIL_HXX

// private helper functions
namespace helper {
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

class BadConversion : public std::runtime_error {
 public:
   BadConversion(std::string const& s)
     : std::runtime_error(s)
     { }
};
 
inline double convertToDouble(std::string const& s) {
   std::istringstream i(s);
   double x;
   if (!(i >> x))
     throw BadConversion("convertToDouble(\"" + s + "\")");
   return x;
} 

inline int convertToInt(std::string const& s) {
   std::istringstream i(s);
   int x;
   if (!(i >> x))
     throw BadConversion("convertToDouble(\"" + s + "\")");
   return x;
} 

/**
 * Take python like range string and split it into start:end:stride
 * @return true if successful, false on errors
 */
bool rangeSplit(const std::string &r, int &beg, int &end, unsigned int &stride) {
	size_t c1 = r.find(':');
	size_t c2 = r.find(':',c1+1);
	size_t c3 = r.find(':',c2+1);
	
	// exactly one or two colons needed
	if(c1==std::string::npos || (c2!=std::string::npos && c3!=std::string::npos)) {
		return false;
	}
	try { // beg
		beg = helper::convertToInt(r.substr(0,c1));
	} catch (helper::BadConversion & e) { // dont change value, if its not a number
		;
	}
	try { // end
		end = helper::convertToInt(r.substr(c1+1,c2-c1-1));
	} catch (helper::BadConversion & e) { // dont change value, if its not a number
		;
	}
	if(c2!=std::string::npos) {
		try { // stride
			stride = helper::convertToInt(r.substr(c2+1));
		} catch (helper::BadConversion & e) { // dont change value, if its not a number
			;
		}
	}
	return true;
}

}// namespace helper

#endif // UTIL_HXX
