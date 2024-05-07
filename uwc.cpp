#include "util.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>

namespace {
// #define Logging 1
#ifdef Logging
    std::mutex logMutex_;
    [[maybe_unused]]
    void log( char const* what ) {
        std::unique_lock lock( logMutex_ );
        // std::cout << std::this_thread::get_id() << ": ";
        std::cout << what << std::endl;
    }
    [[maybe_unused]]
    void log( std::string const& what ) {
        std::unique_lock lock( logMutex_ );
        // std::cout << std::this_thread::get_id() << ": ";
        std::cout << what << std::endl;
    }
    template< typename... ARGS >
    void log( ARGS const&... args ) {
        std::unique_lock lock( logMutex_ );
        // std::cout << std::this_thread::get_id() << ": ";
        ( std::cout << ... << args ) << std::endl;
    }
#else
    [[maybe_unused]] void log( char const* ) {}
    template< typename... ARGS >
    void log( ARGS const&... ) {}

#endif
} // namespace

namespace uwc {

    // to enable heterogenous lookup by string_view or const char*
    struct string_hash {
        using is_transparent = void;
        [[nodiscard]] size_t operator()( const char* txt ) const { return std::hash< std::string_view >{}( txt ); }
        [[nodiscard]] size_t operator()( std::string_view txt ) const { return std::hash< std::string_view >{}( txt ); }
        [[nodiscard]] size_t operator()( const std::string& txt ) const { return std::hash< std::string >{}( txt ); }
    };
    using Words = std::unordered_set< std::string, string_hash, std::equal_to<> >;

    const auto npos = std::string_view::npos;

    class Worker {
      public:
        explicit Worker( int id, Words const& final, std::atomic< unsigned >& done )
            : id_( id ), done_( done ), finalWords_( final ), thread_( &Worker::process, this ) {}
        ~Worker() {
            // log( "~worker()", id_ );
            stop();
        }
        void stop() {
            {
                std::unique_lock lock( m_ );
                state_ = Exit;
                cv_.notify_one();
            }
            thread_.join();
            log( id_, ": Worker stopped." );
        }

        void run( std::string_view input, bool clear ) {
            std::unique_lock lock( m_ );
            if ( clear )
                words_.clear();
            data_ = input;
            mergeWith_ = nullptr;
            if ( data_.empty() ) {
                state_ = Done;
                done_.fetch_add( 1, std::memory_order_release );
            } else {
                state_ = Go;
                cv_.notify_one();
            }
        }

        Words const& getWords() const {
            assert( state_ == Done );
            return words_;
        }

        Words& useWords() { return words_; }

        void mergeWith( Worker& other ) {
            std::unique_lock lock( m_ );
            mergeWith_ = &other;
            state_ = Go;
            cv_.notify_one();
        }
        int id() const { return id_; }

      private:
        bool goOrExit() const { return state_ == Go || state_ == Exit; }
        void process() {
            log( id_, ": Worker wait for data" );
            { // wait for data
                std::unique_lock lock( m_ );
                while ( !goOrExit() )
                    cv_.wait( lock );
                if ( state_ == Exit ) {
                    log( id_, ": Worker process() exit" );
                    return;
                }
            }
            while ( true ) {
                if ( mergeWith_ ) {
                    log( id_, ": Merge ", mergeWith_->id_, " into ", id_ );
                    words_.merge( mergeWith_->words_ );
                } else {
                    /// log( id_, ": Worker processing data..." );
                    while ( !data_.empty() ) {
                        // skip spaces/tabs/newlines before word
                        auto idx = data_.find_first_not_of( " \t\n" );
                        if ( idx != npos )
                            data_.remove_prefix( idx );
                        // find end of the word
                        idx = data_.find_first_of( " \t\n" );
                        if ( idx == npos ) {
                            // no delimiter -> last word
                            /// log( id_, ": got word '", word, "'" );
                            if ( !data_.empty() ) {
                                if ( !finalWords_.contains( std::string( data_ ) ) )
                                    words_.emplace( data_ );
                            }
                            break;
                        }
                        std::string_view word( data_.data(), idx );
                        if ( !word.empty() ) {
                            // log( id_, ": got word '", word, "'" );
                            if ( !finalWords_.contains( std::string( word ) ) )
                                words_.emplace( word ); // put word into set
                        }
                        data_.remove_prefix( idx + 1 ); // remove word and delimiter from input
                    }
                    /// log( id_, ": Worker data processed" );
                }

                /*unsigned d = */ done_.fetch_add( 1, std::memory_order_release );
                /// log( id_, ": ", words_.size(), " unique words, doneCounter: ", d + 1 );
                std::unique_lock lock( m_ );
                state_ = Done;
                // wait for next chunk or exit
                while ( !goOrExit() )
                    cv_.wait( lock );
                if ( state_ == Exit ) {
                    log( id_, ": Worker process() exit" );
                    return;
                }
            }
        }

