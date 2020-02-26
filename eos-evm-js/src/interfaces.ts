export interface Account {
  index: number
  address: string
  balance: string
  code: string | Buffer
  nonce: number
  account: string
}

export interface AccountState {
  index: number
  key: string
  value: string
}
