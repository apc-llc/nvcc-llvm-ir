#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

static bool endsWith(string const &str, string const &ending)
{
	if (str.length() >= ending.length())
		return (str.compare(str.length() - ending.length(), ending.length(), ending) == 0);

	return false;
}

__attribute__((constructor)) static void activate(int argc, char** argv)
{
	// Do not do anything, if we are not running cicc.
	string progname = argv[0];
	if (!endsWith(progname, "cicc"))
		return;

	stringstream s;
	s << "LD_PRELOAD=./libcicc.so " << argv[0];
	for (int i = 1; i < argc; i++)
		s << " " << argv[i];

	string cmd = s.str();
	int result = system(cmd.c_str());
	exit(result);
}

