class MachineInspector::FPR
{
  public:
  explicit FPR( mips32::FPR *fpr ) noexcept : fpr( fpr ) {}

  FPR &operator=( float f ) noexcept
  {
    fpr->f = f;
    return *this;
  }
  FPR &operator=( double d ) noexcept
  {
    fpr->d = d;
    return *this;
  }
  FPR &operator=( std::uint32_t i32 ) noexcept
  {
    fpr->i32 = i32;
    return *this;
  }
  FPR &operator=( std::uint64_t i64 ) noexcept
  {
    fpr->i64 = i64;
    return *this;
  }

  float read_single() const noexcept
  {
    return fpr->f;
  }
  double read_double() const noexcept
  {
    return fpr->d;
  }

  std::uint32_t single_binary() const noexcept
  {
    return fpr->i32;
  }
  std::uint64_t double_binary() const noexcept
  {
    return fpr->i64;
  }

  //// Random Access Iterator functions ////

  FPR &operator++() noexcept
  {
    ++fpr;
    return *this;
  }

  FPR operator++( int ) noexcept
  {
    FPR old( *this );
    ++fpr;
    return old;
  }

  FPR &operator--() noexcept
  {
    --fpr;
    return *this;
  }

  FPR operator--( int ) noexcept
  {
    FPR old( *this );
    --fpr;
    return old;
  }

  FPR friend operator+( FPR it, int offset ) noexcept
  {
    return FPR( it.fpr + offset );
  }

  FPR friend operator+( int offset, FPR it ) noexcept
  {
    return it + offset;
  }

  FPR friend operator-( FPR it, int offset ) noexcept
  {
    return it + ( -offset );
  }

  bool operator==( FPR other ) const noexcept
  {
    return fpr == other.fpr;
  }

  bool operator!=( FPR other ) const noexcept
  {
    return !operator==( other );
  }

  std::ptrdiff_t operator-( FPR other ) const noexcept
  {
    return fpr - other.fpr;
  }

  private:
  mips32::FPR *fpr;
};