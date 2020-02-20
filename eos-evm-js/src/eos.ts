import { Api, JsonRpc, Serialize } from '@jafri/eosjs2';
import { JsSignatureProvider } from '@jafri/eosjs2/dist/eosjs-jssig';
import fetch from 'node-fetch';
import { TextEncoder, TextDecoder } from 'text-encoding'
import { readFileSync } from 'fs'
import { getDeployableFilesFromDir } from './deploy'
import { EOSIO_TOKEN, EOSIO_SYSTEM } from './constants';

export class EosApi {
  eosPrivateKeys: string[];
  signatureProvider: any;
  rpc: any;
  api: any;

  constructor ({ eosPrivateKeys, endpoint }: { eosPrivateKeys: string[], endpoint: string } ) {
    this.eosPrivateKeys = eosPrivateKeys;
    this.signatureProvider = new JsSignatureProvider(this.eosPrivateKeys)
    this.rpc = new JsonRpc(endpoint, { fetch: fetch as any });
    this.api = new Api({
      rpc: this.rpc,
      signatureProvider: this.signatureProvider,
      textEncoder: new TextEncoder() as any,
      textDecoder: new TextDecoder() as any,
    });
  }

  /**
   * Actions
   */
  async transact (actions: any[]) {
    try {
      const result = await this.api.transact({
        actions
      }, {
        blocksBehind: 3,
        expireSeconds: 3000,
        broadcast: true,
        sign: true
      })
      return result
    } catch (e) {
      if (e.json) {
        console.log(e.json.error.details[0].message)
      } else {
        console.dir(e, { depth: null })
      }
      throw e
    }
  }

  async raw ({ contract, account, tx, sender } : { contract: string, account: string, tx: string, sender: string | undefined }) {
    if (tx && tx.startsWith('0x')) tx = tx.substr(2)
    if (sender && sender.startsWith('0x')) sender = sender.substr(2)

    return await this.transact([
      {
        account: contract,
        name: "raw",
        data: {
          tx,
          sender
        },
        authorization: [{ actor: account, permission: "active" }]
      }
    ])
  }

  async create ({ contract, account, data } : { contract: string, account: string, data: string }) {
    return await this.transact([
      {
        account: contract,
        name: "create",
        data: {
          account,
          data
        },
        authorization: [{ actor: account, permission: "active" }]
      }
    ])
  }

  async withdraw ({ contract, account, to, quantity } : { contract: string, account: string, to: string, quantity: string }) {
    return await this.transact([
      {
        account: contract,
        name: "withdraw",
        data: {
          to,
          quantity
        },
        authorization: [{ actor: account, permission: "active" }]
      }
    ])
  }

  async transfer ({ from, to, quantity, memo } : { from: string, to: string, quantity: string, memo: string }) {
    return await this.transact([
      {
        account: EOSIO_TOKEN,
        name: "transfer",
        data: {
          from,
          to,
          quantity,
          memo
        },
        authorization: [{ actor: from, permission: "active" }]
      }
    ])
  }

  /**
   * Testing
   */
  async clearAll ({ contract } : { contract: string }) {
    return await this.transact([
      {
        account: contract,
        name: "clearall",
        data: {},
        authorization: [{ actor: contract, permission: "active" }]
      }
    ])
  }

  /**
   * Fetching
   */
  async getTable (data: any) {
    const defaultParams = {
      json: true,                 // Get the response as json
      code: '',                   // Contract that we target
      scope: '',                  // Account that owns the data
      table: '',                  // Table name
      key_type: `i64`,            // Type of key
      index_position: 1,          // Position of index
      lower_bound: '',            // Table secondary key value
      limit: 10,                  // Here we limit to 10 to get ten row
      reverse: false,             // Optional: Get reversed data
      show_payer: false,          // Optional: Show ram payer
    }
    const params = Object.assign({}, defaultParams, data)
    return await this.api.rpc.get_table_rows(params)
  }

