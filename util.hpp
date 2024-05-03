#ifndef UTIL_HPP
#define UTIL_HPP

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace util {

    const std::size_t kKB = 1024;
    const std::size_t kMB = 1024 * kKB;
    const std::size_t kGB = 1024 * kMB;
    const std::size_t kTB = 1024 * kGB;

    bool helper( std::string const& input );

    class Buffer {
      public:
        Buffer( std::size_t size ) {
            if ( size == 0 )
                throw std::runtime_error( "Buffer size must be greater than 0" );
            data_ = new char[ size ];
            size_ = size;
        }
        ~Buffer() { delete[] data_; }

        std::size_t size() const { return size_; }
        char* ptr() const { return data_; }
        const char* cptr() const { return data_; }

        std::size_t valid() const { return valid_; }        // number of valid bytes (starting from data_)
        void addValid( std::size_t len ) { valid_ += len; } // update number of valid bytes (e.g. after reading into buffer)

        char* storageStart() const { return data_ + valid_; }      // address of first byte after valid bytes
        std::size_t storageSize() const { return size_ - valid_; } // number of unused bytes after valid bytes

        std::string_view view() const { return std::string_view{ data_, valid_ }; }

        // reset number of valid bytes,
        // optionally move keepCount last valid bytes to the begining of the buffer
        void reset( std::size_t keepCount = 0 ) {
            assert( keepCount < valid_ );
            if ( keepCount > 0 )
                std::memmove( data_, data_ + valid_ - keepCount, keepCount ); // memmove handles overlapping ranges
            valid_ = keepCount;
        }

        // copy (part of) data from what into buffer according to available space
        // update number of valid characters in buffer
        // return number of characters that doesn't fit in buffer
        std::size_t append( std::string_view what ) {
            auto ss = storageSize();
            if ( what.size() <= ss ) {
                std::memmove( storageStart(), what.data(), what.size() );
                addValid( what.size() );
                return 0;
            } else {
                std::memcpy( storageStart(), what.data(), ss );
                addValid( ss );
                return what.size() - ss;
            }
        }

      private:
        char* data_;
        std::size_t size_ = 0;
        std::size_t valid_ = 0; // valid characters in buffer

        Buffer( Buffer& );
        Buffer& operator=( Buffer& );
    };

    // split buffer to count chunks similar in length,
    // each chunk except last one ends with separator
    std::vector< std::string_view > splitToChunks( std::string_view input, unsigned count, char separator = ' ' );

    // support K(ilo),M(ega),G(iga), case insensitive
    std::size_t parseNumberWithOptionalSuffix( std::string const& input );

    class BufferedStream {
        Buffer buffer_;
        std::ifstream stream_;

        BufferedStream( BufferedStream const& ) = delete;
        BufferedStream& operator=( BufferedStream const& ) = delete;

      public:
        explicit BufferedStream( std::size_t bufSize = 1 * kMB ) : buffer_( bufSize ) {
            // setup bigger buffer in stream
            stream_.rdbuf()->pubsetbuf( buffer_.ptr(), buffer_.size() );
        }
        ~BufferedStream() { close(); }
        bool open( std::filesystem::path const& path, std::ios_base::openmode mode ) {
            stream_.open( path, mode );
            return stream_.good();
        }
        void close() {
            if ( stream_.is_open() )
                stream_.close();
        }
        std::ifstream& stream() { return stream_; }
    };

    class Timer {
      public:
        explicit Timer() { reset(); }
        void reset();

        std::string get() const; // time since c-tor or reset till now in seconds
      private:
        std::chrono::time_point< std::chrono::steady_clock > start_;
    };

} // namespace util

#endif
