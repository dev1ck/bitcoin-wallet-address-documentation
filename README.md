# 비트코인 지갑 주소 데이터베이스화 도구

<aside>
💡 Bitcoin Core RPC 인터페이스를 활용하여 비트코인 내의 모든 블록에서 트랜잭션 데이터를 검색하고, 이를 분석하여 각 지갑 주소를 데이터베이스에 기록하는 도구입니다.
BTDS에서 사용될 예정이었으나 현재의 하드웨어 성능과 시간상의 제약으로 인해, 해당 도구 사용은 중단되었습니다."
</aside>

<br/>

## full Branch

- simple Branch 보다 느린 성능
- 지갑 전체 데이터 Parsing

```json
{
  "address": "wallet_address",
  "n_tx": "transaction 개수",
  "n_unredeemed": "utxo 개수",
  "total_received": "총 받은 금액",
  "total_sent": "총 보낸 금액",
  "final_balance": "현재 잔액",
  "txs": ["트랜잭션 정보"]
}
```
<br/>

## simple Branch

- full branch 보다 빠른 성능
- thread/mutex 사용

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
