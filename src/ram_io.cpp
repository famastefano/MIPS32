#include <mips32/ram_io.hpp>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace mips32
{

/**
 * To read a string from our RAM we need to handle these cases:
 * A) The string is inside a single Block
 * B) The string is splitted in 2 or more Blocks
 *
 * 1) The Block is present in memory
 * 2) The Block is swapped on disk
 * 3) The Block doesn't exists yet
 *
 * - Aligned address, nothing special
 * - Unaligned address, we need to skip up to 3 bytes of the 1st word
 *
 **
 *
 * [A] and [B] are handled with a simple loop that continues until:
 *    1. we finished the Blocks, or
 *    2. the number of characters reached `count`, or
 *    3. we reached the end of the string
 *
 * [1] and [2] are handled by accessing the Block with a pointer:
 * [1] -> pointer points to an allocated Block *inside the RAM object*
 * [2] -> pointer points to an allocated Block *local to this function*
 *
 * [3] if the Block doesn't exists we return an empty string with only 1 '\0'
 *
 * [Aligned]   we continue normally, nothing to handle
 * [Unaligned] we need to handle it only for the 1st word
 **/
std::unique_ptr<char[]> RAMIO::read( std::uint32_t address, std::uint32_t count ) const noexcept
{
  if ( address + count < address ) // overflows
    count = 0xFFFF'FFFF - address; // we read up to the last byte

  bool                    eof = false;
  std::uint32_t           length = 0; // length of our string
  std::unique_ptr<char[]> str_buf;    // buffer of our string

  auto[index, in_memory] = get_block( address );

  RAM::Block *block = nullptr;
  RAM::Block  tmp;

  while ( index != -1 && !eof && length < count )
  {
    if ( !in_memory ) // [2]
    {
      tmp.base_address = ram.swapped[index].base_address;

      if ( !tmp.data )
        if ( !tmp.allocate().data )
          break;

      tmp.deserialize();

      block = &tmp;

    }
    else // [1]
    {
      block = ram.blocks.data() + index;
    }

    std::uint32_t char_read = 0;

    std::uint32_t begin = address - block->base_address;
    std::uint32_t limit = RAM::block_size - begin;
    std::uint32_t size = std::min( count, limit );

    char * word_offset = ( char* )( block->data.get() + ( begin & ~0b11 ) );
    std::uint32_t byte_offset = ( begin & 0b11 );

    char * start = word_offset + byte_offset;
    char * end = start + size;

    while ( start != end )
    {
      if ( *start++ == '\0' )
      {
        eof = true;
        break;
      }
      ++char_read;
    }

    if ( char_read )
    {
      std::unique_ptr<char[]> buf( new( std::nothrow ) char[char_read + length + 1] );

      assert( buf && "Couldn't allocate memory for the string." );

      if ( str_buf )
        std::memcpy( buf.get(), str_buf.get(), length );

      auto const * src = word_offset + byte_offset;

      std::memcpy( buf.get() + length, src, char_read );

      length += char_read;

      str_buf = std::move( buf );
    } // if ( char_read )

    if ( eof )
      break;

    address += char_read;

    // Corner case where the address overflows
    if ( address < address - char_read )
      break;

    { // [B]
      auto[_index, _in_memory] = get_block( address );

      index = _index;
      in_memory = _in_memory;
    }
  } // while ( index != -1 && !eof && length < count )

  // [3]
  if ( !str_buf )
    str_buf.reset( new( std::nothrow ) char[1] );

  str_buf[length] = '\0';

  return str_buf;
}

/**
 * To copy a string into our RAM we need to handle these cases:
 * A) The string is inside a single Block
 * B) The string is splitted in 2 or more Blocks
 *
 * 1) The Block is present in memory
 * 2) The Block is swapped on disk
 * 3) The Block doesn't exists yet
 *
 * - Aligned address, nothing special
 * - Unaligned address, we need to skip up to 3 bytes of the 1st word
 *
 **
 *
 * [A] and [B] are handled with a simple loop that continues until:
 *    1. the number of characters reached `count`, or
 *    1. we reached the end of the string
 *
 * [1] and [2] are handled by copying the content to:
 *   [1] the Block, or
 *   [2] the file.
 *
 * [3] if the Block doesn't exists we need to create it and push it into our RAM.
 *
 * [Aligned]   we continue normally, nothing to handle
 * [Unaligned] we need to handle it only for the 1st word
 **/
void RAMIO::write( std::uint32_t address, char const *src, std::uint32_t count ) noexcept
{
  if ( count == 0 )
    return;

  if ( address + count < address ) // overflows
    count = 0xFFFF'FFFF - address; // we write up to the last byte in RAM

  std::uint32_t char_written = 0;

  bool eof = false;

  do
  {
    auto[index, in_memory] = get_block( address );

    if ( in_memory ) // [1]
    {
      auto &block = ram.blocks[index];

      std::uint32_t begin = address - block.base_address;
      std::uint32_t limit = RAM::block_size - begin;
      std::uint32_t size = std::min( count, limit );

      char * word_offset = ( char* )( block.data.get() + ( begin & ~0b11 ) );
      std::uint32_t byte_offset = begin & 0b11;

      char * dst = word_offset + byte_offset;

      std::memcpy( dst, src + char_written, size );

      char_written += size;
      count -= size;
    }
    else if ( !in_memory && index != -1 ) // [2]
    {
      auto &block = ram.swapped[index];

      char file_name[18]{ '\0' };
      std::sprintf( file_name, "0x%08X.block", block.base_address );

      std::FILE *block_file = std::fopen( file_name, "r+b" );

      assert( block_file && "Coudln't open file!" );

      std::uint32_t begin = address - block.base_address;
      std::uint32_t limit = RAM::block_size - begin;
      std::uint32_t size = count < limit ? count : limit;

      std::uint32_t word_offset = sizeof( std::uint32_t ) * ( begin & ~0b11 );
      std::uint32_t byte_offset = ( address & 0b11 ) + char_written;

      [[maybe_unused]] auto _seek = std::fseek( block_file, word_offset + byte_offset, SEEK_SET );
      assert( !_seek && "Couldn't seek string position." );

      [[maybe_unused]] auto _write = std::fwrite( src + char_written, sizeof( char ), size, block_file );
      assert( _write == size && "Couldn't write string to file." );

      [[maybe_unused]] auto _close = std::fclose( block_file );
      assert( !_close && "Couldn't close file." );

      char_written += size;
      count -= size;
    }
    else // [3], once the block is created, we can reuse [1] and [2] by adding the block to the RAM and iterate again
    {
      RAM::Block block;

      block.base_address = RAM::calculate_base_address( address );

      block.allocate();
      assert( block.data && "Couldn't allocate block!" );

      // If we can push the new block directly into memory, we add it to the allocated blocks
      if ( ram.swapped.empty() && ( ram.blocks.size() < ram.alloc_limit ) )
      {
        ram.blocks.emplace_back( std::move( block ) );
      }
      else // Otherwise we treat it like a swapped one
      {
        block.deserialize();
        ram.swapped.push_back( { block.base_address } );
      }

      continue;
    }

    address += char_written;

    // Corner case where the address overflows
    if ( address < address - char_written )
      return;

  } while ( !eof && count );
}

std::pair<std::uint32_t, bool> RAMIO::get_block( std::uint32_t address ) const noexcept
{
  for ( auto i = 0u; i < ram.blocks.size(); ++i )
  {
    if ( ram.contains( ram.blocks[i].base_address, address, RAM::block_size ) )
      return std::make_pair( i, true );
  }

  for ( auto i = 0u; i < ram.swapped.size(); ++i )
  {
    if ( ram.contains( ram.swapped[i].base_address, address, RAM::block_size ) )
      return std::make_pair( i, false );
  }

  return std::make_pair( -1, false );
}
} // namespace mips32