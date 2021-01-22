var BN = require('bn.js')
const abiDecoder = require('abi-decoder')
const abi = require('ethereumjs-abi')

const { api } = require('./evm')

const receipts = {};

const balance_abi = [{"constant":true,"inputs":[{"name":"user","type":"address"},{"name":"token","type":"address"}],"name":"tokenBalance","outputs":[{"name":"","type":"uint256"}],"payable":false,"stateMutability":"view","type":"function"},{"constant":true,"inputs":[{"name":"users","type":"address[]"},{"name":"tokens","type":"address[]"}],"name":"balances","outputs":[{"name":"","type":"uint256[]"}],"payable":false,"stateMutability":"view","type":"function"},{"payable":true,"stateMutability":"payable","type":"fallback"}]
abiDecoder.addABI(balance_abi);

const methods = {
  net_listening  : () => true,
  eth_blockNumber: async () => (await api.eos.rpc.get_info()).head_block_num,
  net_version            : () => api.chainId,
  eth_chainId            : () => '0x' + api.chainId.toString(16),
  eth_accounts           : () => Object.keys(api.ethPrivateKeys),
  eth_getTransactionCount: async ([address, block]) => await api.eos.getNonce(address),
  eth_getCode            : async ([address, block]) => {
    try {
      const account = await api.eos.getEthAccount(address)
      return '0x' + Buffer.from(account.code).toString('hex')
    } catch (e) {
      return '0x0'
    }
  },
  eth_getStorageAt       : async ([address, position, block]) => await api.eos.getStorageAt(address, position),
  eth_estimateGas        : async ([{ from, data, value }]) => 2000000,
  eth_gasPrice           : () => "0x1",

  eth_getBalance : async ([address, block]) => {
    try {
      const account = await api.eos.getEthAccount(address)
      return '0x' + account.balance.toString(16)
    } catch (e) {
      return '0x0'
    }
  },

  eth_call: async ([txParams, block]) => {
    // Special case for metamask on mainnet, kovan, etc to fetch balances
    if (
      [1,3,4,42].includes(api.chainId) &&
      [
        "0xb1f8e55c7f64d203c1400b9d8555d050f94adf39",
        "0x9f510b19f1ad66f0dcf6e45559fab0d6752c1db7",
        "0xb8e671734ce5c8d7dfbbea5574fa4cf39f7a54a4",
        "0xb1d3fbb2f83aecd196f474c16ca5d9cffa0d0ffc"
      ].includes(txParams.to)
    ) {
      const { params: [users, tokens] } = abiDecoder.decodeMethod(txParams.data);
      if (tokens.value.length === 1 && tokens.value[0] === "0x0000000000000000000000000000000000000000") {
        const balances = await Promise.all(users.value.map(user => methods.eth_getBalance([user, null])))
        return '0x' + abi.rawEncode(balances.map(x => "uint256"), balances).toString('hex')
      }
    }

    const encodedTx = await api.createEthTx({
      ...txParams,
      value: txParams.value ? new BN(Buffer.from(txParams.value.slice(2), 'hex')) : 0,
      sender: txParams.from
    })
    const output = await api.eos.call({ account: api.eos.eosContract, tx: encodedTx, sender: txParams.from })
    return '0x' + output
  },

  eth_sendRawTransaction: async ([signedTx]) => {
    try {
      const { eos, eth } = await api.eos.raw({
        account: api.eos.eosContract,
        tx: signedTx
      })

      console.log(eos, eth)

      const hash = '0x' + eth.transactionHash
      receipts[hash] = eth;

      return hash;
    } catch (e) {
      console.log(e)
      return null
    }
  },

  eth_sendTransaction: async ([txParams]) => {
    const encodedTx = await api.createEthTx({
      ...txParams,
      value: new BN(Buffer.from(txParams.value.slice(2), 'hex')),
      rawSign: true,
      sender: txParams.from
    })

    try {
      const { eos, eth } = await api.eos.raw({
        account: api.eos.eosContract,
        tx: encodedTx
      })

      const hash = '0x' + eth.transactionHash
      receipts[hash] = {
        ...eth,
        input: txParams.data // Add this in so we dont have to print in contract
      };

      return hash;
    } catch (e) {
      console.log(e)
      return null
    }
  },

  eth_getTransactionReceipt: ([hash]) => {
    const stored = receipts[hash]
    if (stored) {
      return {
        status           : '0x' + stored.status,
        transactionHash  : '0x' + stored.transactionHash,
        transactionIndex : '0x0',
        blockHash        : '0x0',
        blockNumber      : '0x0',
        from             : '0x' + stored.from,
        to               : '0x' + stored.to,
        cumulativeGasUsed: '0x' + stored.gasUsed,
        gasUsed          : '0x' + stored.gasUsed,
        contractAddress  : '0x' + stored.createdAddress,
        logs             : stored.logs || [],
        logsBloom        : '0x0',
      }
    } else {
      return null
    }
  },

  eth_getTransactionByHash: ([hash]) => {
    const stored = receipts[hash]
    if (stored) {
      return {
        blockHash       : '0x0',
        blockNumber     : '0x0',
        from            : '0x' + stored.from,
        gas             : '0x' + stored.gasLimit,
        gasPrice        : '0x' + stored.gasPrice,
        hash            : '0x' + stored.transactionHash,
        input           : '0x' + stored.input,
        nonce           : '0x' + stored.nonce,
        to              : '0x' + stored.to,
        transactionIndex: '0x0',
        value           : '0x' + stored.value,
        v               : '0x' + stored.v,
        r               : '0x' + stored.r,
        s               : '0x' + stored.s
      }
    } else {
      return null
    }
  },

  // Not implemented, random data
  eth_getBlockByNumber: ([block, full]) => {
    return {
      "number"          : "0x0",
      "hash"            : "0x0",
      "parentHash"      : "0x0",
      "nonce"           : "0x0",
      "sha3Uncles"      : "0x0",
      "logsBloom"       : "0x0",
      "transactionsRoot": "0x0",
      "stateRoot"       : "0x0",
      "miner"           : "0x0000000000000000000000000000000000000000",
      "difficulty"      : "0x0",
      "totalDifficulty" : "0x0",
      "extraData"       : "0x0000000000000000000000000000000000000000000000000000000000000000",
      "size"            : "0x0",
      "gasLimit"        : "0x989680",
      "gasUsed"         : "0x989680",
      "timestamp"       : "0x0",
      "transactions"    : Object.values(receipts),
      "uncles"          : []
    }
  },

  eth_getBlockByHash                  : ([hash, full]) => this.eth_getBlockByNumber(),
  eth_getBlockTransactionCountByHash  : ([hash]) => "Not Implemented",
  eth_getBlockTransactionCountByNumber: ([block]) => "Not Implemented",
  eth_getUncleCountByBlockHash        : ([hash]) => "Not Implemented",
  eth_getUncleCountByBlockNumber      : ([block]) => "Not Implemented",
}

module.exports = methods;