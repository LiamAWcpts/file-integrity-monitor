#ifndef BASELINE_H
#define BASELINE_H
#include <map>
#include <string>

//path -> hash
typedef std::map<std::string,std::string> Tbl;

//goes thru the folder and hashes everything
Tbl scan(const std::string &dir, const std::string &skip1, const std::string &skip2);

bool saveBl(const Tbl &tbl, const std::string &blPath);
bool loadBl(Tbl &tbl, const std::string &blPath);

#endif
    