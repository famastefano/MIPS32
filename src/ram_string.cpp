#include <mips32/ram_string.hpp>

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
std::unique_ptr<char[]> RAMString::read( std::uint32_t address, std::uint32_t count ) const noexcept
{
  assert( ( address + count > address ) && "Reading out of bounds." );

  bool                    eof = false;
  std::uint32_t           length = 0; // length of our string
  std::unique_ptr<char[]> str_buf;    // buffer of our string

  auto[index, in_memory] = get_block( address );

  RAM::Block *block = nullptr;
  RAM::Block  tmp;

  while ( index != -1 || !eof || length < count )
  {
    if ( !in_memory )
    { // [2]
      tmp.base_address = ram.swapped[index].base_address;

      if ( !tmp.data )
        if ( !tmp.allocate().data )
          break;

      tmp.deserialize();

      block = &tmp;

    }
    else
    { // [1]
      block = ram.blocks.data() + index;
    }

    std::uint32_t char_read = 0;

    auto const begin = address - block->base_address;
    auto const end = count < RAM::block_size - begin ? count : RAM::block_size - begin;

    for ( auto i = begin; i < end; ++i )
    {
      auto const word = block->data[i];

      std::uint32_t const byte[] = {
          word >> 24,
          word >> 16 & 0xFF,
          word >> 8 & 0xFF,
          word & 0xFF,
      };

      // [Aligned] | [Unaligned]
      for ( int i = address & 0b11; i < 4; ++i )
      {
        if ( !byte[i] )
        {
          eof = true;
          goto _CopyString;
        }
        ++char_read;
      }
    } // for ( i < end )

  _CopyString:
    if ( char_read )
    {
      std::unique_ptr<char[]> buf( new char[char_read + length + 1] );

      if ( str_buf )
        std::memcpy( buf.get(), str_buf.get(), length );

      std::memcpy( buf.get() + length, ( char * )( block->data.get() + begin ) + ( address & 0b11 ), char_read );

      length += char_read;

      str_buf = std::move( buf );
    } // if ( char_read )

    if ( !eof )
      address += char_read;

    // Corner case where the address overflows
    if ( address < address - char_read )
      break;

    { // [B]
      auto[_index, _in_memory] = get_block( address );

      index = _index;
      in_memory = _in_memory;
    }
  } // while ( index != -1 || !eof || length < count )

  // [3]
  if ( !str_buf )
    str_buf.reset( new char[1] );

  str_buf[length] = '\0';

  return std::move( str_buf );
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
void RAMString::write( std::uint32_t address, char const *src, std::uint32_t count ) noexcept
{
  assert( ( address + count > address ) && "Writing out of bounds." );

  std::uint32_t char_written = 0;

  bool eof = false;

  auto[index, in_memory] = get_block( address );

  while ( !eof || count )
  {
    if ( in_memory )
    { // [1]

      auto &block = ram.blocks[index];

      auto const begin = address - block.base_address;
      auto const limit = ( RAM::block_size - begin ) * sizeof( std::uint32_t );
      auto const size = count < limit ? count : limit;

      std::memcpy( ( char * )( block.data.get() + begin ) + ( address & 0b11 ), src + char_written, size );

      char_written += size;
      count -= size;

    }
    else if ( !in_memory && index != -1 )
    { // [2]

      auto const &block = ram.swapped[index];

      char file_name[18]{ '\0' };
      std::sprintf( file_name, "0x%08X.block", block.base_address );

      std::FILE *block_file = std::fopen( file_name, "r+b" );

      assert( block_file && "Coudln't open file!" );

      auto const begin = address - block.base_address;
      auto const limit = ( RAM::block_size - begin ) * sizeof( std::uint32_t );
      auto const size = count < limit ? count : limit;

      std::fseek( block_file, sizeof( std::uint32_t ) * begin + char_written + address & 0b11, SEEK_SET );

      std::fwrite( src, sizeof( char ), size, block_file );

      std::fclose( block_file );

      char_written += size;
      count -= size;

    }
    else
    { // [3]

      RAM::Block block;

      block.base_address = 0;
      while ( block.base_address + RAM::block_size <= address )
        block.base_address += RAM::block_size;

      assert( block.allocate().data && "Couldn't allocate block!" );

      auto const begin = address - block.base_address;
      auto const limit = ( RAM::block_size - begin ) * sizeof( std::uint32_t );
      auto const size = count < limit ? count : limit;

      std::memcpy( ( char * )( block.data.get() + begin ) + ( address & 0b11 ), src + char_written, size );

      block.deserialize();

      char_written += size;
      count -= size;
    }

    address += char_written;

    // Corner case where the address overflows
    if ( address < address - char_written )
      return;

    { // [B]
      auto[_index, _in_memory] = get_block( address );

      index = _index;
      in_memory = _in_memory;
    }
  } // while ( !eof || count )
}

std::pair<std::uint32_t, bool> RAMString::get_block( std::uint32_t address ) const noexcept
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