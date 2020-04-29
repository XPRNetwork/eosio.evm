import { EosEvmApi } from '../src/eos-evm-js'
const BN = require('bn.js')
const compiled = require('../src/eth-contracts/compiled.json')

// Change accordingly
export const contractDir = '/Users/jafri/eosio.evm/eosio.evm'

export const ethContractAddress = '0xb3e48339798967507eeda1773e824255ac7c6258'
export const contract = 'evmcontract2'
export const account = 'vestvestvest'
export const sender = '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd'
export const allowanceAddress = '0xab21f17d0c3e30be30e115508643817b297ae8d6'
export const initialAccount = {
  account,
  address: '0x10d10ef03ef3316b750d2544bae8c96309aa4360',
  balance: new BN(0),
  code: [],
  index: 0,
  nonce: 1
}

export const api = new EosEvmApi({
  endpoint: 'https://jungle.eosdac.io',
  chainId: 1,
  eosContract: contract,
  ethPrivateKeys: [
    '0x8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d', // 0xf79b834a37f3143f4a73fc3934edac67fd3a01cd
    '0x206cbcc0ccbb96a0df7ba079d25866bd0d52f4248861a0a38bc9bfd58c00556a' // 0xab21f17d0c3e30be30e115508643817b297ae8d6
  ],
  eosPrivateKeys: [
    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    '5K93jQD9fruYKPpifxD6CrSNw1G3kUb1QjekzR3CkQaBvcq5JJq',
    '5JACk8gJ98x3AkypbicNQviXzkqAM2wbbE3FtA79icT2Ks4bWws'
  ],
  ethContract: ethContractAddress
})

api.loadContractFromAbi({
  account,
  abi: compiled.contracts.ERC20.Token.abi,
  bytecodeObject: compiled.contracts.ERC20.Token.evm.bytecode.object
})
