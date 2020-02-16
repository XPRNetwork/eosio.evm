import { transact } from '../eos'

export async function clearall () {
  console.log("Clear all...")

  transact([
    {
      "account": process.env.CONTRACT,
      "name": "clearall",
      "data": {},
      "authorization": [
        {
          "actor": process.env.CONTRACT,
          "permission": "active"
        }
      ]
    }
  ])
}