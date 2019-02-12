#include <mips32/ram_io.hpp>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace mips32
{

/**
 * To read a sequence of bytes from our RAM we need to handle these cases:
 * A) The seq is inside a single Block
 * B) The seq is splitted in 2 or more Blocks
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
 *    2. the number of characters reached `count`
 *
 * [1] and [2] are handled by accessing the Block with a pointer:
 * [1] -> pointer points to an allocated Block *inside the RAM object*
 * [2] -> pointer points to an allocated Block *local to this function*
 *
 * [3] if the Block doesn't exists we return an empty seq
 *
 * [Aligned]   we continue normally, nothing to handle
 * [Unaligned] we need to handle it only for the 1st word
 **/
std::vector<char> RAMIO::read( std::uint32_t address, std::uint32_t count, bool read_string ) const noexcept
{
  std::vector<char> seq_buf; // buffer of our sequence

  // only valid memory regions (checked only if we are not reading a string)
  if ( !read_string && ( count == 0 || address + count < address ) )
    return seq_buf;

  auto[index, in_memory] = get_block( address );

  if ( index == -1 ) // [3]
    return seq_buf;

  RAM::Block *block = nullptr;
  RAM::Block tmp;

  bool eof = false;

  while ( index != -1 && count && !eof )
  {
    if ( in_memory ) // [1]
    {
      block = ram.blocks.data() + index;
    }
    else             // [2]
    {
      tmp.base_address = ram.swapped[index].base_address;

      if ( !tmp.data )
        if ( !tmp.allocate().data )
          break;

      tmp.deserialize();

      block = &tmp;
    }

    std::uint32_t begin = address - block->base_address;
    std::uint32_t limit = RAM::block_size - begin;
    std::uint32_t size  = std::min( count, limit );

    auto _old_length = seq_buf.size();

    char * start = (char*)block->data.get() + begin;
    char * end = start + size;

    if ( read_string )
    {
      while ( start != end )
      {
        seq_buf.emplace_back( *start );
        if ( *start == '\0' )
        {
          return seq_buf;
        }
        ++start;
      }
    }
    else
    {
      std::copy( start, end, std::back_inserter( seq_buf ) );
    }

    auto _new_length = seq_buf.size();

    address += _new_length - _old_length;
    count -= _new_length - _old_length;

    // Corner case where the address overflows
    if ( address < address - _new_length )
      break;

    { // [B]
      auto[_index, _in_memory] = get_block( address );

      index = _index;
      in_memory = _in_memory;
    }
  }

  return seq_buf;
}

/**
 * To copy a sequence of bytes into our RAM we need to handle these cases:
 * A) The seq is inside a single Block
 * B) The seq is splitted in 2 or more Blocks
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
 * [A] and [B] are handled with a simple loop that continues until
 * the number of characters reached `count`
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
void RAMIO::write( std::uint32_t address, void const *src, std::uint32_t count ) noexcept
{
  // only valid memory region can be used
  if ( address + count < address )
    return;

  std::uint32_t byte_written = 0;

  while ( count )
  {
    auto[index, in_memory] = get_block( address );

    if ( in_memory ) // [1]
    {
      auto &block = ram.blocks[index];

      std::uint32_t begin = address - block.base_address;
      std::uint32_t limit = RAM::block_size - begin;
      std::uint32_t size  = std::min( count, limit );

      char * dst = ( char* )block.data.get() + begin;
      char * _src = ( char* )src + byte_written;

      std::copy( ( char* )_src, ( char* )_src + size, dst );

      byte_written += size;
      count -= size;
      address += size;
    }
    else if ( !in_memory && index != -1 ) // [2]
    {
      auto &block = ram.swapped[index];

      char file_name[18]{ '\0' };
      std::sprintf( file_name, "0x%08X.block", block.base_address );

      std::FILE *block_file = std::fopen( file_name, "r+b" );

      assert( block_file && "Couldn't open file!" );

      std::uint32_t begin = address - block.base_address;
      std::uint32_t limit = RAM::block_size - begin;
      std::uint32_t size  = std::min( count, limit-1 );

      if ( size == 0 )
      {
        ++byte_written;
        continue;
      }

      [[maybe_unused]] auto _seek = std::fseek( block_file, begin, SEEK_SET );
      assert( !_seek && "Couldn't seek string position." );

      [[maybe_unused]] auto _write = std::fwrite( (char*)src + byte_written, 1, size, block_file );
      assert( _write == size && "Couldn't write string to file." );

      [[maybe_unused]] auto _close = std::fclose( block_file );
      assert( !_close && "Couldn't close file." );

      byte_written += size;
      count -= size;
      address += size;
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

    // Corner case where the address overflows
    if ( address < address - byte_written )
      return;
  }
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