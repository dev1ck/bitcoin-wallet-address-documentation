#include "BitcoinCore.hpp"

#include <curl/curl.h>
#include <unistd.h>

#include <cmath>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

BitcoinCore::BitcoinCore(string url, string userpass) : url(url), userpass(userpass) {}

int BitcoinCore::GetHeight() {
  json rawData = Send("getchaintips");
  return rawData[0]["height"];
}
string BitcoinCore::GetBlockHash(const int height) {
  return Send("getblockhash", to_string(height));
}
json BitcoinCore::GetBlock(const string &blockhash, int verbosity) {
  return Send("getblock", "\"" + blockhash + "\", " + to_string(verbosity));
}
json BitcoinCore::GetRawTransaction(const string &txid) {
  return Send("getrawtransaction", '"' + txid + "\", true");
}

size_t BitcoinCore::WriteCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  ((string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

json BitcoinCore::Send(const string &method, const string &params) {
  string data = R"({
        "jsonrpc": "1.0",
        "id": 1,
        "method": ")" +
                method + R"(",
        "params": [)" +
                params + R"(]
    })";
  for (int i = 0; i < 10; i++) {
    try {
      return SendCurl(data);
    } catch (int &err) {
      sleep(1);
      continue;
    }
  }
  exit(0);
}

json BitcoinCore::Send(const string &method) {
  string data = R"({
        "jsonrpc": "1.0",
        "id": 1,
        "method": ")" +
                method + R"("
    })";

  return SendCurl(data);
}

json BitcoinCore::SendCurl(const string &data) {
  CURL *curl = curl_easy_init();
  string readBuffer;

  if (curl) {
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "content-type: text/plain;");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERPWD, userpass.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
           << std::endl;
    }
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }
  try {
    json jsonRes = json::parse(readBuffer);
    return jsonRes["result"];
  } catch (json::parse_error &e) {
    throw -1;
  }
}
