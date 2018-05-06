#pragma once


namespace xbtc {


class Block;
class BlockIndex;
class Transaction;

class Compatibility
{
public:
    // BIP30
    static bool forbidDuplicateTransaction(const BlockIndex* block);
};


}
