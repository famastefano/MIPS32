#include <mips32/cache.hpp>

#include <cassert>
#include <cmath>

namespace mips32
{

// Credits:
// https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
// Check if a number is a power of 2.
constexpr bool is_pow_of_2( std::uint32_t n ) noexcept
{
  return n && !( n & ( n - 1 ) );
}

/* Generate a bitmask with only `n` bits set.
 * Let's say you want to right shift for 3 bits and **read only** the first 4 bits.
 * You'll do something like that:
 *   `n >> 3 & 0xF`
 * But what if you don't know at compile-time how many bits you need to read?
 * This function helps you in the process, just use it like that:
 *   `auto bit_to_mask = ... calculation ...`
 *   `n >> 3 & maskify_32(bit_to_mask)`
 *   `assert(0xF == maskify_32(4))`
 *
 * The calculation is pretty simple, it relies on logical bit shifting.
 * 1. We start with all bits set `0xFFFF'FFFF`.
 * 2. Then we **left shift** by removing the unwanted bits.
 * 3. At last we **right shift** and thanks to logical bit shifting, 0 are added on the left.
 **/
constexpr std::uint32_t maskify_32( std::uint32_t n ) noexcept
{
  return 0xFFFF'FFFF << ( 32 - n ) >> ( 32 - n );
}

/**
 * To construct a Cache correctly we need to execute those steps:
 * 1. Check the constraints.
 * 2. Calculate the shifting and masking variables.
 * 3. Allocate memory for data and headers.
 **/
Cache::Cache( std::uint32_t capacity, std::uint32_t associativity, std::uint32_t words_per_block )
{
  // 1
  assert( is_pow_of_2( capacity ) && "Capacity must be a power of 2." );
  assert( is_pow_of_2( associativity ) && "Associativity must be a power of 2." );
  assert( is_pow_of_2( words_per_block ) && "The number of words per block must be a power of 2." );

  assert( capacity >= words_per_block && "The capacity must be greater or equal than the number of words in a block." );
  assert( ( associativity <= capacity / sizeof( std::uint32_t ) / words_per_block ) && "The associativity must be less or equal than the number of blocks." );

  this->associativity = associativity;
  this->words_per_block = words_per_block;

  // 2
  auto word_no = capacity / sizeof( std::uint32_t );
  auto block_no = word_no / words_per_block;
  auto line_no = block_no / associativity;

  // 3
  word[shamt] = 2;
  word[mask] = maskify_32( ( std::uint32_t )std::log2( words_per_block ) );

  line[shamt] = word[shamt] + ( std::uint32_t )std::log2( words_per_block );
  line[mask] = maskify_32( ( std::uint32_t )std::log2( line_no ) );

  tag[shamt] = 32 - line[shamt] - word[shamt];
  tag[mask] = maskify_32( 32 - ( std::uint32_t )std::log2( line_no ) - ( std::uint32_t )std::log2( words_per_block ) );

  // 4
  headers.resize( block_no );
  lines.resize( word_no );
}

Cache::Cache( std::uint32_t capacity, Cache::FullyAssociative, std::uint32_t words_per_block )
  : Cache( capacity, capacity / sizeof( std::uint32_t ) / words_per_block, words_per_block )
{}

/**
 * 1. Extract data from the `address`
 * 2. Search through all the line headers if a valid block exists and has the same tag.
 *    2.1 If found [HIT]
 *    2.2 else     [MISS]
 **/
Cache::Word Cache::operator[]( std::uint32_t address ) noexcept
{
  // 1
  auto _word = extract_word( address );
  auto _line = extract_line( address );
  auto _tag = extract_tag( address );

  // 2
  for ( std::uint32_t i = _line; i < _line + associativity; ++i )
  {
    if ( headers[i].valid && headers[i].tag == _tag )
    {
// [HIT]
      return Cache::Word{ headers[i], lines[i * words_per_block + _word] };
    }
  }

  // [MISS]
  return {};
}

Cache::Line Cache::get_line( std::uint32_t address ) noexcept
{
  auto _line = address >> line[shamt] >> line[mask];

  return { headers.data() + _line,
          lines.data() + _line * words_per_block,
          words_per_block,
          associativity };
}

std::uint32_t Cache::extract_word( std::uint32_t address ) const noexcept
{
  return address >> word[shamt] & word[mask];
}

std::uint32_t Cache::extract_line( std::uint32_t address ) const noexcept
{
  return address >> line[shamt] & line[mask];
}

std::uint32_t Cache::extract_tag( std::uint32_t address ) const noexcept
{
  return address >> tag[shamt] & tag[mask];
}

Cache::Word &Cache::Word::operator=( std::uint32_t data ) noexcept
{
  assert( header && word && "Trying to write to an invalid word." );

  *word = data;
  header->dirty = true;

  return *this;
}

std::uint32_t Cache::Word::operator*() const noexcept
{
  assert( header && word && "Trying to read from an invalid word." );

  return *word;
}

bool Cache::Word::valid() const noexcept
{
  if ( header )
    return header->valid;
  else
    return false;
}

Cache::Word::operator bool() const noexcept { return valid(); }

Cache::Line::Line( Header *heads, std::uint32_t *blocks, std::uint32_t words_per_block, std::uint32_t associativity ) noexcept
  : headers( heads ), blocks( blocks ), words_per_block( words_per_block ), associativity( associativity )
{}

std::uint32_t Cache::Line::block_no() const noexcept
{
  return associativity;
}

std::uint32_t Cache::Line::block_size() const noexcept
{
  return words_per_block;
}

Header &Cache::Line::header( std::uint32_t pos ) noexcept
{
  assert( pos < associativity && "Out of bounds access header access." );

  return headers[pos];
}

std::uint32_t *Cache::Line::block( std::uint32_t pos ) noexcept
{
  assert( pos < associativity && "Out of bounds access block access." );

  return blocks + ( words_per_block * pos );
}

} // namespace mips32