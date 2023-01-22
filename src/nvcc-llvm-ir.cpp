// This wrapper simply converts --nvcc-llvm-ir-unopt and --nvcc-llvm-ir-opt
// arguments into CICC_MODIFY_UNOPT_MODULE=1 and CICC_MODIFY_OPT_MODULE=1
// env vars, respectively. We have to do it this way, because CMake does not
// support prepending compilers with environment variables. 

#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

static bool startsWith(string const &str, string const &start)
{
	if (str.length() >= start.length())
		return (str.compare(0, start.length(), start) == 0);
	
	return false;
}

static bool endsWith(string const &str, string const &ending)
{
	if (str.length() >= ending.length())
		return (str.compare(str.length() - ending.length(), ending.length(), ending) == 0);

	return false;
}

int main(int argc, char** argv)
{
	stringstream ss;

	// Do not do anything further, if we are not running nvcc.
	string progname = argv[1];
	if (endsWith(progname, "nvcc"))
	{
		string unopt, opt;
		for (int i = 1; i < argc; i++)
		{
			string arg = argv[i];
			if (startsWith(arg, "--nvcc-llvm-ir-unopt="))
			{
				unopt = arg.substr(string("--nvcc-llvm-ir-unopt=").length());
				continue;
			}
			if (startsWith(arg, "--nvcc-llvm-ir-opt="))
			{
				opt = arg.substr(string("--nvcc-llvm-ir-opt=").length());
				continue;
			}
		}

		// Cannot be both unopt and opt at the same time.
		if (unopt != "")
			ss << "CICC_MODIFY_UNOPT_MODULE=" << unopt << " ";
		else if (opt != "")
			ss << "CICC_MODIFY_OPT_MODULE=" << opt << " ";

		ss << "LD_PRELOAD=" << LIBNVCC << " " << argv[1];
	}

	for (int i = 2; i < argc; i++)
	{
		string arg = argv[i];
		if (startsWith(arg, "--nvcc-llvm-ir-unopt="))
			continue;
		if (startsWith(arg, "--nvcc-llvm-ir-opt="))
			continue;

		ss << " " << arg;
	}

	string cmd = ss.str();
	cout << cmd << endl;
	return system(cmd.c_str());
}