        [[maybe_unused]] int id_; // used in logging only
        std::atomic< unsigned >& done_;
        Words const& finalWords_;

        enum State { Wait, Go, Done, Exit };
        State state_ = Wait;

        std::string_view data_;
        Words words_;
        std::thread thread_;
        mutable std::mutex m_;
        mutable std::condition_variable cv_;

        Worker* mergeWith_ = nullptr;
    };

    class App {
        std::filesystem::path in_;
        const std::size_t defaultInBufSize_ = 256 * util::kMB;
        const std::size_t minInBufSize_ = 4;
        const std::size_t maxInBufSize_ = util::kGB;
        std::size_t inBufSize_ = defaultInBufSize_;
        bool simple_ = false;
        bool verbose_ = true;

        enum AggregateMode { SingleThread, MultiThread, DelayedSingle, DelayedMulti };
        AggregateMode agg_ = DelayedSingle;

      public:
        App() {}

        void usage() { std::cout << "Usage: uwc [-quiet] [-agg single|multi|delayed-single|delayed-multi] [-inbuf <read_buffer_size] <input_path>\n"; }

        bool processCmdline( int argc, char** argv ) {
            std::string sw, arg;
            std::optional< std::string > inPath;
            for ( int i = 1; i < argc; ++i ) {
                arg = argv[ i ];
                if ( sw.empty() ) {
                    if ( arg == "-simple" )
                        simple_ = true;
                    else if ( arg == "-quiet" )
                        verbose_ = false;
                    else if ( arg == "-inbuf" || arg == "-agg" )
                        sw = arg;
                    else if ( !inPath )
                        inPath = arg;
                    else {
                        std::cerr << "Unexpected argument '" << arg << "'\n";
                        return false;
                    }
                } else {
                    // get switch value
                    if ( sw == "-inbuf" ) {
                        try {
                            inBufSize_ = util::parseNumberWithOptionalSuffix( arg );
                        } catch ( std::exception const& e ) {
                            std::cerr << "Bad input buffer size: " << e.what() << "\n";
                            return false;
                        }
                        if ( inBufSize_ < minInBufSize_ || inBufSize_ > maxInBufSize_ ) {
                            std::cerr << "Bad input buffer size: " << inBufSize_ << ", should be in range " << minInBufSize_
                                      << " .. " << maxInBufSize_ << " (bytes)\n";
                            return false;
                        }
                    } else if ( sw == "-agg" ) {
                        if ( arg == "single" )
                            agg_ = SingleThread;
                        else if ( arg == "multi" )
                            agg_ = MultiThread;
                        else if ( arg == "delayed-single" )
                            agg_ = DelayedSingle;
                        else if ( arg == "delayed-multi" )
                            agg_ = DelayedMulti;
                        else {
                            std::cerr << "Bad value of -agg switch '" << arg
                                      << "', should be single, multi, delayed-single or delayed-multi\n";
                            return false;
                        }
                    }
                    sw.clear();
                }
            }
            if ( !inPath ) {
                std::cerr << "Error: Specify input file\n";
                return false;
            }
            in_ = *inPath;
            return true;
        }
        int countSimple() {
            // single thread, single set
            std::ifstream input( in_, std::ios::binary );
            if ( !input ) {
                std::cerr << "Error: Cannot open input file: " << in_.string() << "." << std::endl;
                return 1;
            }
            auto startTime = std::chrono::steady_clock::now();
            if ( verbose_ ) {
                std::cout << "================================================\n";
                std::cout << "Processing file (-simple) " << in_.string() << "..." << std::endl;
            }
            Words words;
            std::size_t total = 0;

            util::Buffer buf( inBufSize_ );
            bool allDone = false;
            while ( !allDone ) {
                input.read( buf.storageStart(), buf.storageSize() );
                buf.addValid( input.gcount() );
                if ( input.eof() )
                    allDone = true;
                std::string_view data = buf.view();
                // process data up to last space
                while ( true ) {
                    auto idx = data.find_first_not_of( " \t\n" );
                    if ( idx != npos )
                        data.remove_prefix( idx );
                    // find end of the word
                    idx = data.find_first_of( " \t\n" );
                    if ( idx == npos ) {
                        if ( allDone ) {
                            if ( !data.empty() ) {
                                ++total;
                                words.emplace( data ); // put last word into set
                            }
                        } else
                            buf.reset( data.size() ); // keep in buffer
                        break;
                    } else {
                        std::string_view word( data.data(), idx );
                        ++total;
                        /// log( id_, ": got word '", word, "'" );
                        words.emplace( word );         // put word into set
                        data.remove_prefix( idx + 1 ); // remove word and delimiter from input
                    }
                }
            }
            if ( verbose_ ) {
                auto stopTime = std::chrono::steady_clock::now();
                std::chrono::duration< float > sec = stopTime - startTime;
                if ( sec.count() < 1 ) {
                    auto dur = std::chrono::duration_cast< std::chrono::milliseconds >( stopTime - startTime );

                    std::cout << "!!! Done in " << dur.count() << " milliseconds.\n";
                } else
                    std::cout << "!!! Done in " << sec.count() << " seconds.\n";
                std::cout << "File " << in_.string() << " contains " << words.size() << " unique words, total " << total << "\n";
            } else {
                std::cout << words.size() << "\n";
            }
            /// for ( auto const& w : words )
            ///     std::cout << "'" << w << "'\n";
            return 0;
        }

