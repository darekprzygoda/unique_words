#include "catch2/matchers/catch_matchers_string.hpp"
#include "trie.hpp"
#include "util.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <string>

using RE = std::runtime_error;
using Catch::Matchers::EndsWith;
using Catch::Matchers::Message;

namespace {
    const std::string digits = "0123456789";
    const std::string lettersL = "abcdefghijklmnopqrstuvwxyz";
    const std::string lettersU = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string alphanums = digits + lettersL + lettersU;

} // namespace

TEST_CASE( "buffer-ctor", "[util]" ) {
    CHECK_THROWS_MATCHES( util::Buffer( 0 ), RE, Message( "Buffer size must be greater than 0" ) );

    util::Buffer b1( 1 );
    CHECK( b1.ptr() != nullptr );
    CHECK( b1.ptr() == b1.cptr() );
    CHECK( b1.size() == 1 );
    CHECK( b1.valid() == 0 );
}

TEST_CASE( "buffer", "[util]" ) {
    util::Buffer b( 1 * util::kKB );

    CHECK( b.ptr() != nullptr );
    CHECK( b.size() == 1024 );

    CHECK( b.valid() == 0 );

    std::memcpy( b.storageStart(), lettersU.data(), lettersU.size() );
    CHECK( *b.ptr() == 'A' );
    CHECK( *( b.cptr() + 1 ) == 'B' );
    CHECK( b.view() == "" );
    CHECK( b.storageStart() == b.ptr() );
    CHECK( b.storageSize() == b.size() );

    b.addValid( 2 );
    CHECK( b.valid() == 2 );
    CHECK( b.view() == "AB" );
    CHECK( b.storageStart() == b.ptr() + 2 );
    CHECK( b.storageSize() == b.size() - 2 );

    b.addValid( 2 );
    CHECK( b.valid() == 4 );
    CHECK( b.view() == "ABCD" );
    CHECK( b.storageStart() == b.ptr() + 4 );
    CHECK( b.storageSize() == b.size() - 4 );

    b.addValid( lettersU.size() - 4 );
    CHECK( b.valid() == lettersU.size() );
    CHECK( b.view() == lettersU );
    CHECK( b.storageStart() == b.ptr() + lettersU.size() );
    CHECK( b.storageSize() == b.size() - lettersU.size() );
}

TEST_CASE( "buffer-reset", "[util]" ) {
    util::Buffer b( 128 );
    std::memcpy( b.storageStart(), lettersL.data(), lettersL.size() );
    b.addValid( lettersL.size() );
    CHECK( b.view() == lettersL );
    CHECK( b.storageStart() == b.ptr() + lettersL.size() );
    CHECK( b.storageSize() == b.size() - lettersL.size() );

    b.reset( 5 );
    CHECK( b.view() == "vwxyz" );
    CHECK( b.storageStart() == b.ptr() + 5 );
    CHECK( b.storageSize() == b.size() - 5 );

    std::memcpy( b.storageStart(), lettersU.data(), 5 );
    CHECK( b.view() == "vwxyz" );
    b.addValid( 5 );
    CHECK( b.view() == "vwxyzABCDE" );

    b.reset();
    CHECK( b.view() == "" );
    CHECK( b.storageStart() == b.ptr() );
    CHECK( b.storageSize() == b.size() );
}

TEST_CASE( "buffer-append", "[util]" ) {
    util::Buffer b( 128 );
    b.append( lettersL );
    CHECK( b.view() == lettersL );
    CHECK( b.storageStart() == b.ptr() + lettersL.size() );
    CHECK( b.storageSize() == b.size() - lettersL.size() );

    b.append( digits );
    CHECK( b.view() == lettersL + digits );
}

TEST_CASE( "findLast", "[util]" ) {
    util::Buffer b( 1024 );
    // ________0123456789 123456789 1234
    b.append( "ala ma kota a kot ma ale." );

    const auto none = std::string_view::npos;

    CHECK( b.view().find_last_of( " ", 0 ) == none );
    CHECK( b.view().find_last_of( " ", 2 ) == none );
    CHECK( b.view().find_last_of( " ", 3 ) == 3 );
    CHECK( b.view().find_last_of( " ", 4 ) == 3 );
    CHECK( b.view().find_last_of( " ", 5 ) == 3 );
    CHECK( b.view().find_last_of( " ", 6 ) == 6 );
    CHECK( b.view().find_last_of( " ", 7 ) == 6 );
    CHECK( b.view().find_last_of( " ", 8 ) == 6 );
    CHECK( b.view().find_last_of( " ", 10 ) == 6 );
    CHECK( b.view().find_last_of( " ", 11 ) == 11 );
    CHECK( b.view().find_last_of( " ", 16 ) == 13 );
}

