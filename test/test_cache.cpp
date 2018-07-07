#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <mips32/cache.hpp>
#include <mips32/literals.hpp>
#include <mips32/machine_inspector.hpp>

using namespace mips32;
using namespace mips32::literals;

/*

CacheBlockIterator CACHE_block_begin() noexcept;
CacheBlockIterator CACHE_block_end() noexcept;

std::uint32_t CACHE_line_no() const noexcept;
std::uint32_t CACHE_associativity() const noexcept;
std::uint32_t CACHE_block_size() const noexcept;

*/

constexpr std::uint32_t operator""_words( unsigned long long n ) noexcept
{
  return ( std::uint32_t )( n * sizeof( std::uint32_t ) );
}

SCENARIO( "A Cache object exists" )
{
  GIVEN( "Caches of various dimensions" )
  {
    Cache direct_small_block{256_KB, 1, 32},                    // 1st
        direct_big_block{4_MB, 1, 4096},                        // 2nd
        two_way_small_block{512_KB, 2, 256},                    // 3rd
        two_way_big_block{2_MB, 2, 8192},                       // 4th
        four_way_small_block{1_KB, 4, 16},                      // 5th
        four_way_big_block{64_KB, 4, 1024},                     // 6th
        sixteen_way_small_block{4_MB, 16, 512},                 // 7th
        sixteen_way_big_block{16_MB, 16, 16384},                // 8th
        full_small_block{4_KB, Cache::FullyAssociative{}, 256}, // 9th
        full_big_block{1_MB, Cache::FullyAssociative{}, 4096};  // 10th

    WHEN( "I inspect them" )
    {
      MachineInspector inspector;

      THEN( "The 1st should satisfy those requirements" )
      {
        inspector.inspect( direct_small_block );

        REQUIRE( inspector.CACHE_line_no() == 256_KB / 32_words );
        REQUIRE( inspector.CACHE_associativity() == 1 );
        REQUIRE( inspector.CACHE_block_size() == 32 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 256_KB / 32_words );
      }

      AND_THEN( "The 2nd should satisfy those requirements" )
      {
        inspector.inspect( direct_big_block );

        REQUIRE( inspector.CACHE_line_no() == 4_MB / 4096_words );
        REQUIRE( inspector.CACHE_associativity() == 1 );
        REQUIRE( inspector.CACHE_block_size() == 4096 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 4_MB / 4096_words );
      }

      AND_THEN( "The 3rd should satisfy those requirements" )
      {
        inspector.inspect( two_way_small_block );

        REQUIRE( inspector.CACHE_line_no() == 512_KB / 256_words / 2 );
        REQUIRE( inspector.CACHE_associativity() == 2 );
        REQUIRE( inspector.CACHE_block_size() == 256 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 512_KB / 256_words );
      }

      AND_THEN( "The 4th should satisfy those requirements" )
      {
        inspector.inspect( two_way_big_block );

        REQUIRE( inspector.CACHE_line_no() == 2_MB / 8192_words / 2 );
        REQUIRE( inspector.CACHE_associativity() == 2 );
        REQUIRE( inspector.CACHE_block_size() == 8192 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 2_MB / 8192_words );
      }

      AND_THEN( "The 5th should satisfy those requirements" )
      {
        inspector.inspect( four_way_small_block );

        REQUIRE( inspector.CACHE_line_no() == 1_KB / 16_words / 4 );
        REQUIRE( inspector.CACHE_associativity() == 4 );
        REQUIRE( inspector.CACHE_block_size() == 16 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 1_KB / 16_words );
      }

      AND_THEN( "The 6th should satisfy those requirements" )
      {
        inspector.inspect( four_way_big_block );

        REQUIRE( inspector.CACHE_line_no() == 64_KB / 1024_words / 4 );
        REQUIRE( inspector.CACHE_associativity() == 4 );
        REQUIRE( inspector.CACHE_block_size() == 1024 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 64_KB / 1024_words );
      }

      AND_THEN( "The 7th should satisfy those requirements" )
      {
        inspector.inspect( sixteen_way_small_block );

        REQUIRE( inspector.CACHE_line_no() == 4_MB / 512_words / 16 );
        REQUIRE( inspector.CACHE_associativity() == 16 );
        REQUIRE( inspector.CACHE_block_size() == 512 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 4_MB / 512_words );
      }

      AND_THEN( "The 8th should satisfy those requirements" )
      {
        inspector.inspect( sixteen_way_big_block );

        REQUIRE( inspector.CACHE_line_no() == 16_MB / 16384_words / 16 );
        REQUIRE( inspector.CACHE_associativity() == 16 );
        REQUIRE( inspector.CACHE_block_size() == 16384 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 16_MB / 16384_words );
      }

      AND_THEN( "The 9th should satisfy those requirements" )
      {
        inspector.inspect( full_small_block );

        REQUIRE( inspector.CACHE_line_no() == 1 );
        REQUIRE( inspector.CACHE_associativity() == 4_KB / 256_words );
        REQUIRE( inspector.CACHE_block_size() == 256 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 4_KB / 256_words );
      }

      AND_THEN( "The 10th should satisfy those requirements" )
      {
        inspector.inspect( full_big_block );

        REQUIRE( inspector.CACHE_line_no() == 1 );
        REQUIRE( inspector.CACHE_associativity() == 1_MB / 4096_words );
        REQUIRE( inspector.CACHE_block_size() == 4096 );

        for ( auto block = inspector.CACHE_block_begin();
              block != inspector.CACHE_block_end();
              ++block ) {

          REQUIRE_FALSE( block.header().valid );
          REQUIRE_FALSE( block.header().dirty );
        }

        REQUIRE( inspector.CACHE_block_end() - inspector.CACHE_block_begin() == 1_MB / 4096_words );
      }
    }
  }

  GIVEN( "A Direct-Mapped Cache of 64 B with blocks of 1 word." )
  {
    Cache cache{64_B, 1, 1};

    MachineInspector inspector;

    inspector.inspect( cache );

    WHEN( "I access 10 words" )
    {
      Cache::Word words[10] = {cache[0x0000'0000], cache[0x1000'0000],
                               cache[0x2000'0000], cache[0x3000'0000],
                               cache[0x4000'0000], cache[0x5000'0000],
                               cache[0x6000'0000], cache[0x7000'0000],
                               cache[0x8000'0000], cache[0x9000'0000]};

      THEN( "All of them are invalid" )
      {
        for ( auto &word : words ) {
          REQUIRE_FALSE( word );
        }
      }
    }

    WHEN( "I access words in a valid block" )
    {
      auto first_line = cache.get_line( 0x0000'0000 );

      auto &header = first_line.header( 0 );
      header.valid = true;
      header.tag   = cache.extract_tag( 0x0000'0000 );

      *first_line.block( 0 ) = 0x1234'5678;

      auto word = cache[0x0000'0000];

      THEN( "The word should be valid" )
      {
        REQUIRE( word.valid() );
      }

      AND_THEN( "Reading the word shouldn't set the dirty flag" )
      {
        REQUIRE( *word == 0x1234'5678 );
        REQUIRE_FALSE( header.dirty );
      }

      AND_THEN( "Writing to the word should set the dirty flag" )
      {
        word = 0xABCD'9876;
        REQUIRE( *word == 0xABCD'9876 );
        REQUIRE( header.dirty );
      }
    }
  }
}
