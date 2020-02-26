import { EosEvmApi } from '../src/eos-evm-js'
import { allowanceAddress, api, sender, ethContractAddress } from './common'

describe('Debug Test', () => {
  it(`transfer from`, async () => {
    const { eth, eos } = await api.eth.transferFrom(sender, allowanceAddress, 500, {
      sender: allowanceAddress
    })
    console.log(eth)
  })
})
