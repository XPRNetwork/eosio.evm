pragma solidity ^0.6.0;

import "node_modules/@openzeppelin/contracts/token/ERC721/ERC721Full.sol";
import "node_modules/@openzeppelin/contracts/drafts/Counters.sol";

contract Token is ERC721Full {
    using Counters for Counters.Counter;
    Counters.Counter private _tokenIds;

    constructor(string memory name, string memory symbol) ERC721Full(name, symbol) public {
    }

    function awardItem(address player, string memory tokenURI) public returns (uint256) {
        _tokenIds.increment();

        uint256 newItemId = _tokenIds.current();
        _mint(player, newItemId);
        _setTokenURI(newItemId, tokenURI);

        return newItemId;
    }
}