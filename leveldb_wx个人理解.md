leveldb适合写密集、少查询的场景，因为leveldb是顺序写，写入数据只需要一次寻道，而读取数据时可能需要在多层上查找，所以读数据可能需要多次寻道
Minor Compaction：把memtable中的数据导出到SSTable中
Minor Compaction作用：
    内存是有限的，不能将数据都放在内存中
    断电后内存中的数据就会全部丢失，虽然有日志可以恢复数据，但每次都恢复全部的数据显然不现实



Compaction分为好几类：
Minor Compaction：指的是immutable memtable持久化为sst文件
Major Compaction：指的是sst文件之间的compaction
    Manual Compaction：是人工触发的Compaction，由外部接口调用产生
    Size Compaction：每个level的总文件大小超过一定阈值，则会触发
    Seek Compaction：如果一个文件的seek miss次数超过阈值，则会触发
优先级：Minor > Manual > Size > Seek


mutable memtable、immutable memtable、level0、level1...可以看成多级缓存，如果上一级缓存中没找到需要的entry，那么就从下一级找，并且要将上一级缓存和下一级缓存合并，减少以后的查询次数



compaction作用
数据持久化
提高读写效率
平衡读写差异
整理数据



leveldb中一个较好的做法是引用计数，这帮助避免了内存误释放







## 特点

适合少读多写，这是wal（write-ahead logging）技术的特性

LSM树写性能极高的原理，简单地来说就是尽量减少随机写的次数。对于每次写入操作，并不是直接将最新的数据驻留在磁盘中，而是将其拆分成（1）一次日志文件的顺序写（2）一次内存中的数据插入

- key、value支持任意的byte类型数组，不单单支持字符串。
- LevelDb是一个持久化存储的KV系统，将大部分数据存储到磁盘上。
- 按照记录key值顺序存储数据，并且LevleDb支持按照用户定义的比较函数进行排序。
- 操作接口简单，基本操作包括写记录，读记录以及删除记录，也支持针对多条操作的原子批量操作。
- 支持数据快照（snapshot）功能，使得读取操作不受写操作影响，可以在读操作过程中始终看到一致的数据。
- 支持数据压缩(snappy压缩)操作，有效减小存储空间、并增快IO效率。

level0中文件包含的key是可以重叠的，文件太多不方便查询，而level1~level6 文件中key不重复。Imutable memtable的内容会定期刷到磁盘，然后清楚对应的log文件，生成新的log文件。

#### 基础架构

![leveldb架构图](/media/wx/Data/Books/Lang/C++/C++项目/db/leveldb/pictures/leveldb架构图.png)

​													leveldb架构图

总共有六类文件：

- log文件（磁盘）
- memtable（内存）
- immutable memtable（内存）
- sst文件（磁盘）
- manifest文件（磁盘）
- current文件（磁盘）

sst文件特点：

- level0中的单个文件（sst）是有序的，但是文件与文件之间是无序的并且有可能有重合的key

- level 1 ~ level n 每一个level中在自己level中都是全局有序的

- mainifest文件中包含了每一个sst文件的最小key和最大key，方便查找
