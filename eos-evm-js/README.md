# EOSIO EVM JS SDK

### Installation

Requires nodejs and npm installed

```bash
npm install eos-evm-js
```

### How to setup EVM and deploy ERC-20 Token on EOSIO in 5 minutes

```js
const { EosEvmApi } = require('eos-evm-js')

const evmContractAccount = 'evmcontract2'
const evmNormalAccount = 'evmaccount11'
const SYSTEM_SYMBOL = 'EOS'

const api = new EosEvmApi({
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
  eosContract: evmContractAccount,

  // Enter your own private keys (examples provided)
  eosPrivateKeys: [
    // evmcontract2 (EOS7DJzWuEr1Zu36ZX8GXwGsvNNqdGqx8QRs7KPkqCMTxG6MBT1Eu)
    '5JACk8gJ98x3AkypbicNQviXzkqAM2wbbE3FtA79icT2Ks4bWws',
    // evmaccount11 (EOS8Z9y2b1GfAkFUQxBTsiu9DJSLebVoU8cpwLAfXcgWDRWg9aM2Q)
    '5JhwbcHTVk16Pv7fCgitNSHgwGwjAPEgEJbiaCcXaza1PKrbCns'
  ]
})

// Import contract compiled with solc (check eos-evm-js/src/eth-contracts/compile.ts to compile your own)
// We provide compiled ERC20 and ERC721 contracts
const compiledErc20AndErc721 = require('eos-evm-js/dist/eth-contracts/compiled.json')

// Load ETH contracts with abi and bytecode, plus the TX sending EOS account
api.loadContractFromAbi({
  account: evmNormalAccount, // Example EOS account
  abi: compiledErc20AndErc721.contracts.ERC20.Token.abi,
  bytecodeObject: compiledErc20AndErc721.contracts.ERC20.Token.evm.bytecode.object
})

async function main () {
  // Deploy EVM contract to EOSIO (deploys to eosContract provided in new EosEvmApi)
  await api.eos.setupEvmContract()

  // For development (if TESTING is enabled in contract), clears all data in contract
  await api.eos.clearAll()

  // Creates new address based on RLP(eosaccount, arbitrarydata)
  await api.eos.create({ account: evmNormalAccount, data: 'test' })

  // Transfer EOS to contract to deposit to address
  await api.eos.deposit({ from: evmNormalAccount, quantity: `0.0002 ${SYSTEM_SYMBOL}` })

  // Get all data for new address (address, account, nonce, balance, code)
  const sender = await api.eos.getEthAccountByEosAccount(evmNormalAccount)
  console.log(`${sender.address} (${evmNormalAccount}) Balance:`, sender.balance) // 0.0001 EOS
  console.log(`${sender.address} (${evmNormalAccount}) Nonce:`, sender.nonce) // 0

  // Deploy ERC20 contract (Name, Symbol, Decimals, Total Supply)
  // The returned response "eth" is the EVM transaction receipt, and "eos" is the EOS transaction receipt
  const { eth, eos } = await api.eth.deploy('FIRE Token', 'FIRE', 4, 1000000, { sender: sender.address })

  // Set the created address as the EVM contract to interact with
  api.setEthereumContract(eth.createdAddress)

  // Query ERC20 balance using "view" function calls
  console.log(`${sender.address} FIRE Balance: `, +(await api.eth.balanceOf(sender.address)).toString(10)) // 1,000,000

  // New receiver address to send tokens to
  const receiver = '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd'

  // Transfer system tokens to address to create it
  await api.transfer({ account: evmNormalAccount, sender: sender.address, to: receiver, quantity: `0.0001 ${SYSTEM_SYMBOL}` })

  // Transfer 1000 FIRE ERC20 tokens
  await api.eth.transfer(receiver, 1000, { sender: sender.address })

  // Query ERC20 FIRE balance using "view" function calls
  console.log(`${sender.address} Balance:`, +(await api.eth.balanceOf(sender.address)).toString(10), 'FIRE') // 999,000
  console.log(`${receiver} Balance:`,       +(await api.eth.balanceOf(receiver)).toString(10), 'FIRE'), //   1,000

  // Set allowance, and modify it
  await api.eth.approve(receiver, 100, { sender: sender.address })
  await api.eth.increaseAllowance(receiver, 1000, { sender: sender.address })
  await api.eth.decreaseAllowance(receiver, 600, { sender: sender.address })

  // Query allowance (another example of using non-state modifying calls)
  const allowance = await api.eth.allowance(sender.address, receiver, { sender: receiver })
  console.log(`Allowance for ${sender.address}->${receiver}:`, +allowance.toString(10), 'FIRE') // 500

  // Use the allowance to transfer
  // rawSign uses ethereum private key to sign instead of EOSIO account permissions
  await api.eth.transferFrom(sender.address, receiver, 500, { sender: receiver, rawSign: true })

  // Withdraw tokens
  await api.eos.withdraw({ account: evmNormalAccount, quantity: `0.0001 ${SYSTEM_SYMBOL}` })

  // Other available functions, check docs
  // await getStorageAt(address, key)
  // await createEthTx({ sender, data, gasLimit, value, to, rawSign = false })
  // async getNonce(address)
  // async getEthAccount(address)
}

main()
```

