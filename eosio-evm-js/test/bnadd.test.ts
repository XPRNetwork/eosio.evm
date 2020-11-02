import { api, initialAccount, contractDir, contract } from './common'
const BN = require('bn.js')

describe('Full ERC20 Test', () => {
  jest.setTimeout(30000)

  describe('Create Address and deposit EOS', () => {
    it('clears all (dev only, remove in prod)', async () => {
      await api.eos.clearAll()
      const rows = await api.eos.getAllAddresses()
      expect(rows.length).toEqual(0)
    })

    it(`setups contract at ${contract}`, async () => {
      await api.eos.setupEvmContract(contractDir)
      expect(await api.eos.rpc.getRawAbi(contract)).toBeTruthy()
    })

    it(`Pre-create accounts`, async () => {
      await api.eos.devNewAccount(
        'a94f5374fce5edbc8e2a8697c15331677e6ebf0b',
        '1000000000000000000',
        '',
        1
      )
      await api.eos.devNewAccount(
        'b94f5374fce5edbc8e2a8697c15331677e6ebf0b',
        '0',
        '600035600052602035602052604035604052606035606052604060c860806000600060066207a120f260005560c85160015560e85160025500',
        0
      )
      expect(true).toBeTruthy()
    })

    it(`TX`, async () => {
      await api.eos.raw({
        account: 'vestvestvest',
        tx:
          'f8e10101830f424094b94f5374fce5edbc8e2a8697c15331677e6ebf0b80b88000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000021ca00a52e2926d72b8467dbdd0d67ef152154b16d0fb1e3d36faf726e6e013fadfd9a0560599144485be9409ff55c226a3036efe14ebc2d7170d3093b76a7295b6fb2c'
      })
      expect(true).toBeTruthy()
    })
  })
})