  /**
   * Gets the on-chain account
   *
   * @param contract The EOS contract with EVM deplyoed
   * @param address The ETH address in contract
   *
   * @returns Full EOS table row
   */
  async getAddress (contract: string, address: string) {
    if (!address) return {}
    if (address && address.startsWith('0x')) address = address.substr(2)
    const padded = '0'.repeat(12 * 2) + address;

    const { rows } = await this.getTable({
      code: contract,
      scope: contract,
      table: 'account',
      key_type: 'sha256',
      index_position: 2,
      lower_bound: padded,
      upper_bound: padded,
      limit: 1
    })
    return (rows.length && rows[0].address === address && rows[0]) || {}
  }

  /**
   * Gets the on-chain account
   *
   * @param contract The EOS contract with EVM deplyoed
   * @param address The ETH address in contract
   *
   * @returns Full EOS table row
   */
  async getKey (contract: string, address: string, key: string) {
    if (address && address.startsWith('0x')) address = address.substr(2)
    if (key && key.startsWith('0x')) key = key.substr(2)

    const padded = '0'.repeat(12 * 2) + address;

    const { rows } = await this.getTable({
      code: contract,
      scope: contract,
      table: 'account',
      key_type: 'sha256',
      index_position: 2,
      lower_bound: padded,
      upper_bound: padded,
      limit: 1
    })
    return (rows.length && rows[0].address === address && rows[0]) || {}
  }

  /**
   * Gets nonce for given address
   *
   * @param contract The EOS contract with EVM deplyoed
   * @param address The ETH address in contract
   *
   * @returns Hex-encoded nonce
   */
  async getNonce (contract: string, address: string | undefined) {
    if (!address) return '0x0'

    let nonce = ''
    const account = await this.getAddress(contract, address)

    if (account) {
      nonce = `0x${account.nonce}`
    } else {
      nonce = '0x0'
    }

    return nonce;
  }

  /**
   * Deploy EVM contract to EOS account
   *
   * @param contract The EOS contract to deploy EVM to
   * @param contractDir The directory which contains the ABI and WASM
   *
   */
  async setupEvmContract({ account, contractDir } : { account: string, contractDir: string }) {
    const { wasmPath, abiPath } = getDeployableFilesFromDir(contractDir)

    // 1. Prepare SETCODE
    // read the file and make a hex string out of it
    const wasm = readFileSync(wasmPath).toString(`hex`)

    // 2. Prepare SETABI
    const buffer = new Serialize.SerialBuffer({
      textEncoder: this.api.textEncoder,
      textDecoder: this.api.textDecoder,
    })

    let abi = JSON.parse(readFileSync(abiPath, `utf8`))
    const abiDefinition = this.api.abiTypes.get(`abi_def`)
    // need to make sure abi has every field in abiDefinition.fields
    // otherwise serialize throws
    abi = abiDefinition.fields.reduce(
      (acc: any, { name: fieldName }: any) =>
        Object.assign(acc, { [fieldName]: acc[fieldName] || [] }),
      abi
    )
    abiDefinition.serialize(buffer, abi)

    // 3. Set code
    try {
      await this.transact([
        {
          account: EOSIO_SYSTEM,
          name: 'setcode',
          authorization: [
            {
              actor: account,
              permission: 'active',
            },
          ],
          data: {
            account: account,
            vmtype: 0,
            vmversion: 0,
            code: wasm,
          },
        }
      ])
    } catch (e) {
      console.log('Set code failed')
    }

    // Set ABI
    try {
      await this.transact([
        {
          account: EOSIO_SYSTEM,
          name: 'setabi',
          authorization: [
            {
              actor: account,
              permission: 'active',
            },
          ],
          data: {
            account: account,
            abi: Buffer.from(buffer.asUint8Array()).toString(`hex`),
          },
        }
      ])
    } catch (e) {
      console.log('Set abi failed')
    }
  }
}