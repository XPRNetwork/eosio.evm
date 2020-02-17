pragma solidity ^0.6.0;

import "./ERC20.sol";
import "./ERC20Detailed.sol";

contract Token is ERC20, ERC20Detailed {
  constructor (string memory _name, string memory _symbol, uint8 decimals) public ERC20Detailed(_name, _symbol, decimals) {
    _mint(msg.sender, 1000000 * (10 ** uint256(decimals)));
  }
}