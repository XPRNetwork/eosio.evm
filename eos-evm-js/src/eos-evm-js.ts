import { Transaction } from 'ethereumjs-tx'
import Common from 'ethereumjs-common'
import { EosApi } from './eos'
const abiEncoder = require('ethereumjs-abi')

export class EosEvmApi extends EosApi {
  ethPrivateKeys: any
  chainId: any
  chainConfig: any
  abi: any
  eth: any

  constructor({
    ethPrivateKeys,
    eosPrivateKeys,
    endpoint,
    chainId = 1
  }: {
    ethPrivateKeys?: any
    eosPrivateKeys: string[]
    endpoint: string
    chainId: number
  }) {
    super({ eosPrivateKeys, endpoint })
    this.ethPrivateKeys = ethPrivateKeys
    this.chainId = chainId
    this.chainConfig = Common.forCustomChain('mainnet', { chainId }, 'istanbul')
  }

  async loadContractFromAbi({
    contract,
    account,
    sender,
    to,
    abi,
    bytecodeObject
  }: {
    contract: string
    account: string
    sender: string
    abi: any
    to: string
    bytecodeObject: string
  }) {
    // Load interface
    let abiInterface: any = {
      function: [],
      event: [],
      constructor: []
    }
    for (const item of abi) {
      abiInterface[item.type].push(item)
    }
    this.abi = abi

    const that = this
    let eth: any = {}

    // Populate functions
    for (const action of abiInterface.function) {
      eth[action.name] = async function(...args: any[]) {
        const internalTypes = action.inputs.map((i: any) => i.internalType)
        const names = action.inputs.map((i: any) => i.name)
        const types = action.inputs.map((i: any) => i.type)
        if (args.length != internalTypes.length)
          throw new Error(
            `${internalTypes.length} arguments expected for function ${action.name}: ${names}`
          )

        const params = abiEncoder.rawEncode(internalTypes, args).toString('hex')
        const data = `0x${params}`
        const encodedTx = await that.createEthTx({ contract, sender, data, to })

        // return that.raw({ contract, account, tx: encodedTx, sender })
      }
    }

    eth['deploy'] = async function(...args: any[]) {
      const internalTypes = abiInterface.constructor[0].inputs.map((i: any) => i.internalType)
      const names = abiInterface.constructor[0].inputs.map((i: any) => i.name)
      if (args.length != internalTypes.length)
        throw new Error(`${internalTypes.length} arguments expected for deploy: ${names}`)

      const params = abiEncoder.rawEncode(internalTypes, args).toString('hex')
      const data = `0x${bytecodeObject}${params.toString('hex')}`
      const encodedTx = await that.createEthTx({ contract, sender, data, to: undefined })
      // return that.raw({ contract, account, tx: encodedTx, sender })
    }

    this.eth = eth
  }

  /**
   * Generates RLP encoded transaction from parameters
   *
   * @param contract The EOS account where the EVM contract is deployed
   * @param sender The ETH address sending the transaction (nonce is fetched on-chain for this address)
   * @param data The data in transaction
   * @param gasLimit The gas limit of the transaction
   * @param value The value in the transaction
   * @param to The ETH address to send transaction to
   * @param sign Whether to sign the transaction
   *
   * @returns RLP encoded transaction
   */
  async createEthTx({
    contract,
    sender,
    data,
    gasLimit,
    value,
    to,
    sign = true
  }: {
    contract: string
    sender?: string
    data?: string
    gasLimit?: string
    value?: string
    to?: string
    sign?: boolean
  }) {
    const nonce = await this.getNonce(contract, sender)
    const txData = {
      nonce,
      gasPrice: '0x01',
      gasLimit: gasLimit || '0x1E8480',
      value: value || '0x0',
      to,
      data
    }

    const tx = new Transaction(txData, { common: this.chainConfig })

    if (sign) {
      if (!sender) throw new Error('Signature requested in createEthTx, but no sender provided')
      if (!this.ethPrivateKeys[sender])
        throw new Error('No private key provided for ETH address ' + sender)
      tx.sign(this.ethPrivateKeys[sender])
    }
    console.log(tx.toJSON(), tx)
    return

    return tx.serialize().toString('hex')
  }
}
