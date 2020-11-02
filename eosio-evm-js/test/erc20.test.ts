// The MIT License (MIT)
// Copyright (c) 2016-2019 zOS Global Limited
// Copyright (c) 2020 Syed Jafri

import {
  api,
  contract,
  initialAccount,
  account,
  sender,
  allowanceAddress,
  contractDir
} from './common'
const BN = require('bn.js')

describe('Full ERC20 Test', () => {
  describe('Setup', () => {
    it('clears all (dev only, remove in prod)', async () => {
      await api.eos.clearAll()
      const rows = await api.eos.getAllAddresses()
      expect(rows.length).toEqual(0)
    })

    it(`setups contract at ${contract}`, async () => {
      jest.setTimeout(30000)
      await api.eos.setupEvmContract(contractDir)
      expect(await api.eos.rpc.getRawAbi(contract)).toBeTruthy()
    })
  })

  describe('Create Address and deposit EOS', () => {
    it('creates new address based on RLP(eosaccount, arbitrary)', async () => {
      await api.eos.create({ account, data: 'test' })
      const rows = await api.eos.getAllAddresses()
      expect(
        rows.map((x: any) => {
          x.balance = x.balance.toString('hex')
          return x
        })
      ).toEqual([{ ...initialAccount, balance: initialAccount.balance.toString('hex') }])
    })

    it('transfer EOS to contract to deposit to address', async () => {
      const quantity = '0.0002 EOS'
      await api.eos.deposit({ from: account, quantity })
      const rows = await api.eos.getAllAddresses()
      expect(
        rows.map((x: any) => {
          x.balance = x.balance.toString('hex')
          return x
        })
      ).toEqual([{ ...initialAccount, balance: new BN(0.0002 * Math.pow(10, 18)).toString('hex') }])
    })

    it('transfer in EVM from new address to other addresses', async () => {
      await api.transfer({
        account,
        sender: initialAccount.address,
        to: sender,
        quantity: '0.0001 EOS'
      })
      await api.transfer({
        account,
        sender: initialAccount.address,
        to: allowanceAddress,
        quantity: '0.0001 EOS'
      })
      const rows = await api.eos.getAllAddresses()
      expect(
        rows.map((x: any) => {
          x.balance = x.balance.toString('hex')
          return x
        })
      ).toEqual([
        {
          account,
          address: initialAccount.address,
          balance: new BN(0).toString('hex'),
          code: [],
          index: 0,
          nonce: 3
        },
        {
          account: '',
          address: sender,
          balance: new BN(0.0001 * Math.pow(10, 18)).toString('hex'),
          code: [],
          index: 1,
          nonce: 0
        },
        {
          account: '',
          address: allowanceAddress,
          balance: new BN(0.0001 * Math.pow(10, 18)).toString('hex'),
          code: [],
          index: 2,
          nonce: 0
        }
      ])
    })
  })

  describe('ERC20 Functions', () => {
    describe('Create Syed Token', () => {
      it('Deploy ERC20 (constructor)', async () => {
        await api.eth.deploy('Syed Token', 'SYED', 8, 1000000, { sender, rawSign: true })
        const balance = await api.eth.balanceOf(sender)
        expect(+balance.toString(10)).toEqual(1000000)
      })
    })

    describe('Transfer', () => {
      it('Transfers SYED tokens', async () => {
        const { eth, eos } = await api.eth.transfer(initialAccount.address, 1000, {
          sender,
          rawSign: true
        })

        // Validate
        const senderBalance = await api.eth.balanceOf(sender)
        expect(+senderBalance.toString(10)).toEqual(1000000 - 1000)
        const receiverBalance = await api.eth.balanceOf(initialAccount.address)
        expect(+receiverBalance.toString(10)).toEqual(1000)
      })
    })

    describe('Allow', () => {
      it(`Allow ${allowanceAddress} 100 SYED`, async () => {
        const { eth, eos } = await api.eth.approve(allowanceAddress, 100, { sender, rawSign: true })
        const allowance = await api.eth.allowance(sender, allowanceAddress)
        expect(+allowance.toString(10)).toEqual(100)
      })

      it('Increase Allowance', async () => {
        const { eth, eos } = await api.eth.increaseAllowance(allowanceAddress, 1000, {
          sender,
          rawSign: true
        })
        const allowance = await api.eth.allowance(sender, allowanceAddress)
        expect(+allowance.toString(10)).toEqual(1100)
      })

      it('Decrease Allowance', async () => {
        const { eth, eos } = await api.eth.decreaseAllowance(allowanceAddress, 600, {
          sender,
          rawSign: true
        })
        const allowance = await api.eth.allowance(sender, allowanceAddress)
        expect(+allowance.toString(10)).toEqual(500)
      })

      it('TransferFrom Allowed', async () => {
        const { eth, eos } = await api.eth.transferFrom(sender, allowanceAddress, 500, {
          sender: allowanceAddress,
          rawSign: true
        })

        // Validate
        const senderBalance = await api.eth.balanceOf(sender)
        expect(+senderBalance.toString(10)).toEqual(1000000 - 1000 - 500)
        const receiverBalance = await api.eth.balanceOf(allowanceAddress)
        expect(+receiverBalance.toString(10)).toEqual(500)
      })
    })
  })
})
