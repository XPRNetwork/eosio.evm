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
        '60003560005260203560205260403560405260603560605260803560805260a03560a05260c03560c052604061012c60806000600060066207a120f2600055604061019060606080600060076207a120f260015561012c51600a5561014c51600b55610190516014556101b051601555601454600a5414600255601554600b541460035500',
        0
      )
      expect(true).toBeTruthy()
    })

    it(`TX`, async () => {
      await api.eos.raw({
        account: 'vestvestvest',
        tx:
          'f901410101831e848094b94f5374fce5edbc8e2a8697c15331677e6ebf0b80b8e00f25929bcb43d5a57391564615c9e70a992b10eafa4db109709649cf48c50dd216da2f5cb6be7a0aa72c440c53c9bbdfec6c36c7d515536431b3a865468acbba0f25929bcb43d5a57391564615c9e70a992b10eafa4db109709649cf48c50dd216da2f5cb6be7a0aa72c440c53c9bbdfec6c36c7d515536431b3a865468acbba0f25929bcb43d5a57391564615c9e70a992b10eafa4db109709649cf48c50dd216da2f5cb6be7a0aa72c440c53c9bbdfec6c36c7d515536431b3a865468acbba00000000000000000000000000000000000000000000000000000000000000021ba0beac1d71d2988319b020080b85843ea5f83297a099139e4c73791b17bb73f536a0253f16ee8623833382f778d264e8b9dbf9926ac9c4998b25192beaab312626ef'
      })
      expect(true).toBeTruthy()
    })
  })
})
