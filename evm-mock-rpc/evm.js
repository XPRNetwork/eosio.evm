const { EosEvmApi } = require('eos-evm-js')

module.exports = {
  api: new EosEvmApi({
    // Ensure the API has console printing enabled
    endpoint: 'https://jungle.eosdac.io',

    // Must match the chain ID the contract is compiled with (1 by default)
    chainId: 1,

    // Enter your own private keys if you wish to sign transaction (examples provided)
    ethPrivateKeys: [
      // Public Key: 0xf79b834a37f3143f4a73fc3934edac67fd3a01cd
      '0x8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d',
    ],

    // Enter EOS account that EVM is at / will be deployed to
    eosContract: 'evmcontract2',

    // Enter your own private keys (examples provided)
    eosPrivateKeys: [
      // evmcontract2 (EOS7DJzWuEr1Zu36ZX8GXwGsvNNqdGqx8QRs7KPkqCMTxG6MBT1Eu)
      '5JACk8gJ98x3AkypbicNQviXzkqAM2wbbE3FtA79icT2Ks4bWws',
    ]
  })
}