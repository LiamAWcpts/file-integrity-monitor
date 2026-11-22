#include "baseline.h"
#include "hash.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;


Tbl scan(const std::string &dir, const std::string &skip1, const std::string &skip2){
  Tbl tbl;

  //full paths of baseline n log so it doesnt hash its own files
  std::string s1 = fs::weakly_canonical(skip1).string();
  std::string s2 = fs::weakly_canonical(skip2).string();

  //loop thru every file in the folder and sub folders
  for(const auto &ent : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)){
    if(!ent.is_regular_file()) continue;

    std::string full = fs::weakly_canonical(ent.path()).string();
    if(full==s1 || full==s2) continue;

    std::string h = hashFile(ent.path().string());
    if(h==""){
      std::cerr<<"cant read "<<ent.path().string()<<std::endl;
      continue;
    }

    //save it w the path relative to the folder
    std::string rel = fs::relative(ent.path(),dir).string();
    tbl[rel]=h;
  }
  return tbl;
}


bool saveBl(const Tbl &tbl, const std::string &blPath){
  std::ofstream out(blPath);
  if(!out.is_open()) return false;

  //write each line as hash then path
  for(const auto &it : tbl){
    out<<it.second<<" "<<it.first<<"\n";
  }
  return true;
}


bool loadBl(Tbl &tbl, const std::string &blPath){
  std::ifstream in(blPath);
  if(!in.is_open()) return false;

  std::string ln;
  while(std::getline(in,ln))
  {
    //hash is 64 chars then a space then the path
    if(ln.size()<66) continue;
    std::string h = ln.substr(0,64);
    std::string p = ln.substr(65);
    tbl[p]=h;
  }
  return true;
}
