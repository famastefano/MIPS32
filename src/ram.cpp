#include <mips32/ram.hpp>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <new>

namespace mips32
{
void addr_to_string( char *buf, std::uint32_t addr )
{
  std::sprintf( buf, "0x%08X.block", addr );
}

RAM::RAM( std::uint32_t alloc_limit ) : alloc_limit( alloc_limit / sizeof( std::uint32_t ) / block_size )
{
  assert( alloc_limit && "The allocation limit can't be 0 (zero)." );
  assert( alloc_limit % block_size == 0 && "The allocation limit must be a multiple of RAM::block_size." );

  blocks.reserve( this->alloc_limit );
}

RAM::Block &RAM::least_accessed() noexcept
{
  std::sort( blocks.begin(), blocks.end(), [] ( Block &lhs, Block &rhs ) -> bool { return lhs.access_count > rhs.access_count; } );

  for ( auto &block : blocks ) block.access_count = 0;

  return blocks.back();
}

/**
 * We need to retrieve the block that contains the address.
 * If the block doesn't exists, we need to create it.
 *
 * Case 1:
 * - Block exists
 * - Block is allocated
 * + Return the word
 *
 * Case 2:
 * - Block exists
 * - Block resides on disk
 * + Find a block to swap
 * + Swap that block on disk
 * + Load the block from disk
 * + Return the word
 *
 * Case 3:
 * - Block doesn't exists
 * - Case 3.1:
 *   - We can allocate another block
 *   + Allocate block
 *   + Calculate the base address
 *   + Return the word
 * - Case 3.2:
 *   - We can't allocate another block (limit reached)
 *   + Find a block to swap
 *   + Swap that block on disk
 *   + Overwrite the block
 *   + Return the word
 **/
std::uint32_t &RAM::operator[]( std::uint32_t address ) noexcept
{
  // Case 1
  for ( auto &block : blocks )
  {
    if ( contains( block.base_address, address, block_size ) )
    {
      // Return the word
      return block[( address & ~0b11 ) - block.base_address];
    }
  }

  // Case 2
  for ( auto &block_on_disk : swapped )
  {
    if ( contains( block_on_disk.base_address, address, block_size ) )
    {
      // Find a block to swap
      auto &allocated_block = least_accessed();

      auto old_addr = allocated_block.base_address;

      // Swap that block on disk
      allocated_block.serialize();
      allocated_block.base_address = block_on_disk.base_address;

      // Load the block from disk
      allocated_block.deserialize();

      block_on_disk.base_address = old_addr;

      // Return the word
      return allocated_block[( address & ~0b11 ) - allocated_block.base_address];
    }
  }

  // Case 3.1
  if ( blocks.size() < alloc_limit )
  {
    Block new_block;

    // Allocate block
    new_block.allocate();
    new_block.base_address = calculate_base_address( address );

    blocks.push_back( std::move( new_block ) );

    // Return the word
    auto &block = blocks.back();
    return block[( address & ~0b11 ) - block.base_address];
  }
  // Case 3.2
  else
  {
    // Find a block to swap
    auto &allocated_block = least_accessed();

    swapped.push_back( { allocated_block.base_address } );

    // Swap that block on disk
    allocated_block.serialize();

    // Calculate the base address
    allocated_block.base_address = calculate_base_address( address );

    // Return the word
    return allocated_block[( address & ~0b11 ) - allocated_block.base_address];
  }
}

/**
 * 1. After reading the Header we need to validate it.
 * 2. We need to read the Segment Headers too.
 * 3. Then we need to check if `phys_addr + size` overflows.
 * 4. After that we load the data 1 block at a time.
 * 
 * A) We'll allocate all the needed blocks on demand,
 * B) and then we'll move up to `alloc_limit` blocks into the RAM,
 * C) and swap the others, if some are left.
 **/
bool RAM::load( void * binary ) noexcept
{
  constexpr std::uint32_t magic_tag{ 0x66'61'6D'61 }; // "fama"
  constexpr std::uint32_t version_tag{ 0x1 };

  unsigned char * cursor = ( unsigned char* )binary;

  struct Header
  {
    std::uint32_t magic, version, segment_no;
  };

  struct SegmentHeader
  {
    std::uint32_t phys_addr, size;
  };

  Header header;

  // 1
  std::memcpy( &header, cursor, sizeof( Header ) );

  if ( header.magic != magic_tag || header.version != version_tag )
    return true;

  // 2
  std::vector<SegmentHeader> segments( header.segment_no );

  if ( segments.empty() )
    return false;

  cursor += sizeof( Header );

  for ( std::uint32_t i = 0; i < header.segment_no; ++i )
  {
    std::memcpy( &segments[i], cursor, sizeof( SegmentHeader ) );
    cursor += sizeof( SegmentHeader );
  }

  // 3
  for ( auto & seg : segments )
  {
    std::uint64_t paddr = seg.phys_addr;
    std::uint64_t size = seg.size;

    if ( paddr + size & 0x1'0000'0000 )
      return true;
  }

  // 4
  std::vector<Block> tmp_blocks;

  // A
  auto get_block_ptr = [ &tmp_blocks ] ( std::uint32_t paddr ) -> Block*
  {
    auto baddr = calculate_base_address( paddr );

    for ( std::size_t i = 0; i < tmp_blocks.size(); ++i )
      if ( tmp_blocks[i].base_address == baddr )
        return &tmp_blocks[i];

    tmp_blocks.push_back( {} );
    return &tmp_blocks.back();
  };

  for ( auto & seg : segments )
  {
    std::uint32_t size = seg.size;
    std::uint32_t paddr = seg.phys_addr;

    while ( size )
    {
      auto * block = get_block_ptr( paddr );

      std::uint32_t begin = paddr - block->base_address;
      std::uint32_t count = std::min( RAM::block_size - begin, size );

      char * word_offset = ( char* )( block->data.get() + ( begin & ~0b11 ) );
      std::uint32_t byte_offset = begin & 0b11;

      char * dst = word_offset + byte_offset;

      std::memcpy( dst, cursor, size );

      cursor += count;
      size -= count;
      paddr += count;
    }
  }

  // B only
  if ( tmp_blocks.size() <= alloc_limit )
  {
    blocks = std::move( tmp_blocks );
  }
  else // B + C
  {
    auto tmp_alloc_begin = std::make_move_iterator( tmp_blocks.begin() );
    auto tmp_alloc_end = std::make_move_iterator( tmp_blocks.begin() + alloc_limit );

    auto tmp_swap_begin = tmp_blocks.begin() + alloc_limit;
    auto tmp_swap_end = tmp_blocks.end();

    // B
    blocks.clear();
    blocks.reserve( alloc_limit );
    blocks.insert( blocks.end(), tmp_alloc_begin, tmp_alloc_end );

    // C
    swapped.clear();
    swapped.reserve( tmp_swap_end - tmp_swap_begin );
    while ( tmp_swap_begin != tmp_swap_end )
    {
      tmp_alloc_begin->deserialize();
      swapped.push_back( { tmp_swap_begin->base_address } );

      ++tmp_swap_begin;
    }
  }

  return false;
}

RAM::Block &RAM::Block::allocate() noexcept
{
  constexpr std::uint32_t sigrie{ 0x0417'CCCC };

  assert( !data && "Block already allocated." );

  data.reset( new ( std::nothrow ) std::uint32_t[RAM::block_size] );
  assert( data && "Couldn't allocate the block." );

  if ( data )
    std::fill_n( data.get(), RAM::block_size, sigrie ); // fill the new block with the 'sigrie' instruction

  return *this;
}

RAM::Block &RAM::Block::deallocate() noexcept
{
  data.reset( nullptr );
  return *this;
}

RAM::Block &RAM::Block::serialize() noexcept
{
  assert( data && "Block::serialize() called without allocated data." );

  char file_name[18]{ '\0' };
  addr_to_string( file_name, base_address );

  std::FILE *out = std::fopen( file_name, "wb" );
  assert( out && "Couldn't open the file." );

  [[maybe_unused]] auto write_count = std::fwrite( data.get(), sizeof( std::uint32_t ), RAM::block_size, out );
  assert( write_count == RAM::block_size && "Couldn't write to the file." );

  [[maybe_unused]] auto close = std::fclose( out );
  assert( !close && "Couldn't close the file." );

  return *this;
}

RAM::Block &RAM::Block::deserialize() noexcept
{
  assert( data && "Block::deserialize() called without allocated data." );

  char file_name[18]{ '\0' };
  addr_to_string( file_name, base_address );

  std::FILE *in = std::fopen( file_name, "rb" );
  assert( in && "Couldn't open the file." );

  [[maybe_unused]] auto read_count = std::fread( data.get(), sizeof( std::uint32_t ), RAM::block_size, in );
  assert( read_count == RAM::block_size && "Couldn't read from the file." );

  [[maybe_unused]] auto close = std::fclose( in );
  assert( !close && "Couldn't close the file." );

  return *this;
}

} // namespace mips32