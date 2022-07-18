// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Thread-safe (provides internal synchronization)

#ifndef STORAGE_LEVELDB_DB_TABLE_CACHE_H_
#define STORAGE_LEVELDB_DB_TABLE_CACHE_H_

#include <stdint.h>

#include <string>

#include "db/dbformat.h"
#include "leveldb/cache.h"
#include "leveldb/table.h"
#include "port/port.h"

namespace leveldb {

class Env;

// TableCache缓存各个table的信息，而block cache缓存某一table的各个data block，每个table都有一个block cache
// 查询流程：先查memtable、再查immutable memtable、然后再查L0层的所有文件，最后一层一层往下查
//
// 查找sstable文件的过程：
// 1、通过sstable的key range，确定key所在的level以及sstable文件
// 2、查找Table Cache，获取Index信息。
//      TableCache::Get -> TableCache::FindTable。
//      如果获取到了Table*，那么进入第三步
//      如果没有在TableCache中找到Table*，        那么直接从SSTable文件中查找，并在TableCache中插入对应的key-value对
// 3、根据Table*查找block cache，获取具体的block记录
//       TableCache::Get -> Table::InternalGet -> Table::BlockReader
//       通过index_block判断记录是否存在，不存在的话直接返回
//       如果存在，则先从block cache中查找，如果找不到，则需要从文件中查找，并在block cache中插入对应的entry

// leveldb就是通过这样的“二级”cache来加速查找数据的过程的

class TableCache {
 public:
  TableCache(const std::string& dbname, const Options& options, int entries);
  ~TableCache();

  // Return an iterator for the specified file number (the corresponding
  // file length must be exactly "file_size" bytes).  If "tableptr" is
  // non-null, also sets "*tableptr" to point to the Table object
  // underlying the returned iterator, or to nullptr if no Table object
  // underlies the returned iterator.  The returned "*tableptr" object is owned
  // by the cache and should not be deleted, and is valid for as long as the
  // returned iterator is live.
  // 为一个文件创建一个迭代器，这个迭代器是一个二级迭代器，根据这个迭代器可以遍历文件的所有键值对
  Iterator* NewIterator(const ReadOptions& options, uint64_t file_number,
                        uint64_t file_size, Table** tableptr = nullptr);

  // If a seek to internal key "k" in specified file finds an entry, call (*handle_result)(arg, found_key, found_value).
  Status Get(const ReadOptions& options,
             uint64_t file_number,
             uint64_t file_size,
             const Slice& k,
             void* arg,
             void (*handle_result)(void*, const Slice&, const Slice&));

  // Evict any entry for the specified file number
  void Evict(uint64_t file_number);

 private:
  Status FindTable(uint64_t file_number, uint64_t file_size, Cache::Handle**);

  Env* const env_;
  const std::string dbname_;
  const Options& options_;
  Cache* cache_;// 存储的entry的key为file_number，value为tf（包含file_number对应的文件和file_number对应的table，table结构在内存中，保存了SSTable的index内容以及用来指示block cache的cache_id）
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_TABLE_CACHE_H_
