#pragma once

#include <mips32/header.hpp>

#include <cstdint>
#include <vector>

namespace mips32 {

class RAM;
class Cache;

class MachineInspector
{
  public:
  enum class Component
  {
    RAM,
    CACHE,
  };

  void inspect( RAM &ram ) noexcept;
  void inspect( Cache &cache ) noexcept;

  /*********
   *       *
   * STATE *
   *       *
   *********/

  void save_state( Component c, char const *name ) const noexcept;
  void restore_state( Component c, char const *name ) noexcept;

  /*******
   *     *
   * RAM *
   *     *
   *******/

  struct RAMInfo
  {
    std::uint32_t              alloc_limit;
    std::uint32_t              block_size;
    std::uint32_t              allocated_blocks_no;
    std::uint32_t              swapped_blocks_no;
    std::vector<std::uint32_t> allocated_addresses;
    std::vector<std::uint32_t> swapped_addresses;
  };

  RAMInfo RAM_info() const noexcept;

  std::uint32_t              RAM_alloc_limit() const noexcept;
  std::uint32_t              RAM_block_size() const noexcept;
  std::uint32_t              RAM_allocated_blocks_no() const noexcept;
  std::uint32_t              RAM_swapped_blocks_no() const noexcept;
  std::vector<std::uint32_t> RAM_allocated_addresses() const noexcept;
  std::vector<std::uint32_t> RAM_swapped_addresses() const noexcept;

  /*********
   *       *
   * CACHE *
   *       *
   *********/

  class CacheBlockIterator;

  CacheBlockIterator CACHE_block_begin() noexcept;
  CacheBlockIterator CACHE_block_end() noexcept;

  std::uint32_t CACHE_line_no() const noexcept;
  std::uint32_t CACHE_associativity() const noexcept;
  std::uint32_t CACHE_block_size() const noexcept;

  private:
  RAM *ram;

  Cache *cache;
};

class MachineInspector::CacheBlockIterator
{
  public:
  CacheBlockIterator( Header *h, std::uint32_t *b, std::uint32_t wno ) noexcept
      : _header( h ), _block( b ), words_per_block( wno )
  {}

  Header &header() noexcept
  {
    return *_header;
  }

  std::uint32_t *data() noexcept
  {
    return _block;
  }

  std::uint32_t size() noexcept
  {
    return words_per_block;
  }

  CacheBlockIterator &operator++() noexcept
  {
    ++_header;
    _block += words_per_block;
    return *this;
  }

  CacheBlockIterator operator++( int ) noexcept
  {
    CacheBlockIterator old( *this );
    ++*this;
    return old;
  }

  CacheBlockIterator &operator--() noexcept
  {
    --_header;
    _block -= words_per_block;
    return *this;
  }

  CacheBlockIterator operator--( int ) noexcept
  {
    CacheBlockIterator old( *this );
    --*this;
    return old;
  }

  CacheBlockIterator friend operator+( CacheBlockIterator it, int offset ) noexcept
  {
    it._header += offset;
    it._block += it.words_per_block * offset;

    return it;
  }

  CacheBlockIterator friend operator+( int offset, CacheBlockIterator it ) noexcept
  {
    return it + offset;
  }

  CacheBlockIterator friend operator-( CacheBlockIterator it, int offset ) noexcept
  {
    it._header -= offset;
    it._block -= it.words_per_block * offset;

    return it;
  }

  bool operator==( CacheBlockIterator other ) const noexcept
  {
    return _header == other._header;
  }

  bool operator!=( CacheBlockIterator other ) const noexcept
  {
    return !operator==( other );
  }

  std::ptrdiff_t operator-( CacheBlockIterator other ) const noexcept
  {
    return _header - other._header;
  }

  private:
  Header *_header;

  std::uint32_t *_block;
  std::uint32_t  words_per_block;
};

} // namespace mips32