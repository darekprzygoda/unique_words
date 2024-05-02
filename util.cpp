#include "util.hpp"

namespace util {

    std::uint64_t divideWithCeil( std::uint64_t x, std::uint64_t y ) {
        if ( x == 0 )
            return 0;
        else
            return 1 + ( ( x - 1 ) / y );
    }

    std::vector< std::string_view > splitToChunks( std::string_view input, unsigned count, char separator ) {
        std::vector< std::string_view > res;
        assert( count > 0 );
        std::size_t chunkSize = divideWithCeil( input.size(), count );
        if ( chunkSize < 2 )
            return { input }; // single chunk

        while ( true ) {
            if ( res.size() == count - 1 ) {
                res.push_back( input ); // last chunk
                return res;
            }
            auto pos = input.find_last_of( separator, chunkSize );
            if ( pos != std::string_view::npos ) {
                res.emplace_back( input.data(), pos + 1 ); // up to and including separator
                input.remove_prefix( pos + 1 );
            } else { // no separator ->  last chunk
                res.push_back( input );
                return res;
            }
        }
        return res;
    }

    std::size_t parseNumberWithOptionalSuffix( std::string const& input ) {
        std::size_t count = 0;
        std::size_t num = std::stol( input, &count, 10 );
        if ( count == input.size() )
            return num; // no suffix
        if ( count == input.size() - 1 ) {
            if ( input.ends_with( "g" ) || input.ends_with( "G" ) )
                return num * util::kGB;
            else if ( input.ends_with( "m" ) || input.ends_with( "M" ) )
                return num * util::kMB;
            else if ( input.ends_with( "k" ) || input.ends_with( "K" ) )
                return num * util::kKB;
        }
        throw std::runtime_error(
            "Invalid value '" + std::string( input ) + "', should be positive integer with optional suffix K, M or G" );
    }

} // namespace util
