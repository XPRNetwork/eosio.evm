import { EosEvmApi } from "../src/eos-evm-js"

describe("Full Test", () => {
  const api = new EosEvmApi({
    ethPrivateKeys: {
      '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd': Buffer.from('8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d', 'hex')
    },
    eosPrivateKeys: ['5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3', '5K93jQD9fruYKPpifxD6CrSNw1G3kUb1QjekzR3CkQaBvcq5JJq'],
    endpoint: 'https://api.jungle.alohaeos.com'
  })
  const sender = '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd'

  it("clears all (dev only, remove in prod)", async(done) => {
    await api.clearAll({ contract: '1234test1111' })
    expect(true).toBeTruthy()
    done()
  })

  it("setups contract", async(done) => {
    jest.setTimeout(30000);

    await api.setupEvmContract({ account: '1234test1111', contractDir: '/Users/jafri/eosio.evm/eosio.evm' })
    expect(true).toBeTruthy()
    done()
  })

  it("creates new address based on RLP(eosaccount, arbitrary)", async(done) => {
    await api.create({ contract: '1234test1111', account: 'vestvestvest', data: 'test' })
    expect(true).toBeTruthy()
    done()
  })

  it("transfer EOS to contract to deposit to address", async(done) => {
    await api.transfer({ from: 'vestvestvest', to: '1234test1111', quantity: '0.0001 EOS', memo: '' })
    expect(true).toBeTruthy()
    done()
  })

  it("transfer from new address to another address", async(done) => {
    const transferTx = await api.createEthTx({
      sender: '0x10d10ef03ef3316b750d2544bae8c96309aa4360',
      to: '0xf79b834a37f3143f4a73fc3934edac67fd3a01cd',
      value: '0x01',
      sign: false
    })
    await api.raw({ contract: '1234test1111', account: 'vestvestvest', tx: transferTx, sender: '10d10ef03ef3316b750d2544bae8c96309aa4360' })

    expect(true).toBeTruthy()
    done()
  })

  it("Deploys ERC20", async(done) => {
    const erc20Tx = await api.deployERC20({
      sender,
      name: 'Syed Token',
      symbol: 'SYED',
      decimals: 8,
      initialSupply: 1000000
    })
    await api.raw({
      contract: '1234test1111',
      account: 'vestvestvest',
      tx: erc20Tx,
      sender: undefined
    })

    expect(true).toBeTruthy()
    done()
  })

  it("Deploys ERC721", async (done) => {
    const erc721Tx = await api.deployERC721({
      sender,
      name: 'Cryptokitties',
      symbol: 'KITTIES',
    })
    await api.raw({
      contract: '1234test1111',
      account: 'vestvestvest',
      tx: erc721Tx,
      sender: undefined
    })

    expect(true).toBeTruthy()
    done()
  })
})