## API

<!-- Generated by documentation.js. Update this documentation by updating the source code. -->

#### Table of Contents

-   [EosEvmApi](#eosevmapi)
    -   [setEthereumContract](#setethereumcontract)
        -   [Parameters](#parameters)
    -   [loadContractFromAbi](#loadcontractfromabi)
        -   [Parameters](#parameters-1)
    -   [transfer](#transfer)
        -   [Parameters](#parameters-2)
    -   [createEthTx](#createethtx)
        -   [Parameters](#parameters-3)
-   [EosApi](#eosapi)
    -   [Parameters](#parameters-4)
    -   [transact](#transact)
        -   [Parameters](#parameters-5)
    -   [raw](#raw)
        -   [Parameters](#parameters-6)
    -   [call](#call)
        -   [Parameters](#parameters-7)
    -   [create](#create)
        -   [Parameters](#parameters-8)
    -   [withdraw](#withdraw)
        -   [Parameters](#parameters-9)
    -   [deposit](#deposit)
        -   [Parameters](#parameters-10)
    -   [clearAll](#clearall)
    -   [getTable](#gettable)
        -   [Parameters](#parameters-11)
    -   [getAllAddresses](#getalladdresses)
        -   [Parameters](#parameters-12)
    -   [getEthAccount](#getethaccount)
        -   [Parameters](#parameters-13)
    -   [getNonce](#getnonce)
        -   [Parameters](#parameters-14)
    -   [getNonce](#getnonce-1)
        -   [Parameters](#parameters-15)
    -   [getStorageAt](#getstorageat)
        -   [Parameters](#parameters-16)
    -   [getEthAccountByEosAccount](#getethaccountbyeosaccount)
        -   [Parameters](#parameters-17)
    -   [setupEvmContract](#setupevmcontract)
        -   [Parameters](#parameters-18)
-   [EosApi](#eosapi-1)
    -   [transact](#transact-1)
        -   [Parameters](#parameters-19)
    -   [raw](#raw-1)
        -   [Parameters](#parameters-20)
    -   [call](#call-1)
        -   [Parameters](#parameters-21)
    -   [create](#create-1)
        -   [Parameters](#parameters-22)
    -   [withdraw](#withdraw-1)
        -   [Parameters](#parameters-23)
    -   [deposit](#deposit-1)
        -   [Parameters](#parameters-24)
    -   [clearAll](#clearall-1)
    -   [getTable](#gettable-1)
        -   [Parameters](#parameters-25)
    -   [getAllAddresses](#getalladdresses-1)
        -   [Parameters](#parameters-26)
    -   [getEthAccount](#getethaccount-1)
        -   [Parameters](#parameters-27)
    -   [getNonce](#getnonce-2)
        -   [Parameters](#parameters-28)
    -   [getNonce](#getnonce-3)
        -   [Parameters](#parameters-29)
    -   [getStorageAt](#getstorageat-1)
        -   [Parameters](#parameters-30)
    -   [getEthAccountByEosAccount](#getethaccountbyeosaccount-1)
        -   [Parameters](#parameters-31)
    -   [setupEvmContract](#setupevmcontract-1)
        -   [Parameters](#parameters-32)

### EosEvmApi

#### setEthereumContract

Sets the address for ethereum contract

##### Parameters

-   `contract`  ethereum contract address

#### loadContractFromAbi

Initializes Web3 like interface to send actions to EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments (optional, default `{}`)
    -   `args.account` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)?** EOSIO account to interact with EVM
    -   `args.abi` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)?** ABI object
    -   `args.bytecodeObject` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)?** Bytecode object

#### transfer

Transfers value inside EVM

##### Parameters

-   `overrides`
-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments (optional, default `{}`)
    -   `args.account` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)?** The EOS account associated to ETH address
    -   `args.sender` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)?** The ETH address sending the TX
    -   `args.to` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)?** The ETH address sending the transaction (nonce is fetched on-chain for this address)
    -   `args.quantity` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)?** The ETH address sending the transaction (nonce is fetched on-chain for this address)
    -   `args.rawSign` **[boolean](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Boolean)?** Whether to sign transaction with ethereum private key. False means to use EOSIO authorization

#### createEthTx

Generates RLP encoded transaction sender parameters

##### Parameters

-   `sender`  The ETH address sending the transaction (nonce is fetched on-chain for this address)
-   `data`  The data in transaction
-   `gasLimit`  The gas limit of the transaction
-   `value`  The value in the transaction
-   `to`  The ETH address to send transaction to
-   `sign`  Whether to sign the transaction

Returns **any** RLP encoded transaction

### EosApi

EOS API used as a subset of EosEvmApi

#### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.eosPrivateKeysEOSIO` **[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** private keys
    -   `args.endpointEOSIO` **[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** rpc endpoint
    -   `args.eosContractEOSIO` **[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** contract name with EVM

#### transact

Bundles actions into a transaction to send to EOS Api

##### Parameters

-   `actions`
-   `actionsFull` **[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;any>** EOSIO actions

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EVM receipt and EOS receipt

#### raw

Sends a ETH TX to EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.txRaw` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** RLP encoded hex string
    -   `args.senderThe` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** ETH address of an account if tx is not signed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;EvmResponse>** EVM receipt and EOS receipt

#### call

Sends a non state modifying call to EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.txRaw` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** RLP encoded hex string
    -   `args.senderThe` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** ETH address of an account if tx is not signed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** Hex encoded output

#### create

Creates EVM address from EOS account

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.data` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** Arbitrary string used as salt to generate new address

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOSIO TX Response

#### withdraw

Withdraws token from EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.quantity` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** EOSIO asset type quantity to withdraw (0.0001 EOS)

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOSIO TX Response

#### deposit

Deposits token into EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.fromEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.quantity` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** EOSIO asset type quantity to deposit (0.0001 EOS)
    -   `args.memo` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** Memo to transfer

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOSIO TX Response

#### clearAll

Testing: Clears all data in contract

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOS TX response

#### getTable

Fetches tables based on data

##### Parameters

-   `data`

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOS RPC Get tables row response

#### getAllAddresses

Gets all accounts

##### Parameters

-   `contract`  The EOS contract with EVM deplyoed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;Account>>** all accounts

#### getEthAccount

Gets the on-chain account

##### Parameters

-   `address`  The ETH address in contract
-   `contract`  The EOS contract with EVM deplyoed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;Account>** Account row associated with address

#### getNonce

Gets nonce for given address

##### Parameters

-   `address`  The ETH address in contract
-   `contract`  The EOS contract with EVM deplyoed

Returns **any** Hex-encoded nonce

#### getNonce

Fetches the nonce for an account

##### Parameters

-   `address`  The ETH address in EVM contract

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** Hex encoded nonce

#### getStorageAt

Fetches the on-chain storage value at address and key

##### Parameters

-   `address`  The ETH address in EVM contract
-   `key`  Storage key

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;AccountState>** account state row containing key and value

#### getEthAccountByEosAccount

Gets the on-chain evm account by eos account name

##### Parameters

-   `account`  The EOS contract linked to ETH address

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;Account>**

#### setupEvmContract

Deploy EVM contract to EOS account

##### Parameters

-   `contractDir`  The directory which contains the ABI and WASM
-   `contract`  The EOS contract to deploy EVM to

### EosApi

#### transact

Bundles actions into a transaction to send to EOS Api

##### Parameters

-   `actions`
-   `actionsFull` **[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;any>** EOSIO actions

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EVM receipt and EOS receipt

#### raw

Sends a ETH TX to EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.txRaw` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** RLP encoded hex string
    -   `args.senderThe` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** ETH address of an account if tx is not signed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;EvmResponse>** EVM receipt and EOS receipt

#### call

Sends a non state modifying call to EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.txRaw` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** RLP encoded hex string
    -   `args.senderThe` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** ETH address of an account if tx is not signed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** Hex encoded output

#### create

Creates EVM address from EOS account

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.data` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** Arbitrary string used as salt to generate new address

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOSIO TX Response

#### withdraw

Withdraws token from EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.accountEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.quantity` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** EOSIO asset type quantity to withdraw (0.0001 EOS)

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOSIO TX Response

#### deposit

Deposits token into EVM

##### Parameters

-   `args` **[object](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Object)** Arguments
    -   `args.fromEOSIO` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** account to interact with EVM
    -   `args.quantity` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** EOSIO asset type quantity to deposit (0.0001 EOS)
    -   `args.memo` **[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)** Memo to transfer

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOSIO TX Response

#### clearAll

Testing: Clears all data in contract

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOS TX response

#### getTable

Fetches tables based on data

##### Parameters

-   `data`

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;any>** EOS RPC Get tables row response

#### getAllAddresses

Gets all accounts

##### Parameters

-   `contract`  The EOS contract with EVM deplyoed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;[Array](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Array)&lt;Account>>** all accounts

#### getEthAccount

Gets the on-chain account

##### Parameters

-   `address`  The ETH address in contract
-   `contract`  The EOS contract with EVM deplyoed

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;Account>** Account row associated with address

#### getNonce

Gets nonce for given address

##### Parameters

-   `address`  The ETH address in contract
-   `contract`  The EOS contract with EVM deplyoed

Returns **any** Hex-encoded nonce

#### getNonce

Fetches the nonce for an account

##### Parameters

-   `address`  The ETH address in EVM contract

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;[string](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/String)>** Hex encoded nonce

#### getStorageAt

Fetches the on-chain storage value at address and key

##### Parameters

-   `address`  The ETH address in EVM contract
-   `key`  Storage key

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;AccountState>** account state row containing key and value

#### getEthAccountByEosAccount

Gets the on-chain evm account by eos account name

##### Parameters

-   `account`  The EOS contract linked to ETH address

Returns **[Promise](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/Promise)&lt;Account>**

#### setupEvmContract

Deploy EVM contract to EOS account

##### Parameters

-   `contractDir`  The directory which contains the ABI and WASM
-   `contract`  The EOS contract to deploy EVM to
