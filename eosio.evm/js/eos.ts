import { Api, JsonRpc } from '@jafri/eosjs2';
import fetch from 'node-fetch';
import { TextEncoder, TextDecoder } from '@bloks/utils'
import { JsSignatureProvider } from '@jafri/eosjs2/dist/eosjs-jssig';

import Debug from 'debug';
const debug = Debug("eos-db")

const chainIds = {
  mainnet: 'aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906',
  jungle: 'e70aaab8997e1dfce58fbfac80cbbb8fecec7b99cf982a9444273cbc64c41473',
  kylin: '5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191',
  local: 'cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f'
}

const endpoints = {
  jungle: ['https://api.jungle.alohaeos.com'],
  local: ['http://0.0.0.0:8888']
}

const { EOSIO_NETWORK } = process.env;

const chainId = chainIds[EOSIO_NETWORK];
process.env.EOSIO_CHAIN_ID = chainId // For tests

const rpc = new JsonRpc(endpoints[EOSIO_NETWORK] || endpoints.local, { fetch });

const privateKeys = [
  '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3', // TEST ONLY,
  '5K93jQD9fruYKPpifxD6CrSNw1G3kUb1QjekzR3CkQaBvcq5JJq' // TEST ONLY
]
export const publicKeys = [
  'EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV', // TEST ONLY
  'EOS8TCgBBd6LK6m6auC7T6sDcpD1JvhGdzpUaJQ5ZZb7XPWgtwceL' // TEST ONLY
]
const signatureProvider = new JsSignatureProvider(privateKeys);

export const eos = new Api({
  rpc,
  chainId,
  signatureProvider,
  textEncoder: new TextEncoder() as any,
  textDecoder: new TextDecoder() as any,
});

export const getTable = async (data) => {
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
  return await eos.rpc.get_table_rows(params)
}

export const createAction = (account, name, data, authorization) => {
  const result = {
    account,
    name,
    data,
    authorization
  }
  debug(result, { depth: null })
  return result
}

export const transact = async (actions) => {
  const tx = await eos.transact({
    actions
  }, {
    blocksBehind: 3,
    expireSeconds: 3000,
    broadcast: false,
    sign: true
  })
  try {
    const result = await eos.pushSignedTransaction(tx)
    return result
  } catch (e) {
    console.log(JSON.stringify(e.json, null, 2))
    if (e.json.error.details[0].message == "Invalid packed transaction") {
      console.log(JSON.stringify(tx))
    }
    let message = e
    if (e.json !== undefined) {
      message = e.json.error.details[0].message
      message = message.replace("assertion failure with message: ", "")
    }
    throw e

    // throw new Error(message)
  }
}