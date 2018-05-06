### xbtc
xbtc是bitcoin的一个简单实现，写这个程序的目的是为了学习bitcoin core代码，掌握它的实现原理和重要的技术细节。目前主要实现了BlockIndex和Block数据的下载、校验和存储，目前还没有实现bitcoin core中的本地rpc服务接口。
### 依赖关系
1 网络层用的是xul中基于boost.asio封装的网络框架(https://github.com/hindsights/xul)。<br>
2 数据库跟bitcoin core一样，用的是leveldb，格式跟bitcoin core保持一致。<br>
3 哈希算法和签名校验用的openssl库，sip哈希用的是从bitcoin core中提取的代码。<br>
4 序列化用的是xul中的序列化工具类。<br>
### 软件架构
#### 代码组织：
data目录：核心数据类型定义，包括Transaction/Block/Coin等。<br>
storage目录：数据存储相关的类，包括区块链在内存和磁盘存储（目前还包括区块链的校验部分，未来考虑移到单独的目录中）。<br>
net目录：实现p2p网络的搭建和管理维护，以及block数据的同步。<br>
script目录：实现比特币中的脚本的解释和校验。<br>
util目录：包括哈希、序列化、数据库封装、签名校验、大整数计算等工具类。<br>
#### 线程模型
xbtc使用boost.asio来进行网络通信，使用一个线程来运行网络相关的io_service，使用另一个后台线程进行数据库和文件读写之类的耗时操作，线程之间通过io_service的post来进行通信。<br>
#### 运行过程
启动后加载数据库中的BlockIndex和其他一些重要数据，请求dns seeds，获取初始节点，发起p2p连接请求。然后开始请求新的BlockIndex直到没有更新的数据，接着开始下载Block。对收到的数据，验证数据的有效性，包括区块的工作量和交易的合法性等。<br>
### 代码简介
#### 核心数据类型
Transaction/TransactionInput/TransactionOutput/TransactionOutPoint对应bitcoin的交易、交易输入、交易输出、交易输出的索引信息。<br>
BlockHeader表示block头数据，包括block的前一区块哈希、merkle根哈希、时间戳、难度目标、用于计算PoW的随机数等。BlockHeader的哈希小于难度目标即认为是合法区块头（对挖矿模块来说表示成功挖到一个区块，对于其它模块来说表示此区块的PoW验证通过）。BlockHeader会在p2p网络中分发同步。<br>
BlockIndex是以BlockHeader为主体，在内存和磁盘中存储的区块核心信息。为了方便处理，BlockIndex基于原始的BlockHeader信息扩展出高度、磁盘存储信息、累积工作量、累积交易数、前一BlockIndex对象指针等信息，区块链就是有BlockIndex组成的链条，以区块头哈希为key，用std::unordered_map存储在内存中，同时也会保存在在block/index数据库中。BlockIndex较小，可以完整保存在内存中，方便访问。<br>
Block包含BlockHeader和Transaction数组的完整区块数据，收到Block数据后如果能跟前面的区块链串起来，就可以验证整个区块的合法性，验证通过后，如果此区块的累积工作量最大，对应的BlockIndex就可以成为到BestBlockChain的最新区块。Block数据较大，只在需要的时候从磁盘读取。<br>
Coin相关类用来表示一个UXTO项，通过对Coin对象建立索引，可以比较方便的对交易进行数量合法性验证。Coin数据被存储到chainstate数据库中。<br>
#### 数据存储
BlockChain为到工作量最大的有效Block的完整区块链，用数组来存储，以区块高度为key索引。<br>
BlockIndexDB为block/index数据库的操作类。<br>
BlockStorage为BlockIndex的数据库读写和Block的磁盘读写的封装类。<br>
BlockCache负责BlockIndex的内存存储，为系统中其他模块提供BlockIndex和Block的读写访问接口。<br>
CoinDB为chainstate数据库操作类。<br>
CoinView和BlockCache功能类似，负责Coin数据的内存存储，为系统中其他模块提供Coin的读取访问接口。（由于Coin数据比较简单，所以没有对应的CoinStorage类）<br>
#### 网络层
p2p应用的网络层架构比较类似，都有peer发现(PeerDiscoverer)、peer地址池(PeerPool/NodePool)、发起peer连接请求(PeerConnector/NodeConnector)、接收peer连接请求(PeerAcceptor/NodeAcceptor)、peer握手(PendingPeer/PendingNode)、peer连接(PeerConnection/Node)、管理peer连接(ConnectionManager/NodeManager)等模块和一些报文组装、解析和报文发送类。<br>
#### 脚本解释
bitcoin内置了一套简单的基于堆栈的脚本语言，方便自定义交易中签名和公钥的有效性。ScriptVM类负责解释执行脚本，目前只实现了一般会用到的若干个指令。<br>
### 测试环境
MacOS Sierra<br>
Xcode 8.0<br>
cmake 3.10<br>
openssl 1.0.2n<br>
leveldb 1.20<br>
