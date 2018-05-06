#pragma once

#include <xul/data/slice.hpp>
#include <xul/lang/object.hpp>
#include <vector>

namespace leveldb {
struct Options;
}

namespace xbtc {


class DBWriteBatch : public xul::object
{
public:
    virtual void clear() = 0;
    virtual void write(const std::string& key, const std::string& val) = 0;
    virtual void erase(const std::string& key) = 0;
    virtual bool execute(bool synced) = 0;
};

class DBIterator : public xul::object
{
public:
    virtual bool valid() const = 0;
    virtual std::string getKey() const = 0;
    virtual xul::slice getValue() const = 0;
    virtual void seek(const std::string& key) = 0;
    virtual void seekToFirst() = 0;
    virtual void seekToLast() = 0;
    virtual void next() = 0;
    virtual void prev() = 0;
};

class DB : public xul::object
{
public:
    virtual bool open(const char* datapath) = 0;
    virtual void close() = 0;
    virtual bool read(const std::string& key, std::string& val) = 0;
    virtual bool write(const std::string& key, const std::string& val, bool synced) = 0;
    virtual bool erase(const std::string& key, bool synced) = 0;
    virtual bool sync() = 0;
    virtual DBWriteBatch* createWriteBatch() = 0;
    virtual DBIterator* createIterator() = 0;
};

DB* createLevelDB(const leveldb::Options& opts);

}
