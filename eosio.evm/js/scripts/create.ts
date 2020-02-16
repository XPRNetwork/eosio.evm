import { transact } from '../eos'

export async function create () {
  console.log("Creating...")
  transact([
    {
      "account": process.env.CONTRACT,
      "name": "create",
      "data": {
        account: process.env.CONTRACT,
        data: 'hi'
      },
      "authorization": [
        {
          "actor": process.env.CONTRACT,
          "permission": "active"
        }
      ]
    }
  ])
}