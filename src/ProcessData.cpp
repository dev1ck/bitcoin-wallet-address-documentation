#include "ProcessData.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

#include "BitcoinCore.hpp"
#include "MongoDB.hpp"

using namespace std;
using json = nlohmann::json;

ProcessData::ProcessData(BitcoinCore &bitcoinCore, MongoDB &mongo)
    : bitcoinCore(bitcoinCore), mongo(mongo) {}

void ProcessData::ProcessBlock(int height) {
  string blockHash = bitcoinCore.GetBlockHash(height);
  json blockData = bitcoinCore.GetBlock(blockHash);

  for (auto &txid : blockData["tx"]) ProcessTx(txid.dump(), blockData);
}

void ProcessData::ProcessTx(string txid, json &blockData) {
  json txData = bitcoinCore.GetRawTransaction(txid);
  int index = 0;
  string prevAddress;
  /* coinbase인 경우 */
  if (txData["vin"][0].contains("coinbase")) {
    for (auto &vout : txData["vout"]) {
      if (!vout["scriptPubKey"]["address"].is_null()) {
        string address = vout["scriptPubKey"]["address"];
        if (prevAddress == address) break;
        prevAddress = address;
        json walletData = mongo.GetWalletData(address);
        /* 신규 address */
        if (walletData.is_null()) {
          StoreNewWallet(address, blockData, txData);
        } else {
          UpdateWallet(walletData, blockData, txData);
        }
      }
    }
  }
  /* coinbase가 아닌 경우 */
  else {
    /* vin 데이터 처리 부분 */
    for (auto &vin : txData["vin"]) {
      string txid = vin["txid"];
      int preN = vin["vout"];
      json preTx = bitcoinCore.GetRawTransaction('"' + txid + '"');
      if (!preTx["vout"][preN]["address"].is_null()) {
        string address = preTx["vout"][preN]["address"];
        if (prevAddress == address) break;
        prevAddress = address;
        json walletData = mongo.GetWalletData(address);
        if (walletData.is_null()) {
          // MakeNewInputWallet()
        } else {
          UpdateWallet(walletData, blockData, txData);
        }
      }
    }
    /* vout 데이터 처리 */
    for (auto &vout : txData["vout"]) {
      if (!vout["scriptPubKey"]["address"].is_null()) {
        string address = vout["scriptPubKey"]["address"];
        if (prevAddress == address) break;
        prevAddress = address;
        json walletData = mongo.GetWalletData(address);
        /* 신규 address */
        if (walletData.is_null()) {
          StoreNewWallet(address, blockData, txData);
        } else {
          UpdateWallet(walletData, blockData, txData);
        }
      }
    }
  }
}

void ProcessData::StoreNewWallet(string &address, json &blockData,
                                 json &txData) {
  json data;
  int n_unredeemed = 0;
  long long received = 0;

  for (auto &vout : txData["vout"]) {
    if (!vout["scriptPubKey"]["address"].is_null() &&
        vout["scriptPubKey"]["address"] == address) {
      n_unredeemed++;
      received += (long long)vout["value"];
    }
  }

  data["address"] = address;
  data["n_tx"] = 1;
  data["n_unredeemed"] = n_unredeemed;
  data["total_received"] = received;
  data["total_sent"] = 0;
  data["final_balance"] = received;
  data["txs"].push_back(MakeTxData(address, blockData, txData));
  mongo.StoreWalletData(data);
}

void ProcessData::UpdateWallet(json &walletData, json &blockData,
                               json &txData) {
  if (walletData["txs"][0]["txid"] == txData["txid"]) return;
  string address = walletData["address"];
  json updateData;
  int n_unredeemed;
  long long received, sent;
  received = sent = n_unredeemed = 0;
  json newTx =
      MakeTxData(address, blockData, txData, walletData["txs"][0]["balance"]);
  updateData["txs"] = walletData["txs"];
  updateData["txs"].insert(updateData["txs"].begin(), newTx);
  updateData["n_tx"] = (int)walletData["n_tx"] + 1;

  if (!newTx.contains("coinbase")) {
    for (auto &input : newTx["inputs"]) {
      if (input["prev_out"]["addr"] == address) {
        n_unredeemed--;
        sent += (long long)input["prev_out"]["value"];
        InputSpentData(updateData, input["txid"], input["prev_out"]["n"],
                       newTx["txid"], input["n"]);
      }
    }
  }
  for (auto &vout : txData["vout"]) {
    if (!vout["scriptPubKey"]["address"].is_null() &&
        vout["scriptPubKey"]["address"] == address) {
      n_unredeemed++;
      received += (long long)vout["value"];
    }
  }

  updateData["n_unredeemed"] = (int)walletData["n_unredeemed"] + n_unredeemed;
  updateData["total_received"] =
      (long long)walletData["total_received"] + received;
  updateData["total_sent"] = (long long)walletData["total_sent"] + sent;
  updateData["final_balance"] = updateData["txs"][0]["balance"];

  mongo.UpdateWalletData(walletData["_id"].dump(), updateData);
}

