import { EosEvmApi } from '../src/eos-evm-js'

const contract = '1234test1111'
const account = 'vestvestvest'
const arbitrary = 'test'
const contractDir = '/Users/jafri/eosio.evm/eosio.evm'
const sender = '0x10d10ef03ef3316b750d2544bae8c96309aa4360'
const receiver = '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd'
const initialAccount = {
  account,
  address: sender.substr(2),
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
      )
    },
    eosPrivateKeys: [
      '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
      '5K93jQD9fruYKPpifxD6CrSNw1G3kUb1QjekzR3CkQaBvcq5JJq'
    ],
    endpoint: 'https://api.jungle.alohaeos.com'
  })

  it('clears all (dev only, remove in prod)', async () => {
    await api.clearAll({ contract })
    const rows = await api.eos.getAllAddresses(contract)
    expect(rows.length).toEqual(0)
  })

  it(`setups contract at ${contract}`, async () => {
    jest.setTimeout(30000)
    await api.eos.setupEvmContract(contractDir)
    expect(await api.eos.rpc.getRawAbi(contract)).toBeTruthy()
  })

  it(`creates new address ${sender.substr(
    0,
    8
  )} based on RLP(${account}, ${arbitrary})`, async () => {
    await api.eos.create({ contract, account, data: arbitrary })
    const rows = await api.eos.getAllAddresses(contract)
    expect(rows).toEqual([initialAccount])
  })

  it(`transfer 0.0001 EOS to contract (${contract}) to deposit for ${sender.substr(
    0,
    8
  )} (${account})`, async () => {
    const quantity = '0.0001 EOS'
    await api.transfer({ from: account, to: contract, quantity, memo: '' })
    const rows = await api.eos.getAllAddresses(contract)
    expect(rows).toEqual([{ ...initialAccount, balance: quantity }])
  })

  it(`transfer from ${sender.substr(0, 8)} to ${receiver.substr(0, 8)}`, async () => {
    const tx = await api.eos.createEthTx({
      contract,
      sender,
      to: receiver,
      value: '0x01',
      sign: false
    })

    await api.raw({ contract, account, tx, sender })
    const rows = await api.eos.getAllAddresses(contract)
    expect(rows).toEqual([
      { ...initialAccount, nonce: 2 },
      {
        account: '',
        address: receiver.substr(2),
        balance: '0.0001 EOS',
        code: [],
        index: 1,
        nonce: 0
      }
    ])
  })

  it('Deploys ERC721', async done => {
    const erc721Tx = await api.deployERC721({
      contract,
      sender: receiver,
      name: 'Cryptokitties',
      symbol: 'KITTIES'
    })
    await api.raw({
      contract,
      account,
      tx: erc721Tx,
      sender: undefined
    })

    expect(true).toBeTruthy()
    done()
  })
})
