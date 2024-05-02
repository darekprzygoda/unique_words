#include "util.hpp"
#include <cstdio>
#include <deque>
#include <filesystem>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <thread>

namespace gen {

    namespace {
        struct file_closer {
            using pointer = FILE*;
            void operator()( FILE* f ) { std::fclose( f ); }
        };

        using FileHolder = std::unique_ptr< FILE*, file_closer >;

        std::seed_seq uniqueSeedSequence() {
            std::size_t time_seed = static_cast< size_t >( std::time( nullptr ) );
            std::size_t clock_seed = static_cast< size_t >( std::clock() );
            std::size_t pid_seed = std::hash< std::thread::id >()( std::this_thread::get_id() );
            return std::seed_seq{ time_seed, clock_seed, pid_seed };
        }
        static thread_local std::mt19937 mt = []() {
            std::seed_seq sseq = uniqueSeedSequence();
            std::mt19937 newGen;
            newGen.seed( sseq );
            return newGen;
        }();

        // static const std::string alphanums = "0123456789"
        //                                      "abcdefghijklmnopqrstuvwxyz"
        //                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const std::string letters = "abcdefghijklmnopqrstuvwxyz";

        std::uniform_int_distribution< int > intPool( 0 ); // 0 .. max int
        // std::uniform_int_distribution< int > intPool( 0, std::numeric_limits< std::uint16_t >::max() - 1 );

        std::uniform_int_distribution< int > boolPool( 0, 1 );      // use (0,0) to generate positive numbers only
        std::uniform_int_distribution< int > wordsLenPool( 1, 25 ); // word length
        std::uniform_int_distribution< int > repeatWord( 0, 99 );
        std::uniform_int_distribution< int > newlinePool( 0, 99 );

        void randomWord( std::string& result ) {
            result.clear();
            int len = wordsLenPool( mt );
            std::uniform_int_distribution< std::size_t > pick( 0, letters.size() - 1 );
            for ( int idx = 0; idx < len; ++idx )
                result.push_back( letters[ pick( mt ) ] );
        }

        const std::string_view sw_m = "-multiline=";
        const std::string_view sw_r = "-repeat=";

    } // namespace

    class App {
        std::filesystem::path out_;
        std::size_t size_ = 0;
        int repeat_ = 0;
        int multiline_ = 0;

      public:
        App() {}

        void usage() { std::cout << "Usage: gen [-multiline=<pecent>] [-repeat=<percent>] <output_path> <output_size> \n"; }

        bool processCmdline( int argc, char** argv ) {
            std::string val, size, reps;
            std::size_t count = 0;
            // support for K, M or G suffix
            try {
                for ( int i = 1; i < argc; ++i ) {
                    val = argv[ i ];
                    if ( val.starts_with( sw_m ) ) {
                        val.erase( 0, sw_m.size() );
                        multiline_ = std::stol( val, &count, 10 );
                        if ( count < val.size() || multiline_ < 0 || multiline_ > 100 )
                            throw std::runtime_error(
                                "Invalid value of -multiline '" + val + "', should be integer in range [0,100]" );
                    } else if ( val.starts_with( sw_r ) ) {
                        val.erase( 0, sw_r.size() );
                        repeat_ = std::stol( val, &count, 10 );
                        if ( count < val.size() || repeat_ < 1 || repeat_ > 99 )
                            throw std::runtime_error(
                                "Invalid value of -repeat '" + val + "', should be integer in range [1,99]" );
                    } else {
                        if ( out_.empty() )
                            out_ = val;
                        else if ( size_ == 0 ) {
                            size_ = util::parseNumberWithOptionalSuffix( val );
                            if ( size_ == 0 || size_ >= util::kTB )
                                throw std::runtime_error( "Invalid value of <output_size> '" + val + "'" );
                        } else
                            throw std::runtime_error( "Unexpected argument '" + val + "'" );
                    }
                }
            } catch ( std::exception const& e ) {
                std::cerr << "Error: " << e.what() << "\n";
                return false;
            }
            if ( out_.empty() ) {
                std::cerr << "Error: specify output file path\n";
                return false;
            }
            if ( size_ == 0 ) {
                std::cerr << "Error: specify output file size\n";
                return false;
            }

            return true;
        }

        int generate() {
            auto f = std::fopen( out_.c_str(), "wb" );
            if ( !f ) {
                std::cerr << "Error: Cannot open output file: " << out_ << "." << std::endl;
                return 1;
            }

            std::cout << "Generating file (requested size=" << size_ << ", repeat=" << repeat_ << "%): " //
                      << out_ << "..." << std::endl;
            FileHolder fh( f );

            const std::size_t wbs = 16 * util::kMB; // write buffer size
            // const std::size_t wbs = 512; // write buffer size

            std::string buffer, word;
            std::deque< std::string > usedWords;

            buffer.reserve( wbs );
            std::size_t len = 0;
            std::size_t words = 0;
            std::set< std::string > unique;
            while ( len < size_ ) {
                // extra space?
                if ( boolPool( mt ) )
                    buffer += " ";
                if ( !usedWords.empty() && repeatWord( mt ) <= repeat_ ) {
                    std::uniform_int_distribution< std::size_t > pickWord( 0, usedWords.size() - 1 );
                    auto idx = pickWord( mt );
                    buffer += usedWords[ idx ];
                } else {
                    randomWord( word );
                    while ( unique.contains( word ) )
                        randomWord( word );
                    unique.insert( word );
                    usedWords.push_back( word );
                    buffer += word;
                }
                ++words;
                if ( multiline_ == 0 )
                    buffer += " ";
                else if ( multiline_ == 100 )
                    buffer += "\n";
                else if ( newlinePool( mt ) < multiline_ )
                    buffer += "\n";
                else
                    buffer += " ";

                if ( buffer.size() > size_ || buffer.size() > wbs ) {
                    std::fwrite( buffer.c_str(), 1, buffer.size(), fh.get() );
                    len += buffer.size();
                    buffer.clear();
                }
            }
            if ( !buffer.empty() )
                std::fwrite( buffer.c_str(), 1, buffer.size(), fh.get() );

            std::cout << "File " << out_ << " contains " << words << " words (" << unique.size() << " unique)" << std::endl;
            return 0;
        }

        int run( int argc, char** argv ) {
            if ( !processCmdline( argc, argv ) ) {
                usage();
                return 1;
            }
            return generate();
        }
    };
} // namespace gen

int main( int argc, char** argv ) {
    try {
        gen::App app;
        return app.run( argc, argv );
    } catch ( std::exception& e ) { //
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch ( ... ) { //
        std::cerr << "Unknown exception!" << std::endl;
    }
    return 1;
}