json ProcessData::MakeTxData(string &address, json &blockData, json &txData,
                             long long prevBalance) {
  json txDoc;
  long long inputValue, outValue, fee, result;
  inputValue = outValue = fee = result = 0;
  txDoc["txid"] = txData["txid"];
  txDoc["ver"] = txData["version"];
  txDoc["n_input"] = txData["vin"].size();
  txDoc["n_output"] = txData["vout"].size();
  txDoc["size"] = txData["size"];
  txDoc["weight"] = txData["weight"];
  txDoc["locktime"] = txData["locktime"];
  txDoc["block_height"] = blockData["height"];
  txDoc["time"] = txData["time"];

  int index = 0;
  /* vin 처리 */
  json inputData;
  for (auto &vin : txData["vin"]) {
    if (vin.contains("coinbase")) {
      inputData["coinbase"] = vin["coinbase"];
      inputData["sequence"] = vin["sequence"];
    } else {
      inputData = MakeInputData(vin, index);
      inputValue += (long long)inputData["prev_out"]["value"];
      if (!inputData["prev_out"]["addr"].is_null() &&
          inputData["prev_out"]["addr"] == address)
        result -= (long long)inputData["prev_out"]["value"];
    }
    txDoc["inputs"].push_back(inputData);
    index++;
  }

  /* vout 처리 */
  for (auto &vout : txData["vout"]) {
    txDoc["outputs"].push_back(MakeOutputData(vout));
    outValue += (long long)vout["value"];
    if (!vout["scriptPubKey"]["address"].is_null() &&
        vout["scriptPubKey"]["address"] == address)
      result += (long long)vout["value"];
  }
  txDoc["result"] = result;
  txDoc["balance"] = prevBalance + result;
  if (inputValue == 0)
    txDoc["fee"] = 0;
  else
    txDoc["fee"] = inputValue - outValue;
  return txDoc;
}

json ProcessData::MakeInputData(json &txInput, int index) {
  json input;
  input["sequence"] = txInput["sequence"];
  input["txid"] = txInput["txid"];
  input["n"] = index;
  input["prev_out"] = GetPrevData(txInput["txid"], txInput["vout"]);

  return input;
}

json ProcessData::MakeOutputData(json &txOutput) {
  json output;
  output["spent"] = false;
  output["value"] = txOutput["value"];
  output["spending_outpoints"] = {};
  output["n"] = txOutput["n"];
  if (!txOutput["scriptPubKey"]["address"].is_null())
    output["addr"] = txOutput["scriptPubKey"]["address"];
  else
    output["addr"] = "Unknown";

  return output;
}

json ProcessData::GetPrevData(string txid, int n) {
  json preTx = bitcoinCore.GetRawTransaction('"' + txid + '"')["vout"][n];
  json preData;
  if (!preTx["scriptPubKey"]["address"].is_null())
    preData["addr"] = preTx["scriptPubKey"]["address"];
  else
    preData["addr"] = "Unknown";

  preData["n"] = n;
  preData["value"] = preTx["value"];

  return preData;
}

void ProcessData::InputSpentData(json &updateData, string prevTxid, int prevN,
                                 string txid, int index) {
  for (auto &tx : updateData["txs"]) {
    if (tx["txid"] == prevTxid) {
      json prevData;
      prevData["txid"] = txid;
      prevData["n"] = index;
      tx["outputs"][prevN]["spending_outpoints"] = prevData;
      tx["outputs"][prevN]["spent"] = true;
      break;
    }
  }
}


/* Input 데이터에 새로운 지갑주소가 사용될 때 필요, 정상적인 상황이라면 불필요 함수 */
json ProcessData::MakeNewInputWallet(string &address, json &blockData,
                                     json &txData, json &preTxData int index)
{ json data; json preBlock = bitcoinCore.GetBlock(preTxData["blockhash"]);
  json preTx = MakeTxData(string & address, json & preBlock, json &
preTxData); json preOut = preTx["out"][txData["vin"][index]["vout"]]; int
n_unredeemed, received, sent; n_unredeemed = received = sent = 0;

  preOut["spent"] = true;
  preOut["spending_outpoints"] = {{"txid", txData["txid"]}, {"n", index}};
  preTx["result"] = preOut["value"];
  json nowTx = MakeTxData(string & address, json & blockData, json & txData);
  data["txs"].insert(preTx);
  data["txs"].insert(nowTx);
  data["address"] = address;
  data["n_tx"] = data["txs"].size();

  for (auto &tx : data["txs"]) {
    if (tx["result"] > 0)
      received += tx["result"];
    else
      sent += tx["result"];
    for (auto &out : tx["out"]) {
      if (out["addr"] == address && !out["spent"]) n_unredeemed++;
    }
  }

  data["n_unredeemed"] = n_unredeemed;
  data["total_received"] = received;
  data["total_sent"] = -sent;
  data["final_balance"] = data["txs"][0]["balance"];
}
