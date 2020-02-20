import { Transaction } from 'ethereumjs-tx'
import { EosApi } from './eos'
const abi = require('ethereumjs-abi')
const compiled = require('./eth-contracts/compiled.json')

export class EosEvmApi extends EosApi {
  ethPrivateKeys: any;

  constructor ({ ethPrivateKeys, eosPrivateKeys, endpoint } : { ethPrivateKeys?: any, eosPrivateKeys: string[], endpoint: string }) {
    super({ eosPrivateKeys, endpoint })

    this.ethPrivateKeys = ethPrivateKeys;
  }

  /**
   * Generates RLP encoded transaction from parameters
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
  async createEthTx ({ sender, data, gasLimit, value, to, sign = true }: { sender?: string, data?: string, gasLimit?: string, value?: string, to?: string, sign?: boolean }) {
    const nonce = await this.getNonce('1234test1111', sender)
    const txData = {
      nonce,
      gasPrice: '0x01',
      gasLimit: gasLimit || '0x1E8480',
      value: value || '0x0',
      to,
      data
    }

    const tx = new Transaction(txData);

    if (sign) {
      if (!sender) throw new Error('Signature requested in createEthTx, but no sender provided');
      if (!this.ethPrivateKeys[sender]) throw new Error('No private key provided for ETH address ' + sender);
      tx.sign(this.ethPrivateKeys[sender]);
    }

    return tx.serialize().toString('hex');
  }

  /**
   * Gets RLP encoded transaction for the deployment of ERC721 token
   *
   * @param sender The ETH address sending the transaction
   * @param name Name of token
   * @param symbol Symbol of token
   *
   * @returns RLP encoded transaction
   */
  async deployERC721 ({ sender, name, symbol } : { sender: string, name: string, symbol: string }) {
    const params = abi.rawEncode(['string', 'string'], [name, symbol])
    const deployBytecode = compiled.contracts.ERC721.Token.evm.bytecode.object
    const data = `0x${deployBytecode}${params.toString('hex')}`
    return await this.createEthTx({
      sender,
      data
    })
  }

  /**
   * Gets RLP encoded transaction for the deployment of ERC20 token
   *
   * @param sender The ETH address sending the transaction
   * @param name Name of token
   * @param symbol Symbol of token
   * @param decimals The precision of your new token
   * @param initialSupply The initial supply of the token
   *
   * @returns RLP encoded transaction
   */
  async deployERC20 ({ sender, name, symbol, decimals, initialSupply } : { sender: string, name: string, symbol: string, decimals: number, initialSupply: number }) {
    const params = abi.rawEncode(['string', 'string', 'uint8', 'uint256'], [name, symbol, decimals, initialSupply])
    const deployBytecode = compiled.contracts.ERC20.Token.evm.bytecode.object
    const data = `0x${deployBytecode}${params.toString('hex')}`
    return await this.createEthTx({
      sender,
      data
    })
  }
}
