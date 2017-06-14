# UnbufferedOLEStreamBuf
`std::streambuf` implementation wrapping a COM `IStream`

This provides a subclass of `std::streambuf` that wraps a COM `IStream`, so you can use an `IStream` with any C++ code that uses iostreams or the STL algorithms with a `streambuf_iterator`. It does no internal buffering whatsoever (I'd be happy to change this if someone needs it).
