#include <mips32/ram.hpp>

#include <cassert>
#include <cstdio>
#include <new>

namespace mips32 {

void addr_to_string( char *buf, std::uint32_t addr )
{
  std::sprintf( buf, "0x%08X.block", addr );
}

RAM::RAM( std::uint32_t alloc_limit ) : alloc_limit( alloc_limit / sizeof( std::uint32_t ) )
{
  assert( alloc_limit && "The allocation limit can't be 0 (zero)." );
  assert( alloc_limit % block_size == 0 && "The allocation limit must be a multiple of RAM::block_size." );

  blocks.reserve( alloc_limit / block_size );
}

RAM::Block &RAM::least_accessed() noexcept
{
  std::sort( blocks.begin(), blocks.end(), []( Block &lhs, Block &rhs ) -> bool { return lhs.access_count > rhs.access_count; } );

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
  for ( auto &block : blocks ) {
    if ( contains( block.base_address, address, block_size ) ) {
      // Return the word
      return block[( address & ~0b11 ) - block.base_address];
    }
  }

  // Case 2
  for ( auto &block_on_disk : swapped ) {
    if ( contains( block_on_disk.base_address, address, block_size ) ) {
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
  if ( blocks.size() * ( block_size + 1 ) <= alloc_limit ) {
    Block new_block;

    // Allocate block
    new_block.allocate();
    new_block.base_address = 0;

    // Calculate the base address
    while ( new_block.base_address + block_size < address )
      new_block.base_address += block_size;

    blocks.push_back( std::move( new_block ) );

    // Return the word
    auto &block = blocks.back();
    return block[( address & ~0b11 ) - block.base_address];
  }
  // Case 3.2
  else {
    // Find a block to swap
    auto &allocated_block = least_accessed();

    swapped.push_back( {allocated_block.base_address} );

    // Swap that block on disk
    allocated_block.serialize();

    // Calculate the base address
    allocated_block.base_address = 0;
    while ( allocated_block.base_address + block_size <= address )
      allocated_block.base_address += block_size;

    // Return the word
    return allocated_block[( address & ~0b11 ) - allocated_block.base_address];
  }
}

RAM::Block &RAM::Block::allocate() noexcept
{
  assert( !data && "Block already allocated." );

  data.reset( new ( std::nothrow ) std::uint32_t[RAM::block_size] );
  assert( data && "Couldn't allocate the block." );

  return *this;
}

RAM::Block &RAM::Block::deallocate() noexcept
{
  if ( data ) data.reset( nullptr );
  return *this;
}

RAM::Block &RAM::Block::serialize() noexcept
{
  assert( data && "Block::serialize() called without allocated data." );

  char file_name[18]{'\0'};
  addr_to_string( file_name, base_address );

  std::FILE *out = std::fopen( file_name, "w+b" );
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

  char file_name[18]{'\0'};
  addr_to_string( file_name, base_address );

  std::FILE *in = std::fopen( file_name, "r+b" );
  assert( in && "Couldn't open the file." );

  [[maybe_unused]] auto read_count = std::fread( data.get(), sizeof( std::uint32_t ), RAM::block_size, in );
  assert( read_count == RAM::block_size && "Couldn't read from the file." );

  [[maybe_unused]] auto close = std::fclose( in );
  assert( !close && "Couldn't close the file." );

  return *this;
}

} // namespace mips32