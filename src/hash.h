#ifndef HASH_H
#define HASH_H
#include <string>

//gets the sha256 of a file, gives back "" if it cant open it
std::string hashFile(const std::string &fp);

#endif
