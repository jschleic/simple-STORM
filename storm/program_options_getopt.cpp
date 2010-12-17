
#include "program_options_getopt.h"
 
inline double convertToDouble(const char* const s) {
   std::istringstream i(s);
   double x;
   if (!(i >> x))
     throw BadConversion("convertToDouble(\"\")");
   return x;
} 

void printUsage(const char* prog) {
	std::cout << "usage: " << prog << " [Options] infile.sif outfile.png" << std::endl 
	 << "Allowed Options: " << std::endl 
	 << "    --help           Print this help message" << std::endl 
	 << "    --factor=Arg     Resize factor equivalent to the subpixel-precision" << std::endl 
	 << "    --threshold=Arg  Threshold for background suppression" << std::endl 
	 << "    --coordsfile=Arg filename for output of the found Coordinates" << std::endl 
	 << "    --filter=Arg     tif input for filtering in fft domain" << std::endl 
	 << "                     instead of generating the filter from the data" << std::endl
	 ;
}

int parseProgramOptions(int argc, char **argv, std::map<char,float>& params, std::map<char,std::string>&files)
{
	int c;
	int digit_optind = 0;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"help",     no_argument, 0,  '?' },
			{"factor",  required_argument,       0,  'g' },
			{"threshold",  required_argument, 0,  't' },
			{"coordsfile",    required_argument, 0,  'c'},
			{"filter",    required_argument, 0,  'f' },
			{0,         0,                 0,  0 }

		};

		c = getopt_long(argc, argv, "c:t:v?",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 't':
		case 'g':
			params[c] = convertToDouble(optarg);
			break;
			
		case 'c':
		case 'f':
			files[c] = optarg;
			break;

		case 'v':
			params['v'] = 1; // verbose mode
			break;
			
		case '?':
			printUsage(argv[0]);
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
	if(files['i'] == "" || files['o'] == "") {
		printUsage(argv[0]);
		return -1;
	}
	
	return 0;
}
