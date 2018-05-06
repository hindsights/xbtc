### xbtc
xbtc是bitcoin的一个简单实现，写这个程序的目的是为了学习bitcoin core代码，掌握它的实现原理和重要的技术细节。目前主要实现了BlockIndex和Block数据的下载、校验和存储，目前还没有实现bitcoin core中的本地rpc服务接口。
### 依赖关系
1 网络层用的是xul中基于boost.asio封装的网络框架(https://github.com/hindsights/xul)。<br>
2 数据库跟bitcoin core一样，用的是leveldb，格式跟bitcoin core保持一致。<br>
3 哈希算法和签名校验用的openssl库，sip哈希用的是从bitcoin core中提取的代码。<br>
4 序列化用的是xul中的序列化工具类。<br>
### 系统架构
#### 代码组织：
data目录：核心数据类型定义，包括Transaction/Block/Coin等。<br>
storage目录：数据存储相关的类，包括区块链在内存和磁盘存储（目前还包括区块链的校验部分，未来考虑移到单独的目录中）。<br>
net目录：实现p2p网络的搭建和管理维护，以及block数据的同步。<br>
script目录：实现比特币中的脚本的解释和校验。<br>
util目录：包括哈希、序列化、数据库封装、签名校验、大整数计算等工具类。<br>
#### 线程模型
xbtc使用boost.asio来进行网络通信，使用一个线程来运行网络相关的io_service，使用另一个后台线程进行数据库和文件读写之类的耗时操作，线程之间通过io_service的post来进行通信。<br>
#### 运行过程
启动后加载数据库中的BlockIndex和其他一些重要数据，请求dns seeds，获取初始节点，发起p2p连接请求。然后开始请求新的BlockIndex和Block，对收到的数据，验证数据的有效性，包括区块的工作量和交易的合法性等。<br>
### 测试环境
MacOS Sierra<br>
Xcode 8.0<br>
cmake 3.10<br>
openssl 1.0.2n<br>
leveldb 1.20<br>
