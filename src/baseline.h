#ifndef BASELINE_H
#define BASELINE_H
#include <map>
#include <string>
#include <vector>

//path -> hash
typedef std::map<std::string,std::string> Tbl;

//list of stuff to skip from -x
typedef std::vector<std::string> ExList;

//true when -q is used, its in main.cpp
extern bool quiet;

//goes thru the folder and hashes everything
Tbl scan(const std::string &dir, const std::string &skip1, const std::string &skip2, const ExList &ex);

//true if the path matches one of the -x patterns
bool isExcluded(const std::string &rel, const ExList &ex);

bool saveBl(const Tbl &tbl, const std::string &blPath);
bool loadBl(Tbl &tbl, const std::string &blPath);

#endif
    