# EOSIO EVM JS SDK

### Installation
Requires nodejs and npm installed

```bash
npm install eos-evm-js
```

### ERC-20 Tutorial
```ts
import { EosEvmApi } from 'eos-evm-js'

const api = new EosEvmApi({
  // Ensure the API has console printing enabled
  endpoint: 'https://jungle.eosdac.io',

  // Must match the chain ID the contract is compiled with (1 by default)
  chainId: 1,

  // Enter your own private keys (examples provided)
  ethPrivateKeys: [
    // Public Key: 0xf79b834a37f3143f4a73fc3934edac67fd3a01cd
    '0x8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d',
    // Public Key: 0xab21f17d0c3e30be30e115508643817b297ae8d6
    '0x206cbcc0ccbb96a0df7ba079d25866bd0d52f4248861a0a38bc9bfd58c00556a'
  ],

  // Enter EOS account that contract is at / will be deployed to
  eosContract: '1234test1111',

  // Enter your own private keys (examples provided)
  eosPrivateKeys: [
    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
  ]
})

// Import contract compiled with solc (check eos-evm-js/src/eth-contracts/compile.ts to compile your own)
// We provide compiled ERC20 and ERC721 contracts
const compiledErc20AndErc721 = require('eos-evm-js/dist/lib/eth-contracts/compiled.json')

// Load ETH contracts with abi and bytecode, plus the TX sending EOS account
api.loadContractFromAbi({
  account: 'vestvestvest', // Example EOS account
  abi: compiledErc20AndErc721.contracts.ERC20.Token.abi,
  bytecodeObject: compiledErc20AndErc721.contracts.ERC20.Token.evm.bytecode.object
})

async function main () {
  // Deploy EVM contract to EOSIO (deploys to eosContract provided in new EosEvmApi)
  await api.eos.setupEvmContract()

  // For development (if TESTING is enabled in contract), clears all data in contract
  // await api.eos.clearAll()

  // Creates new address based on RLP(eosaccount, arbitrarydata)
  await api.eos.create({ account: '1234test1111', data: 'test' })

  // Transfer EOS to contract to deposit to address
  await api.eos.deposit({ from: 'vestvestvest', quantity: '0.0001 EOS' })

  // Create new contract account with ERC20 contract deployed (Name, Symbol, Decimals, Total Supply)
  await api.eth.deploy('Syed Token', 'SYED', 4, 1000000)

  // Transfer ERC20 tokens
  // The returned response "eth" is the EVM transaction receipt, and "eos" is the EOS transaction receipt
  const { eth, eos } = await api.eth.transfer(receiver, 1000)

  // Query ERC20 balance using "view" function calls
  console.log(+await api.eth.balanceOf(sender).toString(10)) // 999,000
  console.log(+await api.eth.balanceOf(receiver).toString(10)) // 1,000

  // Set allowance, and modify it
  await api.eth.approve(receiver, 100)
  await api.eth.increaseAllowance(receiver, 1000)
  await api.eth.decreaseAllowance(receiver, 600)

  // Query it (another example of using non-state modifying calls)
  const allowance = await api.eth.allowance(sender, allowanceAddress)
  console.log(+allowance.toString(10)) // 500

  // Use the allowance to transfer
  await api.eth.transferFrom(sender, receiver, 500, { sender: receiver })
}

main()
```

### Further documentation

### Disclosure
This repository is in a highly iterative state, please use at your own risk.