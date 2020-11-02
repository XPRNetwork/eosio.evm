import { BN } from 'ethereumjs-util'

export interface Account {
  index: number
  address: string
  balance: BN
  code: string | Buffer
  nonce: number
  account: string
}

export interface AccountState {
  index: number
  key: string
  value: string
}

export interface Log {
  address: string
  data: string
  topics: string[]
}
export interface EvmReceipt {
  status: string
  from: string
  to: string
  value: number
  nonce: number
  v: number
  r: string
  s: string
  createdAddress: string
  gasUsed: number
  gasLimit: number
  gasPrice: number
  logs: Log[]
  output: string
  errors: string[]
  transactionHash: string
}

export interface EvmResponse {
  eth: EvmReceipt
  eos: any
}
