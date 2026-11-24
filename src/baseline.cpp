#include "baseline.h"
#include "hash.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;


bool isExcluded(const std::string &rel, const ExList &ex){
  for(const auto &pat : ex){
    if(pat.empty()) continue;

    //ends w a / so its a folder, skip everything inside it
    if(pat.back()=='/'){
      if(rel.compare(0,pat.size(),pat)==0) return true;
    }
    //starts w * like *.log so just check the end of the path
    else if(pat[0]=='*' && pat.size()>1){
      std::string end = pat.substr(1);
      if(rel.size()>=end.size() && rel.compare(rel.size()-end.size(),end.size(),end)==0) return true;
    }
    //anything else has to match the whole path
    else if(rel==pat) return true;
  }
  return false;
}


//does one folder and then calls itself for the folders inside it
//used to use recursive_directory_iterator but if one folder errors (like it got deleted mid scan)
//that just throws and crashes, or stops the whole scan so every file after it looks DELETED
//doing it by hand means a bad folder only skips that folder
void scanDir(const fs::path &cur, const std::string &relDir, const std::string &s1, const std::string &s2, const ExList &ex, Tbl &tbl){
  std::error_code ec;
  fs::directory_iterator it(cur, fs::directory_options::skip_permission_denied, ec);
  if(ec){
    std::cerr<<"cant open folder "<<cur.string()<<" ("<<ec.message()<<")"<<std::endl;
    return;
  }

  for(; it!=fs::directory_iterator(); it.increment(ec)){
    const fs::directory_entry &ent = *it;
    std::error_code ec2;

    //path relative to the folder being watched, like sub/new.txt
    std::string name = ent.path().filename().string();
    std::string rel = relDir=="" ? name : relDir + "/" + name;

    //go into sub folders but dont follow folder symlinks so it cant loop forever
    bool isDir = ent.is_directory(ec2);
    bool isLink = ent.is_symlink(ec2);
    if(isDir && !isLink){
      if(isExcluded(rel,ex) || isExcluded(rel + "/",ex)) continue;
      scanDir(ent.path(),rel,s1,s2,ex,tbl);
      continue;
    }

    if(!ent.is_regular_file(ec2)) continue;
    if(isExcluded(rel,ex)) continue;

    std::string full = fs::weakly_canonical(ent.path(),ec2).string();
    if(full==s1 || full==s2) continue;

    std::string h = hashFile(ent.path().string());
    if(h==""){
      std::cerr<<"cant read "<<ent.path().string()<<std::endl;
    }

    //still save it even if it couldnt be read (hash is just "") so check can say CANTREAD instead of DELETED
    tbl[rel]=h;
  }

  //if moving to the next file broke, the loop just ends so say so
  if(ec) std::cerr<<"stopped reading "<<cur.string()<<" early ("<<ec.message()<<")"<<std::endl;
}


Tbl scan(const std::string &dir, const std::string &skip1, const std::string &skip2, const ExList &ex){
  Tbl tbl;
  std::error_code ec;

  //full paths of baseline n log so it doesnt hash its own files
  std::string s1 = fs::weakly_canonical(skip1,ec).string();
  std::string s2 = fs::weakly_canonical(skip2,ec).string();

  //loop thru every file in the folder and sub folders
  scanDir(dir,"",s1,s2,ex,tbl);
  return tbl;
}


bool saveBl(const Tbl &tbl, const std::string &blPath){
  std::ofstream out(blPath);
  if(!out.is_open()) return false;

  //write each line as hash then path
  for(const auto &it : tbl){
    //no hash if it couldnt read the file so theres nothing to save
    if(it.second=="") continue;
    out<<it.second<<" "<<it.first<<"\n";
  }

  //make sure it actually all got written (like if the disk is full)
  out.close();
  return !out.fail();
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
