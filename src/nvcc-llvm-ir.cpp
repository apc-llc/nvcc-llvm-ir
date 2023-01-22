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
	string progname = argv[0];
	if (endsWith(progname, "nvcc"))
	{
		bool unopt = false, opt = false;
		for (int i = 1; i < argc; i++)
		{
			string arg = argv[i];
			if (arg == "--nvcc-llvm-ir-unopt")
			{
				unopt = true;
				continue;
			}
			if (arg == "--nvcc-llvm-ir-opt")
			{
				opt = false;
				continue;
			}
		}

		// Cannot be both unopt and opt at the same time.
		if (unopt)
			ss << "CICC_MODIFY_UNOPT_MODULE=1 ";
		else if (opt)
			ss << "CICC_MODIFY_OPT_MODULE=1 ";

		ss << "LD_PRELOAD=" << LIBNVCC << " " << argv[0];
	}

	for (int i = 1; i < argc; i++)
	{
		string arg = argv[i];
		if (arg == "--nvcc-llvm-ir-unopt")
			continue;
		if (arg == "--nvcc-llvm-ir-opt")
			continue;

		ss << " " << arg;
	}

	string cmd = ss.str();
	return system(cmd.c_str());
}

