// $Id$

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "CliExtension.hh"
#include "MSXConfig.hh"
#include "FileOperations.hh"
#include "FileContext.hh"


CliExtension::CliExtension()
{
	CommandLineParser::instance()->
		registerOption(std::string("-ext"), this);

	SystemFileContext context;
	const std::list<std::string> &paths = context.getPaths();
	std::list<std::string>::const_iterator it;
	for (it = paths.begin(); it != paths.end(); it++) {
		std::string path = FileOperations::expandTilde(*it);
		createExtensions(path + "share/extensions/");
	}
}

CliExtension::~CliExtension()
{
}

void CliExtension::parseOption(const std::string &option,
                               std::list<std::string> &cmdLine)
{
	std::string extension = getArgument(option, cmdLine);
	std::map<std::string, std::string>::const_iterator it =
		extensions.find(extension);
	if (it != extensions.end()) {
		MSXConfig *config = MSXConfig::instance();
		config->loadHardware(new SystemFileContext(), it->second);
	} else {
		PRT_ERROR("Extension \"" << extension << "\" not found!");
	}
}

const std::string& CliExtension::optionHelp() const
{
	static std::string help("Insert the extension specified in argument");
	return help;
}


int select(const struct dirent* d)
{
	struct stat s;
	// entry must be a directory
	if (stat(d->d_name, &s)) {
		return 0;
	}
	if (!S_ISDIR(s.st_mode)) {
		return 0;
	}

	// directory must contain the file "hardwareconfig.xml"
	std::string file(std::string(d->d_name) + "/hardwareconfig.xml");
	if (stat(file.c_str(), &s)) {
		return 0;
	}
	if (!S_ISREG(s.st_mode)) {
		return 0;
	}
	return 1;
}

void CliExtension::createExtensions(const std::string &path)
{
	char buf[PATH_MAX];
	if (!getcwd(buf, PATH_MAX)) {
		return;
	}
	if (chdir(path.c_str())) {
		return;
	}

	DIR* dir = opendir(".");
	if (dir) {
		struct dirent* d = readdir(dir);
		while (d) {
			if (select(d)) {
				std::string name(d->d_name);
				std::string path(path + name +
				                       "/hardwareconfig.xml");
				if (extensions.find(name) == extensions.end()) {
					// doesn't already exists
					extensions[name] = path;
				}
			}
			d = readdir(dir);
		}
		closedir(dir);
	}
	chdir(buf);
}
