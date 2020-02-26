import { Transaction } from 'ethereumjs-tx'
import Common from 'ethereumjs-common'
import util from 'ethereumjs-util'
import { EosApi } from './eos'
import {
  ETH_CHAIN,
  FORK,
  EOSIO_TOKEN_PRECISION,
  DEFAULT_GAS_PRICE,
  DEFAULT_GAS_LIMIT,
  DEFAULT_CHAIN_ID,
  DEFAULT_VALUE,
  DEFAULT_SYMBOL
} from './constants'
const abiEncoder = require('ethereumjs-abi')
export class EosEvmApi {
  ethPrivateKeys: any
  chainId: any
  chainConfig: any
  abi: any
  eth: any
  ethContract: string | undefined
  eos: EosApi

  constructor({
    ethPrivateKeys,
    eosPrivateKeys,
    endpoint,
    eosContract,
    ethContract,
    chainId = DEFAULT_CHAIN_ID
  }: {
    ethPrivateKeys?: any
    eosPrivateKeys: string[]
    endpoint: string
    eosContract: string
    ethContract?: string
    chainId: number
  }) {
    this.eos = new EosApi({ eosPrivateKeys, endpoint, eosContract })
    this.chainId = chainId
    this.ethContract = ethContract
    this.chainConfig = Common.forCustomChain(ETH_CHAIN, { chainId }, FORK)

    this.ethPrivateKeys = ethPrivateKeys.reduce((acc: any, privateKey: string) => {
      if (privateKey.substr(0, 2) === '0x') {
        privateKey = privateKey.substring(2)
      }
      const privateBuffer = Buffer.from(privateKey, 'hex')
      const address = `0x${util.privateToAddress(privateBuffer).toString('hex')}`
      acc[address] = privateBuffer
      return acc
    }, {})
  }

  /**
   * Sets the address for ethereum contract
   *
   * @param address ethereum contract address
   */
  async setEthereumContract({ address }: { address: string }) {
    this.ethContract = address
  }

  /**
   * Initializes Web3 like interface to send actions to EVM
   *
   * @param account EOSIO account to interact with EVM
   * @param sender The EVM address sending the transaction
   * @param to The EVM address with the contract
   * @param abi ABI object
   * @param bytecode Bytecode object
   */
  async loadContractFromAbi({
    account,
    sender = '',
    abi,
    bytecodeObject
  }: {
    account: string
    sender?: string
    abi: any
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
        const types = action.inputs.map((i: any) => i.type)
        const names = action.inputs.map((i: any) => i.name)
        const outputTypes = action.outputs.map((i: any) => i.type)

        // Default
        let overrides = {}

        // Validation
        if (args.length === types.length + 1 && typeof args[args.length - 1] === 'object') {
          overrides = args[args.length - 1]
          args.pop()
        }
        if (args.length !== types.length) {
          throw new Error(
            `${types.length} arguments expected for function ${action.name}: ${names}`
          )
        }
        if (!that.ethContract) {
          throw new Error(
            'Please initialize loadContractFromAbi with ethContract or deploy() to insert automatically'
          )
        }

        // Encode
        const methodID = abiEncoder.methodID(action.name, types).toString('hex')
        const params = abiEncoder.rawEncode(types, args).toString('hex')
        const input = `0x${methodID}${params}`

        // If call (non state modifying)
        if (action.stateMutability && action.stateMutability === 'view') {
          // Create call object
          const txParams = Object.assign(
            { sender, data: input, to: that.ethContract, sign: false },
            overrides
          )
          const encodedTx = await that.createEthTx(txParams)

          // Get output from call and aprse it
          const output = await that.eos.call({ account, tx: encodedTx, sender })
          const parsed = abiEncoder.rawDecode(outputTypes, Buffer.from(output, 'hex'))
          return parsed
        }
        // If transaction (standard transaction)
        else {
          // Create transaction object
          const txParams = Object.assign({ sender, data: input, to: that.ethContract }, overrides)
          const encodedTx = await that.createEthTx(txParams)

          // Send transaction
          return that.eos.raw({ account, tx: encodedTx, sender })
        }
      }
    }

    eth['deploy'] = async function(...args: any[]) {
      const types = abiInterface.constructor[0].inputs.map((i: any) => i.type)
      const names = abiInterface.constructor[0].inputs.map((i: any) => i.name)

      // Default
      let overrides = {}

      // Validation
      if (args.length === types.length + 1 && typeof args[args.length - 1] === 'object') {
        overrides = args[args.length - 1]
        args.pop()
      }
      if (args.length != types.length) {
        throw new Error(`${types.length} arguments expected for deploy: ${names}`)
      }

      // Encode params
      const params = abiEncoder.rawEncode(types, args).toString('hex')
      const data = `0x${bytecodeObject}${params.toString('hex')}`

      // Create transaction and send it
      const txParams = Object.assign({ sender, data, to: undefined }, overrides)
      const encodedTx = await that.createEthTx(txParams)
      const result = await that.eos.raw({ account, tx: encodedTx, sender })

      // Set current contract address to the newly created address
      if (result.eth && result.eth.createdAddress) {
        that.ethContract = `0x${result.eth.createdAddress}`
        console.warn('The contract was deployed to address: ', that.ethContract)
      }
      return result
    }

    this.eth = eth
  }

  /**
   * Transfers value inside EVM
   *
   * @param account The EOS account associated to ETH address
   * @param sender The ETH address sending the TX
   * @param to The ETH address sending the transaction (nonce is fetched on-chain for this address)
   * @param quantity EOSIO quantity
   * @param ethSign Whether to sign transaction with ethereum private key
   *
   */
  async transfer({
    account,
    sender,
    to,
    quantity,
    ethSign = false
  }: {
    account: string
    sender: string
    to: string
    quantity: string
    ethSign?: boolean
  }) {
    const [amount, symbol] = quantity.split(' ')
    if (symbol !== DEFAULT_SYMBOL)
      throw new Error('Must provide asset as quantity to transfer like 0.0001 SYS')

    const tx = await this.createEthTx({
      sender,
      to,
      value: +amount * Math.pow(10, EOSIO_TOKEN_PRECISION),
      sign: ethSign
    })
    return this.eos.raw({ account, tx, sender })
  }

  /**
   * Generates RLP encoded transaction sender parameters
   *
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
    sender,
    data,
    gasLimit,
    value,
    to,
    sign = true
  }: {
    sender?: string
    data?: string
    gasLimit?: string | Buffer
    value?: number | Buffer
    to?: string
    sign?: boolean
  }) {
    const nonce = await this.eos.getNonce(sender)
    const txData = {
      nonce,
      gasPrice: DEFAULT_GAS_PRICE,
      gasLimit: gasLimit !== undefined ? `0x${(gasLimit as any).toString(16)}` : DEFAULT_GAS_LIMIT,
      value: value !== undefined ? `0x${(value as any).toString(16)}` : DEFAULT_VALUE,
      to,
      data
    }

    const tx = new Transaction(txData, { common: this.chainConfig })

    if (sign) {
      if (!sender) throw new Error('Signature requested in createEthTx, but no sender provided')
      if (!this.ethPrivateKeys[sender]) {
        console.log(this.ethPrivateKeys)
        throw new Error('No private key provided for ETH address ' + sender)
      }
      tx.sign(this.ethPrivateKeys[sender])
    }

    return tx.serialize().toString('hex')
  }
}
