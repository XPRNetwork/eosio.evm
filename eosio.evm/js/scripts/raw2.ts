import { transact } from '../eos'

export async function raw () {
  transact([
    {
      "account": process.env.CONTRACT,
      "name": "raw",
      "data": {
        tx: 'f8aa018504a817c800836691b7945384c6c53d93b97e0e955175205e8bfcf1b56ecd80b844a9059cbb000000000000000000000000c7340a9c53f3b1c2a51445295f199f68abab938b000000000000000000000000000000000000000000000000000000000000000a26a0f5bf0c365d03551b25014a554097c72c20f3bfdfc2d68d23042e9e9e28fb43f9a002c336784612b0cf3739edfe8e0169606a5783f3e75b14b13da2f0eaa659f80a',
        sender: '424a26f6de36eb738762cead721bac23c62a724e'
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

raw()