# 비트코인 지갑 주소 데이터베이스화 도구

## Simple Document Version

```json
{
  "address": "wallet_address",
  "n_tx": "transaction 개수",
  "txs": ["트랜잭션 정보"]
}
```

<br/>

---

<br/>

# bitcoin-wallet-address-documentation

## Getting Started

### Prerequisite installer

Install mongocxx, nlohmann/json

> https://mongocxx.org/mongocxx-v3/installation/linux
>
> https://github.com/nlohmann/json

### Setting in main

```cpp
/* mongoDB URL */
MongoDB mongo("mongodb://localhost:27017");
/* bitcoin-core RPC URL, USERNAME:PASSWORD */
BitcoinCore bitcoinCore("http://localhost:8332", "username:password");
```

### Compile and start

```Bash
> make
> ./documentation
```
