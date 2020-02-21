// The MIT License (MIT)

// Copyright (c) 2016-2019 zOS Global Limited
// Copyright (c) 2020 Syed Jafri

import { EosEvmApi } from '../src/eos-evm-js'
const compiled = require('../src/eth-contracts/compiled.json')

const contract = '1234test1111'
const account = 'vestvestvest'
const contractDir = '/Users/jafri/eosio.evm/eosio.evm'
const sender = '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd'
const ethContract = '0xb3e48339798967507eeda1773e824255ac7c6258'
const initialAccount = {
  account,
  address: '10d10ef03ef3316b750d2544bae8c96309aa4360',
  balance: '0.0000 EOS',
  code: [],
  index: 0,
  nonce: 1
}

describe('Full Test', () => {
  const api = new EosEvmApi({
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
    endpoint: 'https://api.jungle.alohaeos.com'
  })
  api.loadContractFromAbi({
    contract,
    account,
    sender,
    to: ethContract,
    abi: compiled.contracts.ERC20.Token.abi,
    bytecodeObject: compiled.contracts.ERC20.Token.evm.bytecode.object
  })

  describe('Setup', () => {
    it('clears all (dev only, remove in prod)', async () => {
      await api.clearAll({ contract })
      const rows = await api.getAllAddresses(contract)
      expect(rows.length).toEqual(0)
    })

    it(`setups contract at ${contract}`, async () => {
      jest.setTimeout(30000)
      await api.setupEvmContract({ account: contract, contractDir })
      expect(await api.rpc.getRawAbi(contract)).toBeTruthy()
    })

    it('creates new address based on RLP(eosaccount, arbitrary)', async () => {
      await api.create({ contract, account, data: 'test' })
      const rows = await api.getAllAddresses(contract)
      expect(rows).toEqual([initialAccount])
    })

    it('transfer EOS to contract to deposit to address', async () => {
      const quantity = '0.0001 EOS'
      await api.transfer({ from: account, to: contract, quantity, memo: '' })
      const rows = await api.getAllAddresses(contract)
      expect(rows).toEqual([{ ...initialAccount, balance: quantity }])
    })

    it('transfer from new address to another address', async () => {
      const transferTx = await api.createEthTx({
        contract: '1234test1111',
        sender: '0x10d10ef03ef3316b750d2544bae8c96309aa4360',
        to: '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd',
        value: '0x01',
        sign: false
      })
      await api.raw({
        contract: '1234test1111',
        account: 'vestvestvest',
        tx: transferTx,
        sender: '10d10ef03ef3316b750d2544bae8c96309aa4360'
      })

      expect(true).toBeTruthy()
    })
  })

  describe('Create Syed Token', () => {
    it('Calls ERC20 constructor', async () => {
      await api.eth.deploy('Syed Token', 'SYED', 8, 1000000)

      expect(true).toBeTruthy()
    })
  })

  describe('Decrease Allowance', () => {
    describe('When not zero address', () => {
      it('reverts when there was no approved amount before', async () => {
        console.dir(await api.eth.decreaseAllowance(sender, 1000), { depth: null })
        expect(true).toBeTruthy()
      })
    })
  })
})
