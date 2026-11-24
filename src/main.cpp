#include "baseline.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <string>
#include <vector>
#include <system_error>

namespace fs = std::filesystem;

//gets the time as a string for the log
std::string getTime(){
  std::time_t now = std::time(nullptr);
  char buf[32] = "";
  //localtime can give back null, passing that to strftime would crash
  std::tm *t = std::localtime(&now);
  if(t==nullptr) return "unknown time";
  std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",t);
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
  std::cout<<"  fim init  <folder> [baseline file] [log file] [-x pattern]..."<<std::endl;
  std::cout<<"  fim check <folder> [baseline file] [log file] [-x pattern]..."<<std::endl;
  std::cout<<"  -x skips stuff, like -x logs/ or -x '*.tmp' or -x notes.txt"<<std::endl;
}


int doInit(const std::string &dir, const std::string &blPath, const std::string &logPath, const ExList &ex){
  Tbl tbl = scan(dir,blPath,logPath,ex);

  if(!saveBl(tbl,blPath)){
    std::cerr<<"couldnt save baseline to "<<blPath<<std::endl;
    return 1;
  }

  //only count the ones that actually got a hash
  int n=0;
  for(const auto &it : tbl){
    if(it.second!="") n++;
  }

  std::cout<<"saved baseline w "<<n<<" files to "<<blPath<<std::endl;
  return 0;
}


int doCheck(const std::string &dir, const std::string &blPath, const std::string &logPath, const ExList &ex){
  //load the old one
  Tbl oldTbl;
  if(!loadBl(oldTbl,blPath)){
    std::cerr<<"couldnt read baseline "<<blPath<<", run init first"<<std::endl;
    return 1;
  }

  //scan again to get the new one
  Tbl newTbl = scan(dir,blPath,logPath,ex);

  std::ofstream log(logPath, std::ios::app);
  //before if this didnt open the alerts just silently didnt get saved
  if(!log.is_open()){
    std::cerr<<"couldnt open log "<<logPath<<", changes will only show here"<<std::endl;
  }
  int cnt=0;

  //check old files, if its gone its deleted if hash changed its modified
  for(const auto &it : oldTbl){
    //if its excluded now just ignore it so adding a -x later doesnt make it say DELETED
    if(isExcluded(it.first,ex)) continue;

    if(newTbl.count(it.first)==0){
      alert(log,"DELETED ",it.first);
      cnt++;
    }
    //its still there but it couldnt be read so theres no hash to compare
    else if(newTbl[it.first]==""){
      alert(log,"CANTREAD",it.first);
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
  //pull out the -x ones first, everything else stays in order like before
  std::vector<std::string> args;
  ExList ex;
  for(int i=1; i<argc; i++){
    std::string a = argv[i];
    if(a=="-x"){
      if(i+1>=argc){
        std::cerr<<"-x needs a pattern after it"<<std::endl;
        return 1;
      }
      ex.push_back(argv[i+1]);
      i++;
    }
    else args.push_back(a);
  }

  if(args.size()<2){
    usage();
    return 1;
  }

  std::string cmd = args[0];
  std::string dir = args[1];
  std::string blPath = "baseline.txt";
  std::string logPath = "fim.log";

  //optional file names
  if(args.size()>=3) blPath = args[2];
  if(args.size()>=4) logPath = args[3];

  //the version w the error code doesnt throw if it cant even look at the path
  std::error_code ec;
  if(!fs::is_directory(dir,ec)){
    std::cerr<<dir<<" isnt a folder"<<std::endl;
    return 1;
  }

  if(cmd=="init") return doInit(dir,blPath,logPath,ex);
  else if(cmd=="check") return doCheck(dir,blPath,logPath,ex);

  usage();
  return 1;
}
