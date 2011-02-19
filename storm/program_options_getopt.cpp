/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*      Copyright 2010-2011 by Joachim Schleicher and Ullrich Koethe    */
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


#include "program_options_getopt.h"
#include "configVersion.hxx"
 
inline double convertToDouble(const char* const s) {
   std::istringstream i(s);
   double x;
   if (!(i >> x))
     throw BadConversion("convertToDouble(\"\")");
   return x;
} 

void printUsage(const char* prog) {
	std::cout << "usage: " << prog << " [Options] infile.sif [outfile.png]" << std::endl 
	 << "Allowed Options: " << std::endl 
	 << "    --help           Print this help message" << std::endl 
	 <<	"    --verbose        verbose message output" << std::endl
	 << "    --factor=Arg     Resize factor equivalent to the subpixel-precision" << std::endl 
	 << "    --threshold=Arg  Threshold for background suppression" << std::endl 
	 << "    --coordsfile=Arg filename for output of the found Coordinates" << std::endl 
	 << "    --filter=Arg     tif input for filtering in fft domain" << std::endl 
	 << "                     instead of generating the filter from the data" << std::endl
	 << "    --roi-len=Arg    size of the roi around maxima candidates" << std::endl 
	 << "    --frames=Arg     run only on a subset of the stack (frames=start:end)" << std::endl 
	 << "    --version        print version information and exit" << std::endl 
	 ;
}

int parseProgramOptions(int argc, char **argv, std::map<char,double>& params, std::map<char,std::string>&files)
{
	int c;
	int digit_optind = 0;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"help",     no_argument, 0,  '?' },
			{"verbose",     no_argument, 0,  'v' },
			{"version",     no_argument, 0,  'V' },
			{"factor",  required_argument,       0,  'g' },
			{"threshold",  required_argument, 0,  't' },
			{"coordsfile",    required_argument, 0,  'c'},
			{"filter",    required_argument, 0,  'f' },
			{"roi-len",    required_argument, 0,  'm' },
			{"frames",    required_argument, 0,  'F' },
			{0,         0,                 0,  0 }

		};

		c = getopt_long(argc, argv, "c:t:v?",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 't': // threshold
		case 'g': // factor
		case 'm': // roi-len
			params[c] = convertToDouble(optarg);
			break;
			
		case 'c': // coordsfile
		case 'f': // filter
		case 'F': // frames
			files[c] = optarg;
			break;

		case 'v':
			params['v'] = 1; // verbose mode
			break;
			
		case '?':
			printUsage(argv[0]);
			return -1;
			break;

		case 'V':
			std::cout << "STORM evaluation software version " << versionString() << std::endl
			 << "Copyright (C) 2011 Joachim Schleicher and Ullrich Koethe" << std::endl
			 << "This is free software; see the source for copying conditions.  There is NO" << std::endl
			 << "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." << std::endl
			 ;			
			return -1;
			break;

		default:
			std::cout << "?? getopt returned character code 0%o ??\n" << std::endl;
		}
	}

	while (optind < argc) {
		if(files['i'] == "") files['i'] = argv[optind++];
		else if (files['o'] == "") files['o'] = argv[optind++];
		else std::cout << "unrecognized non-option Argument: " << argv[optind++] << std::endl;
	}
	if(files['i'] == "" ) {
		printUsage(argv[0]);
		return -1;
	}
	
	return 0;
}
