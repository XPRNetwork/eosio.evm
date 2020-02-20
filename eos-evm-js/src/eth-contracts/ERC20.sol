pragma solidity ^0.6.0;

import "node_modules/@openzeppelin/contracts/token/ERC20/ERC20.sol";
import "node_modules/@openzeppelin/contracts/token/ERC20/ERC20Detailed.sol";

contract Token is ERC20, ERC20Detailed {
    constructor (string memory name, string memory symbol, uint8 decimals, uint256 initialSupply)
        public ERC20Detailed(name, symbol, decimals) {
        _mint(msg.sender, initialSupply);
    }
}