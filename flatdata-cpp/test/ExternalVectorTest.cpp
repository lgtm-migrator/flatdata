/**
 * Copyright (c) 2018 HERE Europe B.V.
 * See the LICENSE file in the root of this project for license details.
 */

#include "test_structures.hpp"

#include <flatdata/flatdata.h>
#include <boost/filesystem.hpp>
#include "catch_amalgamated.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using namespace flatdata;
using namespace test_structures;

TEST_CASE( "FillingData", "[ExternalVector]" )
{
    auto storage = MemoryResourceStorage::create( );
    auto data = storage->create_external_vector< AStruct >( "data", "foo" );
    REQUIRE( data.size( ) == size_t( 0 ) );
    data.grow( ).value = 10;
    data.grow( ).value = 11;
    data.grow( ).value = 12;
    REQUIRE( data.size( ) == size_t( 3 ) );

    boost::optional< ArrayView< AStruct > > view_from_close = data.close( );
    REQUIRE( static_cast< bool >( view_from_close ) );
    REQUIRE( view_from_close->size( ) == size_t( 3 ) );
    CHECK( ( *view_from_close )[ 0 ].value == uint64_t( 10 ) );
    CHECK( ( *view_from_close )[ 1 ].value == uint64_t( 11 ) );
    CHECK( ( *view_from_close )[ 2 ].value == uint64_t( 12 ) );

    auto view_from_storage = storage->read< ArrayView< AStruct > >( "data", "foo" );
    REQUIRE( static_cast< bool >( view_from_storage ) );
    REQUIRE( view_from_storage->size( ) == size_t( 3 ) );
    CHECK( ( *view_from_storage )[ 0 ].value == uint64_t( 10 ) );
    CHECK( ( *view_from_storage )[ 1 ].value == uint64_t( 11 ) );
    CHECK( ( *view_from_storage )[ 2 ].value == uint64_t( 12 ) );
}

namespace
{
class Barrier
{
private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    size_t m_count;

public:
    explicit Barrier( size_t count )
        : m_count{count}
    {
    }

    void
    wait( )
    {
        std::unique_lock< std::mutex > lock{m_mutex};
        m_count -= 1;
        if ( m_count == 0 )
        {
            m_cv.notify_all( );
        }
        else
        {
            m_cv.wait( lock, [this] { return m_count == 0; } );
        }
    }
};

void
run_close_in_loop( std::unique_ptr< ResourceStorage > storage )
{
    for ( size_t i = 0; i < 1000; ++i )
    {
        constexpr size_t NUM_THREADS = 4;
        std::vector< std::thread > threads;
        Barrier barrier( NUM_THREADS );
        for ( uint32_t thread_id = 0; thread_id < NUM_THREADS; ++thread_id )
        {
            threads.emplace_back( [&storage, thread_id, &barrier] {
                std::string resource_name = "data_" + std::to_string( thread_id );
                auto data
                    = storage->create_external_vector< AStruct >( resource_name.c_str( ), "foo" );
                data.grow( ).value = 10;
                data.grow( ).value = 11;
                data.grow( ).value = 12;
                barrier.wait( );
                data.close( );
            } );
        }
        for ( uint32_t thread_id = 0; thread_id < NUM_THREADS; ++thread_id )
        {
            threads[ thread_id ].join( );
        }
    }
}
}  // namespace

TEST_CASE( "Close is thread safe for memory resource storage", "[ExternalVector]" )
{
    run_close_in_loop( MemoryResourceStorage::create( ) );
}

TEST_CASE( "Close is thread safe for file resource storage", "[ExternalVector]" )
{
    auto tmpdir = boost::filesystem::temp_directory_path( ) / boost::filesystem::unique_path( );
    boost::filesystem::create_directory( tmpdir );
    run_close_in_loop( FileResourceStorage::create( tmpdir.c_str( ) ) );
    boost::filesystem::remove_all( tmpdir );
}
