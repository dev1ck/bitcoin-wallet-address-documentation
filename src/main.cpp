#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "BitcoinCore.hpp"
#include "MongoDB.hpp"
#include "ProcessData.hpp"
using namespace std;
using json = nlohmann::json;

int main() {
  MongoDB::Instance();
  /* mongoDB URL */
  MongoDB mongo("mongodb://localhost:27017");
  /* bitcoin-core RPC URL, USERNAME:PASSWORD */
  BitcoinCore bitcoinCore("http://localhost:8332", "username:password");
  ProcessData processData(bitcoinCore, mongo);
  int startBlock;
  int lastBlock;

  // errLog.open("../err.log");

  /* wallets collection 유무 확인, 없으면 인덱싱 생성 */
  mongo.CreateIndexes();
  /* update collection 유무 확인, 없으면 생성 */
  startBlock = mongo.GetSavedHeight();

  /* DB화 진행 */
  while (1) {
    /* 현재 블럭 확인 후 저장된 블록과 동일하면 종료 */
    lastBlock = bitcoinCore.GetHeight();
    if (startBlock >= lastBlock) break;

    for (startBlock++; startBlock <= lastBlock; startBlock++) {
      try {
        processData.ProcessBlock(startBlock);
        string resultMessage =
            to_string(startBlock) + " block documantation complite!!\n";

        ofstream resultLog("../result.log", ios::app);
        resultLog << resultMessage;
        resultLog.close();

        /* update collection 업데이트 */
        mongo.UpdateHeight(startBlock);

      } catch (string exception) {
        cout << exception << endl;
        // string errMessage = "block height : " + to_string(startBlock) +
        //                     ", message : " + exception;
        // errLog.write(errMessage.c_str(), errMessage.size());
        // resultLog.close();
        // errLog.close();
        // return 0;
      }
    }
    startBlock--;
  }

  return 0;
}