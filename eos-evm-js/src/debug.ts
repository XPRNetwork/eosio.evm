const { EosEvmApi } = require('./eos-evm-js')
const compiled = require('./eth-contracts/compiled.json')

const api = new EosEvmApi({
  ethPrivateKeys: {
    '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd': Buffer.from(
      '8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d',
      'hex'
    )
  },
  eosPrivateKeys: [
    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    '5K93jQD9fruYKPpifxD6CrSNw1G3kUb1QjekzR3CkQaBvcq5JJq'
  ],
  endpoint: 'https://api.jungle.alohaeos.com'
})
api.loadContractFromAbi({
  contract: '1234test1111',
  account: 'vestvestvest',
  sender: '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd',
  to: '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd',
  abi: compiled.contracts.ERC20.Token.abi
})

console.log(api.eth.allowance('hi', 333).then((i: any) => console.log(i)))
