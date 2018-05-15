#include "BlockStorage.hpp"
#include "BlockIndexDB.hpp"
#include "ChainParams.hpp"
#include "data/Block.hpp"
#include "AppInfo.hpp"
#include "AppConfig.hpp"
#include "db.hpp"

#include <xul/lang/object_impl.hpp>
#include <xul/net/io_services.hpp>
#include <xul/log/log.hpp>
#include <xul/util/time_counter.hpp>
#include <xul/io/data_input_stream.hpp>
#include <xul/io/data_output_stream.hpp>
#include <xul/io/data_encoding.hpp>
#include <xul/io/stdfile.hpp>
#include <xul/os/paths.hpp>

#include <deque>
#include <functional>
#include <unordered_map>


namespace xbtc {


class DummyBlockStorageListener : public BlockStorageListener
{
public:
    virtual void onBlockWritten(Block* block, BlockIndex* blockIndex, const DiskBlockPos& pos) {}
};

class BlockStorageImpl : public xul::object_impl<BlockStorage>
{
public:
    explicit BlockStorageImpl(AppInfo* appInfo) : m_appInfo(appInfo)
    {
        XUL_LOGGER_INIT("BlockStorage");
        XUL_REL_EVENT("new");
        m_db = createBlockIndexDB(appInfo->getAppConfig());
        m_lastBlockFile = 0;
        setListener(nullptr);
    }
    ~BlockStorageImpl()
    {
        XUL_REL_EVENT("delete");
    }
    virtual bool load(BlockIndexesData& data)
    {
        XUL_REL_EVENT("load " << m_appInfo->getAppConfig()->dataDir);
        xul::time_counter starttime;
        if (!m_db->open())
            return false;
        xul::time_counter starttime2;
        m_db->loadAll(data);
        XUL_DEBUG("load " << xul::make_tuple(starttime.elapsed(), starttime2.elapsed()));
        m_blockFiles = std::move(data.files);
        m_lastBlockFile = data.lastBlockFile;
        return true;
    }
    virtual void setListener(BlockStorageListener* listener)
    {
        m_listener = listener ? listener : &m_dummyListener;
    }

    virtual DiskBlockPos writeBlock(Block* block, BlockIndex* blockIndex)
    {
        auto s = std::make_shared<std::string>();
        s->resize(2*1024*1024);
        xul::memory_data_output_stream os(&(*s)[0], s->size(), false);
        os << *block;
        s->resize(os.position());
        DiskBlockPos pos = findBlockPos(0, s->size() + 8);
        m_blockFiles[pos.fileIndex].addBlock(blockIndex->height, blockIndex->header.timestamp);
        xul::io_services::post(m_appInfo->getDiskIOService(), std::bind(&BlockStorageImpl::doWriteBlock, this, s, pos, BlockPtr(block), BlockIndexPtr(blockIndex)));
        return pos;
    }
    virtual Block* readBlock(const BlockIndex* blockIndex)
    {
        return doReadBlock(blockIndex);
    }
    virtual void flush(const std::shared_ptr<BlockIndexesData>& data)
    {
        data->lastBlockFile = m_lastBlockFile;
        for (auto blockFile : m_dirtyFiles)
        {
            assert(blockFile >= 0 && blockFile < m_blockFiles.size());
            data->files.push_back(m_blockFiles[blockFile]);
        }
        m_dirtyFiles.clear();
        xul::io_services::post(m_appInfo->getDiskIOService(), std::bind(&BlockStorageImpl::doFlush, this, data));
    }
private:
    void doFlush(std::shared_ptr<BlockIndexesData> data)
    {
        m_db->writeBlocks(*data);
    }
    DiskBlockPos findBlockPos(int preferredFile, unsigned blockSize)
    {
        int blockFile = preferredFile > 0 ? preferredFile : m_lastBlockFile;
        if (m_blockFiles.size() <= blockFile)
        {
            m_blockFiles.resize(blockFile + 1);
        }

        while (m_blockFiles[blockFile].size + blockSize >= MAX_BLOCKFILE_SIZE)
        {
            blockFile++;
            if (m_blockFiles.size() <= blockFile)
            {
                m_blockFiles.resize(blockFile + 1);
            }
        }
        DiskBlockPos pos;
        pos.fileIndex = blockFile;
        pos.position = m_blockFiles[blockFile].size;
        m_blockFiles[blockFile].size += blockSize;
        m_lastBlockFile = blockFile;
        m_dirtyFiles.insert(blockFile);
        return pos;
    }

