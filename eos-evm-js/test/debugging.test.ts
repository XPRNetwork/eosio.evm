import { EosEvmApi } from '../src/eos-evm-js'
import { allowanceAddress, api, sender, ethContractAddress } from './common'

describe('Debug Test', () => {
  it(`transfer from`, async () => {
    console.log(await api.eos.getEthAccountByEosAccount('vestvestvest'))
  })
})
