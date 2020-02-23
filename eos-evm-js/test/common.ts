import { EosEvmApi } from '../src/eos-evm-js'

export const api = new EosEvmApi({
  ethPrivateKeys: {
    '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd': Buffer.from(
      '8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d',
      'hex'
    ),
    '0xab21f17d0c3e30be30e115508643817b297ae8d6': Buffer.from(
      '206cbcc0ccbb96a0df7ba079d25866bd0d52f4248861a0a38bc9bfd58c00556a',
      'hex'
    )
  },
  eosPrivateKeys: [
    '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    '5K93jQD9fruYKPpifxD6CrSNw1G3kUb1QjekzR3CkQaBvcq5JJq'
  ],
  endpoint: 'https://api.jungle.alohaeos.com',
  chainId: 1
})
