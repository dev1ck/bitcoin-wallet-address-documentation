#ifndef PROCESSDATA_H
#define PROCESSDATA_H
#include <iostream>
#include <mutex>
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
  void StoreNewWallet(string &address, string &txid);
  void StoreNewWallet(string &address, string &txid, string &prevTxid);
  void UpdateWallet(json &, string &);
  void ProcessAddress(string &address, string &txid,
                      string *prevTxid = nullptr);
  void ProcessTx_vin(string &, json &);
  void ProcessTx_vout(string &, json &);
  mutex mtx;
};

#endif