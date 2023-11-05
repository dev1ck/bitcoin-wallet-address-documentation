#ifndef MONGODB_H
#define MONGODB_H
#include <iostream>
#include <mongocxx/client.hpp>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class MongoDB {
 public:
  MongoDB(string);
  void CreateIndexes();
  static void Instance();
  void UpdateHeight(int);
  int GetSavedHeight();
  json GetWalletData(string);
  void StoreWalletData(json &);
  void UpdateWalletData(string, json &);

 private:
  mongocxx::client client;
  mongocxx::database db;
  mongocxx::collection updateCol;
  mongocxx::collection walletCol;
};

#endif