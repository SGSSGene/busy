#include <iostream>
//#define _GNU_SOURCE
#include <link.h>
#include <dlfcn.h>

#include <busyPluginsBase/Base.h>
void printNames() {
		auto names = genericFactory::getClassList<Base>();
		std::cout << "list of names: " << names.size() << "\n";
		for (auto const& n : names) {
			std::cout << n << "\n";
		}
}

int main() {
	printNames();
	{
		auto handle = dlopen("build/system-gcc/debug/pluginbusyPlugins1.so", RTLD_NOW | RTLD_LOCAL);
		printNames();

		dlclose(handle);
		std::cout << "handle: " << handle << std::endl;
	}
	printNames();
	{
		auto h1 = dlopen("build/system-gcc/debug/pluginbusyPlugins1.so", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
		auto h2 = dlopen("build/system-gcc/debug/pluginbusyPlugins2.so", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);

//		auto h1 = dlmopen(LM_ID_NEWLM, "build/system-gcc/debug/pluginbusyPlugins.so", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
//		auto h2 = dlmopen(LM_ID_NEWLM, "build/system-gcc/debug/pluginbusyPlugins.so", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
		printNames();

		std::cout << "handle: " << h1 << std::endl;
		std::cout << "handle: " << h2 << std::endl;

		dlclose(h2);
		dlclose(h1);
	}

	return EXIT_SUCCESS;
}