    std::string formatBlockFilePath(int fileIndex)
    {
        std::string filepath = xul::paths::join(m_appInfo->getAppConfig()->dataDir, xul::strings::format("blocks/blk%05u.dat", fileIndex));
        return filepath;
    }

    Block* doReadBlock(const BlockIndex* blockIndex)
    {
        std::string filepath = formatBlockFilePath(blockIndex->fileIndex);
        xul::stdfile_reader fin;
        if (!fin.open_binary(filepath.c_str()))
        {
            return nullptr;
        }
        assert(blockIndex->dataPosition >= 8);
        if (!fin.seek(blockIndex->dataPosition - 8))
        {
            return nullptr;
        }
        uint8_t buf[8];
        size_t size = fin.read(buf, 8);
        if (size != 8)
        {
            return nullptr;
        }
        uint32_t magic = xul::bit_converter::little_endian().to_dword(buf);
        uint32_t blockSize = xul::bit_converter::little_endian().to_dword(buf + 4);
        if (magic != m_appInfo->getChainParams()->protocolMagic || blockSize == 0 || blockSize > 2*1024*1024)
        {
            return nullptr;
        }
        std::string s;
        s.resize(blockSize);
        size = fin.read(&s[0], blockSize);
        if (size != blockSize)
        {
            return nullptr;
        }
        Block* block = createBlock();
        if (!xul::data_encoding::little_endian().decode(s, *block))
        {
            block->release_reference();
            return nullptr;
        }
        block->header.computeHash();
        for (auto& tx : block->transactions)
        {
            tx.computeHash();
        }
        return block;
    }
    void doWriteBlock(std::shared_ptr<std::string> data, DiskBlockPos pos, BlockPtr block, BlockIndexPtr blockIndex)
    {
        xul::stdfile_writer fout;
        std::string filepath = formatBlockFilePath(pos.fileIndex);
        if (!fout.open_binary_writing(filepath.c_str()))
        {
            if (!fout.open_binary(filepath.c_str()))
            {
                assert(false);
                return;
            }
        }
        if (!fout.seek(pos.position))
        {
            assert(false);
            return;
        }
        uint8_t buf[8];
        xul::bit_converter::little_endian().from_dword(buf, m_appInfo->getChainParams()->protocolMagic);
        xul::bit_converter::little_endian().from_dword(buf + 4, data->size());
        fout.write(buf, 8);
        size_t size = fout.write(*data);
        XUL_EVENT("doWriteBlock " << xul::make_tuple(pos.fileIndex, pos.position, size, data->size()));
        assert(size == data->size());
        fout.flush();
        xul::io_services::post(m_appInfo->getDiskIOService(), std::bind(&BlockStorageImpl::signalBlockWritten, this, pos, block, blockIndex));
    }
    void signalBlockWritten(DiskBlockPos pos, BlockPtr block, BlockIndexPtr blockIndex)
    {
        m_listener->onBlockWritten(block.get(), blockIndex.get(), pos);
    }
private:
    XUL_LOGGER_DEFINE();
    boost::intrusive_ptr<AppInfo> m_appInfo;
    boost::intrusive_ptr<BlockIndexDB> m_db;
    int m_lastBlockFile;
    std::vector<BlockFileInfo> m_blockFiles;
    std::set<int> m_dirtyFiles;
    std::vector<DiskBlockPos> m_blockPositions;
    BlockStorageListener* m_listener;
    DummyBlockStorageListener m_dummyListener;
};


BlockStorage* createBlockStorage(AppInfo* appInfo)
{
    return new BlockStorageImpl(appInfo);
}


}
