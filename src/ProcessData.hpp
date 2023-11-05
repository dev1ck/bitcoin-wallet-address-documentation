#ifndef PROCESSDATA_H
#define PROCESSDATA_H
#include <iostream>
#include <nlohmann/json.hpp>

#include "BitcoinCore.hpp"
#include "MongoDB.hpp"

using namespace std;
using json = nlohmann::json;

class ProcessData {
 public:
  ProcessData(BitcoinCore &, MongoDB &);
  void ProcessBlock(int);

 private:
  int height;
  BitcoinCore &bitcoinCore;
  MongoDB &mongo;
  void ProcessTx(string, json &);
  json MakeNewInputWallet(string &, json &, json &, json &, int);
  void StoreNewWallet(string &, json &, json &);
  void UpdateWallet(json &, json &, json &);
  json MakeInputData(json &, int);
  json MakeOutputData(json &);
  json GetPrevData(string, int);
  json MakeTxData(string &, json &, json &, long long = 0);
  void InputSpentData(json &, string, int, string, int);
};

#endif