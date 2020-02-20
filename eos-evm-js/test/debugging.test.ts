import { EosEvmApi } from "../src/eos-evm-js"

describe("Debug", () => {
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

  it("Deploys ERC721", async (done) => {
    const erc721Tx = await api.deployERC721({
      sender,
      name: 'Cryptokitties',
      symbol: 'KITTIES',
    })
    console.log(erc721Tx)

    expect(true).toBeTruthy()
    done()
  })
})