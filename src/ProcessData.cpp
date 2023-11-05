#include "ProcessData.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

#include "BitcoinCore.hpp"
#include "MongoDB.hpp"

using namespace std;
using json = nlohmann::json;

ProcessData::ProcessData(BitcoinCore &bitcoinCore, MongoDB &mongo)
    : bitcoinCore(bitcoinCore), mongo(mongo) {}

void ProcessData::ProcessBlock(int height) {
  string blockHash = bitcoinCore.GetBlockHash(height);
  json blockData = bitcoinCore.GetBlock(blockHash);
  int getIndex = mongo.GetSavedIndex();
  int index = 0;
  for (auto &txid : blockData["tx"]) {
    if (index++ < getIndex) {
      continue;
    }
    ProcessTx(txid, blockData);
    mongo.UpdateIndex();
  }
}

void ProcessData::ProcessTx(string txid, json &blockData) {
  json txData = bitcoinCore.GetRawTransaction(txid);
  vector<thread> threads;

  if (!txData["vin"][0].contains("coinbase")) {
    for (auto &vin : txData["vin"]) {
      threads.push_back(
          thread([this, &txid, &vin] { this->ProcessTx_vin(txid, vin); }));
    }
  }

  for (auto &vout : txData["vout"]) {
    threads.push_back(
        thread([this, &txid, &vout] { this->ProcessTx_vout(txid, vout); }));
  }

  for (auto &t : threads) {
    t.join();
  }
}

/* vin 데이터 처리 */
void ProcessData::ProcessTx_vin(string &txid, json &vin) {
  string prevTxid = vin["txid"];
  int preN = vin["vout"];
  json preTx = bitcoinCore.GetRawTransaction(prevTxid);
  if (!preTx["vout"][preN]["address"].is_null()) {
    string address = preTx["vout"][preN]["address"];
    ProcessAddress(address, txid, &prevTxid);
  }
}

/* vout 데이터 처리 */
void ProcessData::ProcessTx_vout(string &txid, json &vout) {
  if (!vout["scriptPubKey"]["address"].is_null()) {
    string address = vout["scriptPubKey"]["address"];
    ProcessAddress(address, txid);
  }
}

void ProcessData::ProcessAddress(string &address, string &txid,
                                 string *prevTxid) {
  lock_guard<mutex> lock(mtx);
  json walletData = mongo.GetWalletData(address);
  if (walletData.is_null()) {
    if (prevTxid != nullptr) {
      StoreNewWallet(address, txid, *prevTxid);
    } else {
      StoreNewWallet(address, txid);
    }
  } else {
    UpdateWallet(walletData, txid);
  }
}

void ProcessData::StoreNewWallet(string &address, string &txid) {
  json data;
  data["address"] = address;
  data["n_tx"] = 1;
  data["txs"].push_back(txid);
  mongo.StoreWalletData(data);
}

void ProcessData::StoreNewWallet(string &address, string &txid,
                                 string &prevTxid) {
  json data;
  data["address"] = address;
  data["n_tx"] = 2;
  data["txs"].push_back(txid);
  data["prevTxid"].push_back(prevTxid);
  mongo.StoreWalletData(data);
}

void ProcessData::UpdateWallet(json &walletData, string &txid) {
  if (walletData["txs"][0] == txid) return;
  // txid 추가
  mongo.UpdateWalletData(walletData["_id"].dump(), txid);
}
