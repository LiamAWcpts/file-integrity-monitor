#include "hash.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

std::string hashFile(const std::string &fp){
  //open file
  std::ifstream f(fp, std::ios::binary);
  if(!f.is_open()) return "";

  //setup the hash thing from openssl
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha256(),nullptr);

  //read file in chunks so big files dont break it
  char buf[4096];
  while(f.read(buf,sizeof(buf)) || f.gcount()>0){
    EVP_DigestUpdate(ctx, buf, f.gcount());
  }

  //get the final hash
  unsigned char dig[EVP_MAX_MD_SIZE];
  unsigned int len=0;
  EVP_DigestFinal_ex(ctx,dig,&len);
  EVP_MD_CTX_free(ctx);

  //bytes to hex string
  std::stringstream ss;
  for(unsigned int i=0; i<len; i++)
  {
    ss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)dig[i];
  }
  return ss.str();
}
