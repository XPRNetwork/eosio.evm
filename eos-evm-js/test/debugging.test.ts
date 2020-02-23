import { api } from './common'

const compiled = require('../src/eth-contracts/compiled.json')

const contract = '1234test1111'
const account = 'vestvestvest'
const contractDir = '/Users/jafri/eosio.evm/eosio.evm'
const sender = '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd'
const ethContract = '0xb3e48339798967507eeda1773e824255ac7c6258'

// [ 'nonce',
// 'gasPrice',
// 'gasLimit',
// 'to',
// 'value',
// 'data',
// 'v',
// 'r',
// 's' ],

api.loadContractFromAbi({
  contract,
  account,
  sender,
  to: ethContract,
  abi: compiled.contracts.ERC20.Token.abi,
  bytecodeObject: compiled.contracts.ERC20.Token.evm.bytecode.object
})

describe('Debug', () => {
  it('test', async done => {
    console.dir(await api.eth.decreaseAllowance(sender, 1000), { depth: null })
  })
})
