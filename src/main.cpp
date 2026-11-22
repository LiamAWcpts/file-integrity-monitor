#include "baseline.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <string>

namespace fs = std::filesystem;

//gets the time as a string for the log
std::string getTime(){
  std::time_t now = std::time(nullptr);
  char buf[32];
  std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",std::localtime(&now));
  return std::string(buf);
}

//prints the alert and puts it in the log
void alert(std::ofstream &log, const std::string &type, const std::string &p){
  std::string msg = "[" + getTime() + "] " + type + " " + p;
  std::cout<<msg<<std::endl;
  log<<msg<<"\n";
}

void usage(){
  std::cout<<"usage:"<<std::endl;
  std::cout<<"  fim init  <folder> [baseline file] [log file]"<<std::endl;
  std::cout<<"  fim check <folder> [baseline file] [log file]"<<std::endl;
}


int doInit(const std::string &dir, const std::string &blPath, const std::string &logPath){
  Tbl tbl = scan(dir,blPath,logPath);

  if(!saveBl(tbl,blPath)){
    std::cerr<<"couldnt save baseline to "<<blPath<<std::endl;
    return 1;
  }

  std::cout<<"saved baseline w "<<tbl.size()<<" files to "<<blPath<<std::endl;
  return 0;
}


int doCheck(const std::string &dir, const std::string &blPath, const std::string &logPath){
  //load the old one
  Tbl oldTbl;
  if(!loadBl(oldTbl,blPath)){
    std::cerr<<"couldnt read baseline "<<blPath<<", run init first"<<std::endl;
    return 1;
  }

  //scan again to get the new one
  Tbl newTbl = scan(dir,blPath,logPath);

  std::ofstream log(logPath, std::ios::app);
  int cnt=0;

  //check old files, if its gone its deleted if hash changed its modified
  for(const auto &it : oldTbl){
    if(newTbl.count(it.first)==0){
      alert(log,"DELETED ",it.first);
      cnt++;
    }
    else if(newTbl[it.first]!=it.second){
      alert(log,"MODIFIED",it.first);
      cnt++;
    }
  }

  //check new files, if it wasnt in the old one its added
  for(const auto &it : newTbl){
    if(oldTbl.count(it.first)==0){
      alert(log,"ADDED   ",it.first);
      cnt++;
    }
  }

  if(cnt==0){
    std::cout<<"no changes, checked "<<newTbl.size()<<" files"<<std::endl;
    return 0;
  }

  std::cout<<cnt<<" change(s) found, saved to "<<logPath<<std::endl;
  //return 2 so cron or a script knows something changed
  return 2;
}


int main(int argc, char *argv[]){
  if(argc<3){
    usage();
    return 1;
  }

  std::string cmd = argv[1];
  std::string dir = argv[2];
  std::string blPath = "baseline.txt";
  std::string logPath = "fim.log";

  //optional file names
  if(argc>=4) blPath = argv[3];
  if(argc>=5) logPath = argv[4];

  if(!fs::is_directory(dir)){
    std::cerr<<dir<<" isnt a folder"<<std::endl;
    return 1;
  }

  if(cmd=="init") return doInit(dir,blPath,logPath);
  else if(cmd=="check") return doCheck(dir,blPath,logPath);

  usage();
  return 1;
}
