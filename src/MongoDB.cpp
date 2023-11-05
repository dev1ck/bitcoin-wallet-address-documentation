#include "MongoDB.hpp"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <iostream>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

MongoDB::MongoDB(string url) : client{mongocxx::uri{url}} {
  db = client["bitcoin"];
  updateCol = db["update"];
  walletCol = db["wallets"];
}

void MongoDB::CreateIndexes() {
  auto result = walletCol.find_one({});
  if (result)
    return;
  else {
    auto index_specification = make_document(kvp("address", 1));
    auto index_option = make_document(kvp("unique", true));
    walletCol.create_index(move(index_specification), move(index_option));
  }
}

int MongoDB::GetSavedHeight() {
  auto result = updateCol.find_one({});
  if (result) {
    auto height = result->view()["height"];
    return height.get_int32().value;
  } else {
    auto doc = make_document(kvp("height", 0), kvp("index", 0),
                             kvp("time", time(NULL)));
    updateCol.insert_one(doc.view());

    return 0;
  }
}

int MongoDB::GetSavedIndex() {
  auto result = updateCol.find_one({});

  auto index = result->view()["index"];
  return index.get_int32().value;
}

void MongoDB::UpdateHeight() {
  auto doc = make_document(
      kvp("$inc", make_document(kvp("height", 1))),
      kvp("$set", make_document(kvp("index", 0), kvp("time", time(NULL)))));
  updateCol.update_many({}, doc.view());
}
void MongoDB::UpdateIndex() {
  auto doc = make_document(kvp("$inc", make_document(kvp("index", 1))));
  try {
    updateCol.update_many({}, doc.view());
  } catch (string& err) {
    cout << "update index err" << endl;
  }
}

json MongoDB::GetWalletData(string addr) {
  try {
    auto result = walletCol.find_one(make_document(kvp("address", addr)));
    if (result) {
      return json::parse(bsoncxx::to_json(*result));
    }
  } catch (string& err) {
    cout << "GetWalletData err" << endl;
  }

  return {};
}

void MongoDB::StoreWalletData(json& data) {
  auto document = bsoncxx::from_json(data.dump());
  try {
    walletCol.insert_one(document.view());
  } catch (string& err) {
    cout << "storewallet err" << endl;
  }
}

void MongoDB::UpdateWalletData(string id, string& txid) {
  string oid_value = id.substr(9, 24);
  auto updateData = make_document(
      kvp("$push",
          make_document(kvp("txs", make_document(kvp("$each", make_array(txid)),
                                                 kvp("$position", 0))))),
      kvp("$inc", make_document(kvp("n_tx", 1))));
  try {
    walletCol.update_one(make_document(kvp("_id", bsoncxx::oid(oid_value))),
                         updateData.view());
  } catch (string& err) {
    cout << "update one err" << endl;
  }
}

void MongoDB::Instance() { mongocxx::instance inst{}; }