#ifndef BITCOINCORE_H
#define BITCOINCORE_H
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class BitcoinCore {
 public:
  BitcoinCore(string, string);
  int GetHeight();
  json GetBlock(const string &, int = 1);
  string GetBlockHash(const int);
  json GetRawTransaction(const string &);

 private:
  string userpass;
  string url;

  json Send(const string &, const string &);
  json Send(const string &);
  json SendCurl(const string &);

  static size_t WriteCallback(void *, size_t, size_t, void *);
};

#endif
