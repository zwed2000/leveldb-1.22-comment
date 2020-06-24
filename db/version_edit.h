// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_VERSION_EDIT_H_
#define STORAGE_LEVELDB_DB_VERSION_EDIT_H_

#include <set>
#include <utility>
#include <vector>

#include "db/dbformat.h"

// 参考Manifest：https://juejin.im/post/5ea9cc0b5188256d6c59466b
namespace leveldb {

class VersionSet;

struct FileMetaData {
  FileMetaData() : refs(0), allowed_seeks(1 << 30), file_size(0) {}

  int refs;
  // 每个文件的空检索率，按比例计算检索miss次数和文件大小，超过一定比率则需要compaction
  int allowed_seeks;  // Seeks allowed until compaction
  uint64_t number;  // 文件号
  uint64_t file_size;    // File size in bytes
  InternalKey smallest;  // Smallest internal key served by table
  InternalKey largest;   // Largest internal key served by table
};

class VersionEdit {
 public:
  VersionEdit() { Clear(); }
  ~VersionEdit() {}

  void Clear();

  void SetComparatorName(const Slice& name) {
    has_comparator_ = true;
    comparator_ = name.ToString();
  }
  void SetLogNumber(uint64_t num) {
    has_log_number_ = true;
    log_number_ = num;
  }
  void SetPrevLogNumber(uint64_t num) {
    has_prev_log_number_ = true;
    prev_log_number_ = num;
  }
  void SetNextFile(uint64_t num) {
    has_next_file_number_ = true;
    next_file_number_ = num;
  }
  void SetLastSequence(SequenceNumber seq) {
    has_last_sequence_ = true;
    last_sequence_ = seq;
  }
  void SetCompactPointer(int level, const InternalKey& key) {
    compact_pointers_.push_back(std::make_pair(level, key));
  }

  // Add the specified file at the specified number.
  // REQUIRES: This version has not been saved (see VersionSet::SaveTo)
  // REQUIRES: "smallest" and "largest" are smallest and largest keys in file
  void AddFile(int level, uint64_t file, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest) {
    FileMetaData f;
    f.number = file;
    f.file_size = file_size;
    f.smallest = smallest;
    f.largest = largest;
    new_files_.push_back(std::make_pair(level, f));
  }

  // Delete the specified "file" from the specified "level".
  void DeleteFile(int level, uint64_t file) {
    deleted_files_.insert(std::make_pair(level, file));
  }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(const Slice& src);

  std::string DebugString() const;

 private:
  friend class VersionSet;

  typedef std::set<std::pair<int, uint64_t> > DeletedFileSet;

  std::string comparator_;
  uint64_t log_number_; // 最小的有效 log number。小于 log_numbers_ 的 log 文件都可以删除。
  uint64_t prev_log_number_;  // 已经废弃
  uint64_t next_file_number_; // 下一个文件的编号
  SequenceNumber last_sequence_;  // SSTable 中的最大的 sequence number
  bool has_comparator_;
  bool has_log_number_;
  bool has_prev_log_number_;
  bool has_next_file_number_;
  bool has_last_sequence_;

  std::vector<std::pair<int, InternalKey> > compact_pointers_;  // 记录每一层，本次已压缩的最大key，下次压缩的起始key，<level, startKey>
  DeletedFileSet deleted_files_;  // 可以删除的 SSTable <level, file-no>
  std::vector<std::pair<int, FileMetaData> > new_files_;  // 新增的 SSTable，<level, FileMetaData>
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_VERSION_EDIT_H_
