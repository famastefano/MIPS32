#pragma once

#include <mips32/header.hpp>

#include <cstdint>
#include <vector>

namespace mips32 {

/**
 * This class simulates a generic Cache.
 * 
 * It can be modeled as you wish at construction time by choosing:
 * - The amount of data it can hold with the `capacity` parameter.
 * - It's "type", Direct Mapped or N-Way associative, with `associativity`,
 *   or use the placeholder `FullyAssociative` to construct a Fully Associative cache.
 * - How many words a block has.
 * 
 * For design reasons it was chosen to maintain it simple, that's why
 * it doesn't have a block substitution policy or other "complex" features.
 **/
class Cache
{
  friend class MachineInspector;

  // Placeholder used to call the correct constructor.
  struct FullyAssociative
  {};

  public:
  /**
   * Constructs a Cache.
   * 
   * `capacity` - the
   **/
  Cache( std::uint32_t capacity, std::uint32_t associativity, std::uint32_t words_per_block );
  Cache( std::uint32_t capacity, FullyAssociative, std::uint32_t words_per_block );

  class Word;
  class Line;

  Word operator[]( std::uint32_t address ) noexcept;

  Line get_line( std::uint32_t address ) noexcept;

  private:
  static inline constexpr std::uint32_t mask{0}, shamt{1};

  std::uint32_t word[2];
  std::uint32_t line[2];
  std::uint32_t tag[2];

  std::uint32_t associativity;
  std::uint32_t words_per_block;

  std::vector<Header>        headers;
  std::vector<std::uint32_t> lines;
};

/**
 * Proxy class to encapsulate the behaviour of the Cache lookup.
 * 
 * #!#!#!
 * It is undefined behaviour to use other methods of this class
 * if `valid()` or `operator bool()` return false.
 * !#!#!#
 **/
class Cache::Word
{
  public:
  Word() = default;

  Word( Header &h, std::uint32_t &w ) noexcept : header( &h ), word( &w ) {}

  // Write `data` to the word.
  // Also set the `dirty` flag.
  Word &operator=( std::uint32_t data ) noexcept;

  // Read the word.
  // Flags are untouched.
  std::uint32_t operator*() const noexcept;

  // Hit or Miss?
  bool valid() const noexcept;

  // See `valid()`
  operator bool() const noexcept;

  private:
  Header *       header{nullptr};
  std::uint32_t *word{nullptr};
};

/**
 * Proxy class to emulate a cache line.
 * 
 * Its purpose is to facilitate the substitution of a block.
 **/
class Cache::Line
{
  public:
  Line( Header *heads, std::uint32_t *blocks, std::uint32_t words_per_block, std::uint32_t associativity ) noexcept;

  // How many blocks do we have in a single line?
  std::uint32_t block_no() const noexcept;

  // How many words does a block hold?
  std::uint32_t block_size() const noexcept;

  // Access the header of the block at `pos`.
  Header &header( std::uint32_t pos ) noexcept;

  // Access the block at `pos`.
  std::uint32_t *block( std::uint32_t pos ) noexcept;

  private:
  Header *headers;

  std::uint32_t *blocks;
  std::uint32_t  words_per_block;
  std::uint32_t  associativity;
};

} // namespace mips32