        int countUniqueWords() {
            const unsigned cpuCores = std::thread::hardware_concurrency() + 1;
            log( "Cores: ", cpuCores );

            std::ifstream input( in_, std::ios::binary );
            if ( !input ) {
                std::cerr << "Error: Cannot open input file: " << in_.string() << "." << std::endl;
                return 1;
            }
            auto startTime = std::chrono::steady_clock::now();
            if ( verbose_ ) {
                std::cout << "================================================\n";
                std::cout << "Processing file " << in_.string() << "..." << std::endl;
                if ( agg_ == SingleThread )
                    std::cout << "Aggregate in single thread" << std::endl;
                else if ( agg_ == MultiThread )
                    std::cout << "Aggregate in multiple threads" << std::endl;
                else if ( agg_ == DelayedSingle )
                    std::cout << "Aggregate in single thread after processing all data" << std::endl;
                else // if ( agg_ == DelayedMulti )
                    std::cout << "Aggregate in single multiple threads after processing all data" << std::endl;
            }

            Words finalSet;
            std::vector< std::unique_ptr< Worker > > workers;
            std::vector< Worker* > toMerge;
            toMerge.reserve( cpuCores );

            std::atomic< unsigned > doneCounter{};
            for ( std::size_t i = 0; i < cpuCores; ++i )
                workers.emplace_back( new Worker( i, finalSet, doneCounter ) );

            util::Buffer buf( inBufSize_ );
            bool allDone = false;
            std::size_t keep = 0;
            [[maybe_unused]] std::size_t round = 0;
            log( "Read buffer size: ", buf.size() );

            while ( !allDone ) {
                log( "Read input (round ", round, ")..." );
                input.read( buf.storageStart(), buf.storageSize() );
                buf.addValid( input.gcount() );
                // log( input.gcount(), " bytes read, ", buf.valid(), " bytes available" );
                if ( input.eof() )
                    allDone = true;
                std::string_view data = buf.view();
                if ( input.eof() )
                    keep = 0; // no more data in file, process whole buffer
                else {
                    // find last space in buffer, process data up to this space in this round
                    auto pos = data.find_last_of( ' ' );
                    if ( pos == npos )
                        throw std::runtime_error( "Input buffer to small" );
                    keep = buf.valid() - pos - 1; // -1 to skip space
                    data.remove_suffix( keep );
                }
                auto chunks = util::splitToChunks( data, cpuCores );
                //// for ( auto c : chunks )
                ////     log( "CHUNK|", c, "|" );

                // run workers
                doneCounter = 0;
                toMerge.clear();
                std::size_t usedWorkers = chunks.size();
                for ( std::size_t i = 0; i < usedWorkers; ++i ) {
                    workers[ i ]->run( chunks[ i ], ( agg_ == SingleThread || agg_ == MultiThread ) );
                    toMerge.push_back( workers[ i ].get() );
                }

                log( "wait for workers ", usedWorkers );
                // wait for workers to finish
                while ( doneCounter.load( std::memory_order_acquire ) < usedWorkers )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

                if ( agg_ == SingleThread ) {
                    log( "aggregate results in single thread" );
                    for ( std::size_t i = 0; i < usedWorkers; ++i )
                        finalSet.merge( workers[ i ]->useWords() );
                } else if ( agg_ == MultiThread ) {
                    // pairwise in multiple threads
                    log( "aggregate results in multiple threads" );
                    while ( toMerge.size() > 1 ) {
                        auto first = toMerge.begin();
                        auto last = --toMerge.end();
                        doneCounter = 0;
                        std::size_t expected = 0;
                        while ( first < last ) {
                            ( *first )->mergeWith( **last );
                            ++first;
                            --last;
                            ++expected;
                        }
                        // wait for workers to finish
                        log( "wait for ", expected, " workers" );
                        while ( doneCounter.load( std::memory_order_acquire ) < expected )
                            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                        toMerge.erase( std::prev( toMerge.end(), expected ), toMerge.end() );
                    }
                    finalSet.merge( workers[ 0 ]->useWords() );
                }
                buf.reset( keep );
                log( "'", buf.view(), "' kept to process in next round" );
                ++round;
            }
            if ( agg_ == DelayedSingle ) {
                log( "Delayed Merge start" );
                if ( workers.size() > 0 ) {
                    finalSet = std::move( workers[ 0 ]->useWords() );
                    for ( std::size_t i = 1; i < workers.size(); ++i ) {
                        log( "Merge ", workers[ i ]->id(), "into final set" );
                        finalSet.merge( workers[ i ]->useWords() );
                    }
                }
                log( "Delayed Merge done" );
            } else if ( agg_ == DelayedMulti ) {
                // pairwise in multiple threads
                log( "Delayed Merge start" );
                toMerge.clear();
                if ( workers.size() > 0 ) {
                    finalSet = std::move( workers[ 0 ]->useWords() );
                    for ( std::size_t i = 1; i < workers.size(); ++i )
                        toMerge.push_back( workers[ i ].get() );
                }
                while ( toMerge.size() > 1 ) {
                    auto first = toMerge.begin();
                    auto last = --toMerge.end();
                    doneCounter = 0;
                    std::size_t expected = 0;
                    while ( first < last ) {
                        ( *first )->mergeWith( **last );
                        ++first;
                        --last;
                        ++expected;
                    }
                    // wait for workers to finish
                    while ( doneCounter.load( std::memory_order_acquire ) < expected )
                        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                    toMerge.erase( std::prev( toMerge.end(), expected ), toMerge.end() );
                }
                log( "Merge ", toMerge[ 0 ]->id(), " into final set" );
                finalSet.merge( toMerge[ 0 ]->useWords() );
            }

            workers.clear(); // stop and join worker threads

            if ( verbose_ ) {
                auto stopTime = std::chrono::steady_clock::now();
                std::chrono::duration< float > sec = stopTime - startTime;
                if ( sec.count() < 1 ) {
                    auto dur = std::chrono::duration_cast< std::chrono::milliseconds >( stopTime - startTime );
                    std::cout << "!!! Done in " << dur.count() << " milliseconds.\n";
                } else
                    std::cout << "!!! Done in " << sec.count() << " seconds.\n";
                std::cout << "File " << in_.string() << " contains " << finalSet.size() << " unique words, total ???\n";
            } else {
                std::cout << finalSet.size() << "\n";
            }
            return 0;
        }

        int run( int argc, char** argv ) {
            if ( !processCmdline( argc, argv ) ) {
                usage();
                return 1;
            }
            if ( simple_ )
                return countSimple();
            else
                return countUniqueWords();
        }
    };
} // namespace uwc

int main( int argc, char** argv ) {
    try {
        uwc::App app;
        return app.run( argc, argv );
    } catch ( std::exception& e ) { //
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch ( ... ) { //
        std::cerr << "Unknown exception!" << std::endl;
    }
    return 1;
}
