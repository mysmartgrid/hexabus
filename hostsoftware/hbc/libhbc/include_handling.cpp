#include "include_handling.hpp"

#include <boost/foreach.hpp>
#include <iostream>

using namespace hexabus;

IncludeHandling::IncludeHandling(std::string main_file_name) {
  // add the file the user specified, more files will be added when "include"s are parsed
  if(fs::path(main_file_name).has_root_directory()) {
    filenames.push_back(fs::path(main_file_name)); // if it's a full path (starting with a root), add it as it is
  } else {
    filenames.push_back(fs::initial_path() / fs::path(main_file_name)); // if it's a relative path, put our working directory in front of it.
  }

  base_dir = filenames[0].branch_path();
}

fs::path IncludeHandling::getFileListElement(unsigned int index) {
  return filenames[index];
}

unsigned int IncludeHandling::getFileListLength() {
  return filenames.size();
}

void IncludeHandling::addFileName(include_doc incl) {
  // make boost::filesystem::path object
  const fs::path includepath(incl.filename); // the path given in the include statement
  fs::path file_to_add; // here the path that is actually added is stored!

  if(includepath.has_root_directory()) { // If the path in the include is an absolute path, just look there!
    file_to_add = includepath;
    // check whether file exists
    if(!fs::exists(file_to_add)) {
      std::ostringstream oss;
      oss << "[" << incl.read_from_file << ":" << incl.lineno << "] Include file not found: " << incl.filename << std::endl;
      throw FileNotFoundException(oss.str());
    }
  } else {
    // ascend in the directory tree to find the file
    fs::path search_dir = base_dir;
    file_to_add = search_dir / includepath;
    while(!fs::exists(file_to_add) && search_dir != base_dir.root_path()) { // ascend until file is found OR we reach root.
      search_dir /= fs::path(".."); // append ".."
      search_dir = search_dir.normalize(); // normalize to turn the path ending in /.. into the parent directory.
      file_to_add = search_dir / includepath;
    }

    if(search_dir == base_dir.root_path() && !exists(file_to_add)) {
      // This means we searched up to the root dir, and didn't find the file.
      std::ostringstream oss;
      oss << "[" << incl.read_from_file << ":" << incl.lineno << "] Include file not found: " << incl.filename << std::endl;
      throw FileNotFoundException(oss.str());
    }
    // TODO catch filename too long exception if the while -- for whatever reason -- gets stuck in an infinite loop
  }

  // only add if filename does not already exist
  bool exists = false;
  BOOST_FOREACH(fs::path fn, filenames) {
    if(fn == file_to_add)
      exists = true;
  }
  if(!exists) {
    std::cout << "Adding " << file_to_add.string() << " to list of files to parse. - from include in " << incl.read_from_file << " line " << incl.lineno << std::endl;
    filenames.push_back(file_to_add);
  }
}