std::string_view skipFirst( std::string_view in ) {
    in.remove_prefix( 1 );
    return in;
}
TEST_CASE( "splitToChunks", "[util]" ) {
    auto chunks = util::splitToChunks( "", 4 );
    REQUIRE( chunks.size() == 1 );
    CHECK( chunks[ 0 ] == "" );

    chunks = util::splitToChunks( "abcd", 5 );
    REQUIRE( chunks.size() == 1 );
    CHECK( chunks[ 0 ] == "abcd" );

    chunks = util::splitToChunks( "abcdefgh", 5 );
    REQUIRE( chunks.size() == 1 );
    CHECK( chunks[ 0 ] == "abcdefgh" );

    chunks = util::splitToChunks( "abc def gh", 5 );
    REQUIRE( chunks.size() == 1 );
    CHECK( chunks[ 0 ] == "abc def gh" );

    chunks = util::splitToChunks( "abc def gh", 3 );
    REQUIRE( chunks.size() == 3 );
    CHECK( chunks[ 0 ] == "abc " );
    CHECK( chunks[ 1 ] == "def " );
    CHECK( chunks[ 2 ] == "gh" );

    chunks = util::splitToChunks( "a b c defgh", 3 );
    REQUIRE( chunks.size() == 3 );
    CHECK( chunks[ 0 ] == "a b " );
    CHECK( chunks[ 1 ] == "c " );
    CHECK( chunks[ 2 ] == "defgh" );

    chunks = util::splitToChunks( "a b c d            ef               gh", 3 );
    CHECK( chunks.size() == 3 );
    CHECK( chunks[ 0 ] == "a b c d       " );
    CHECK( chunks[ 1 ] == "     ef       " );
    CHECK( chunks[ 2 ] == "        gh" );

    auto v = skipFirst( R"(
ptaszek sobie leci z dala,
w gore slonce zatralala,
zaba tylek z stawie moczy,
kurcze co za dzien uroczy.)" );

    auto res = util::splitToChunks( v, 4, '\n' );
    REQUIRE( res.size() == 4 );
    CHECK( res[ 0 ] == "ptaszek sobie leci z dala,\n" );
    CHECK( res[ 1 ] == "w gore slonce zatralala,\n" );
    CHECK( res[ 2 ] == "zaba tylek z stawie moczy,\n" );
    CHECK( res[ 3 ] == "kurcze co za dzien uroczy." );
}

TEST_CASE( "trie-ctor", "[trie]" ) {
    util::Trie t;
    CHECK( t.is_empty() );
    CHECK( t.size() == 0 );

    t.clear();
    CHECK( t.is_empty() );
    CHECK( t.size() == 0 );

    CHECK( !t.contains( "" ) );
    CHECK( !t.contains( "ala" ) );
}

TEST_CASE( "trie-dtor", "[trie]" ) {
    {
        util::Trie t;
        CHECK( t.insert( "ala" ) );
        CHECK( t.insert( "ma" ) );
        CHECK( t.insert( "ale" ) );
    }
}

TEST_CASE( "trie-insert", "[trie]" ) {
    util::Trie t;
    CHECK( t.insert( "a" ) );
    CHECK( t.size() == 1 );
    CHECK( t.insert( "a" ) == false );

    CHECK( t.contains( "a" ) );
    CHECK( !t.contains( "b" ) );

    CHECK( t.insert( "b" ) );
    CHECK( t.size() == 2 );
    CHECK( t.insert( "b" ) == false );

    CHECK( t.insert( "as" ) );
    CHECK( t.size() == 3 );
    CHECK( t.insert( "as" ) == false );

    CHECK( t.insert( "astra" ) );
    CHECK( t.size() == 4 );
}


TEST_CASE( "trie-merge", "[trie]" ) {
    util::Trie t1;
    util::Trie t2;
    CHECK( t1.insert( "a" ) );
    CHECK( t1.insert( "as" ) );
    CHECK( t1.insert( "astra" ) );
    CHECK( t1.size() == 3 );

    CHECK( t2.insert( "asparagus" ) );
    CHECK( t2.insert( "bura" ) );
    CHECK( t2.insert( "cela" ) );
    CHECK( t2.insert( "as" ) );
    CHECK( t2.insert( "celofan" ) );

    CHECK( t2.size() == 5 );

    t1.merge( t2 );
    CHECK( t1.size() == 7 );
    CHECK( t2.size() == 1 );
}
