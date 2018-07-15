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

  //// Random Access Iterator functions ////

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