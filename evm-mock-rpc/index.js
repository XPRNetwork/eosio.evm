const express = require('express')
const cors = require('cors')
const bodyParser = require('body-parser')
const methods = require('./methods')

const app = express()
app.use(cors())
app.use(bodyParser.json())

const port = 3000

app.post('/', async (req, res) => {
  const { jsonrpc, id, method, params } = req.body;
  console.log({ id, jsonrpc, method, params })

  if (jsonrpc !== "2.0") throw new Error("Invalid JSON RPC");
  if (typeof method !== "string") throw new Error("Invalid method");

  if (method) {
    const result = await methods[method](params);
    console.log({ id, jsonrpc, result })
    res.json({ id, jsonrpc, result });
  }
})

app.listen(port, () => console.log(`Example app listening at http://localhost:${port}`))
