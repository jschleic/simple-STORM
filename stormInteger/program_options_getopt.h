/* Include file for parsing command line Options */
/* Project: Storm                                */

#include <getopt.h>
#include <iostream>
#include <map>
#include <vigra/impex.hxx>

class BadConversion : public std::runtime_error {
 public:
   BadConversion(std::string const& s)
     : std::runtime_error(s)
     { }
};
 
inline double convertToDouble(const char* const s);

void printUsage(const char* prog);

int parseProgramOptions(int argc, char **argv, std::map<char,float>& params, std::map<char,std::string>&files);
