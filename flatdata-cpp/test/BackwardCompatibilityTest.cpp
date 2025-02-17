/**
 * Copyright (c) 2018 HERE Europe B.V.
 * See the LICENSE file in the root of this project for license details.
 */

#include "test_structures.hpp"

#include <flatdata/flatdata.h>
#include "catch_amalgamated.hpp"

#include <array>

namespace flatdata
{
using namespace test_structures;
using namespace test_structures::backward_compatibility;
namespace tbi = test_structures::backward_compatibility::internal;

/**
 *             ,d
 *             88
 * ,adPPYba, MM88MMM ,adPPYba,  8b,dPPYba,
 * I8[    ""   88   a8"     "8a 88P'    "8a
 *  `"Y8ba,    88   8b       d8 88       d8
 * aa    ]8I   88,  "8a,   ,a8" 88b,   ,a8"
 * `"YbbdP"'   "Y888 `"YbbdP"'  88`YbbdP"'
 *                              88
 *                              88
 *
 * These tests freeze binary layout of flatdata resources:
 * - Instance
 * - Vector
 * - Multivector
 * - RawData
 *
 * As binary format is not part of flatdata schema, we freeze it. If format of existing resources
 * has to change, consider adding new resource (for example, `vector2` or `v2vector`). This will
 * save flatdata customers from undefined behavior in case software and archive are incompatible.
 *
 * If you have more questions, please contact flatdata maintainers, among which:
 * - Alexey Kolganov
 * - Christian Vetter
 * - boxdot <d@zerovolt.org>
 */

namespace
{
template < typename Array, typename Storage >
void
compare_byte_arrays( const Array& expected,
                     const flatdata::MemoryDescriptor actual,
                     const Storage& storage )
{
    INFO( "Sizes differ. Hexdump: \n" << storage.hexdump( ) );
    CHECK( actual.size_in_bytes( ) + 1 == expected.size( ) );

    for ( size_t i = 0; i < actual.size_in_bytes( ); ++i )
    {
        INFO( "Difference at position " << i << ". Hexdump: \n" << storage.hexdump( ) );
        CHECK( actual.data( )[ i ] == static_cast< uint8_t >( expected[ i ] ) );
    }
}

void
fill_signed_struct( SignedStructMutator s )
{
    REQUIRE( s.size_in_bytes( ) == size_t( 10 ) );
    s.a = -0x1;
    s.b = 0x01234567;
    s.c = -0x28;
    s.d = 0;
}

void
fill_simple_struct( SimpleStructMutator s )
{
    REQUIRE( s.size_in_bytes( ) == size_t( 8 ) );
    s.a = 0xFFFFFFFF;
    s.b = 0xDEADBEEF;
}

void
check_signed_struct( SignedStruct s )
{
    REQUIRE( s.size_in_bytes( ) == size_t( 10 ) );
    REQUIRE( s.a == -0x1 );
    REQUIRE( s.b == 0x01234567u );
    REQUIRE( s.c == -0x28 );
    REQUIRE( s.d == 0u );
}

void
check_simple_struct( SimpleStruct s )
{
    REQUIRE( s.size_in_bytes( ) == size_t( 8 ) );
    REQUIRE( s.a == 0xFFFFFFFFu );
    REQUIRE( s.b == 0xDEADBEEFu );
}

template < typename Archive >
std::shared_ptr< MemoryResourceStorage >
openable_storage( )
{
    std::shared_ptr< MemoryResourceStorage > storage = MemoryResourceStorage::create( );
    auto schema_key = std::string( Archive::name_definition( ) ) + ".archive.schema";
    auto signature_key = std::string( Archive::name_definition( ) ) + ".archive";

    storage->assign_value( schema_key.c_str( ), Archive::schema_definition( ) );
    storage->assign_value( signature_key.c_str( ), MemoryDescriptor( "\0\0\0\0\0\0\0\0"
                                                                     "\0\0\0\0\0\0\0\0",
                                                                     16 ) );
    return storage;
}

std::array< uint8_t, 27 > expected_instance_binary
    = {"\x0a\x00\x00\x00\x00\x00\x00\x00"    // Size of payload in bytes
       "\xff\xac\x68\x24\x00\x0b\x00\x00"    // Payload
       "\x00\x00"                            // Payload
       "\x00\x00\x00\x00\x00\x00\x00\x00"};  // Padding

std::array< uint8_t, 37 > expected_vector_binary
    = {"\x14\x00\x00\x00\x00\x00\x00\x00"    // Payload size in bytes
       "\xff\xac\x68\x24\x00\x0b\x00\x00"    // Payload
       "\x00\x00\xff\xac\x68\x24\x00\x0b"    // Payload
       "\x00\x00\x00\x00"                    // Payload
       "\x00\x00\x00\x00\x00\x00\x00\x00"};  // Padding

std::array< uint8_t, 66 > expected_multivector_data
    = {"\x31\x00\x00\x00\x00\x00\x00\x00"              // Payload size in bytes
       "\x01\xff\xac\x68\x24\x00\x0b\x00\x00\x00\x00"  // Payload
       "\x00\xff\xff\xff\xff\xef\xbe\xad\xde"          // Payload
       "\x00\xff\xff\xff\xff\xef\xbe\xad\xde"          // Payload
       "\x01\xff\xac\x68\x24\x00\x0b\x00\x00\x00\x00"  // Payload
       "\x00\xff\xff\xff\xff\xef\xbe\xad\xde"          // Payload
       "\x00\x00\x00\x00\x00\x00\x00\x00"};            // Padding

std::array< uint8_t, 42 > expected_multivector_index
    = {"\x19\x00\x00\x00\x00\x00\x00\x00"    // Index size in bytes
       "\x00\x00\x00\x00\x00"                // Data pointer 1
       "\x14\x00\x00\x00\x00"                // Data pointer 2
       "\x14\x00\x00\x00\x00"                // Data pointer 3
       "\x28\x00\x00\x00\x00"                // Data pointer 4
       "\x31\x00\x00\x00\x00"                // Sentinel (end of data 4)
       "\x00\x00\x00\x00\x00\x00\x00\x00"};  // Padding

std::array< uint8_t, 22 > expected_raw_data_binary
    = {"\x05\x00\x00\x00\x00\x00\x00\x00"    // Payload size in bytes
       "\xff\xef\xbe\xad\xde"                // Payload
       "\x00\x00\x00\x00\x00\x00\x00\x00"};  // Padding

const std::string multivector_index_schema = std::string( "index(" )
                                             + tbi::TestMultivector__multivector_resource__schema__
                                             + std::string( ")" );
}  // namespace

TEST_CASE( "Writing instance resources layout", "[BackwardCompatibility]" )
{
    std::shared_ptr< MemoryResourceStorage > storage = MemoryResourceStorage::create( );
    auto builder = TestInstanceBuilder::open( storage );
    REQUIRE( builder.is_open( ) );

    Struct< SignedStruct > v;
    fill_signed_struct( *v );
    builder.set_instance_resource( *v );

    REQUIRE( storage->read_resource( "instance_resource.schema" ).char_ptr( )
             == std::string( tbi::TestInstance__instance_resource__schema__ ) );
    compare_byte_arrays( expected_instance_binary, storage->read_resource( "instance_resource" ),
                         *storage );
}

TEST_CASE( "Reading instance resources layout", "[BackwardCompatibility]" )
{
    std::shared_ptr< MemoryResourceStorage > storage = openable_storage< TestInstance >( );
    storage->assign_value( "instance_resource",
                           MemoryDescriptor( expected_instance_binary.data( ),
                                             expected_instance_binary.size( ) - 1 ) );
    storage->assign_value( "instance_resource.schema",
                           tbi::TestInstance__instance_resource__schema__ );

    auto archive = TestInstance::open( storage );
    INFO( archive.describe( ) );
    REQUIRE( archive.is_open( ) );
    check_signed_struct( archive.instance_resource( ) );
}

TEST_CASE( "Writing vector resources layout", "[BackwardCompatibility]" )
{
    std::shared_ptr< MemoryResourceStorage > storage = MemoryResourceStorage::create( );
    auto builder = TestVectorBuilder::open( storage );
    REQUIRE( builder.is_open( ) );

    Vector< SignedStruct > v( 2 );
    fill_signed_struct( v[ 0 ] );
    fill_signed_struct( v[ 1 ] );
    builder.set_vector_resource( v );

    REQUIRE( storage->read_resource( "vector_resource.schema" ).char_ptr( )
             == std::string( tbi::TestVector__vector_resource__schema__ ) );
    compare_byte_arrays( expected_vector_binary, storage->read_resource( "vector_resource" ),
                         *storage );
}

TEST_CASE( "Reading vector resources layout", "[BackwardCompatibility]" )
{
    std::shared_ptr< MemoryResourceStorage > storage = openable_storage< TestVector >( );
    storage->assign_value(
        "vector_resource",
        MemoryDescriptor( expected_vector_binary.data( ), expected_vector_binary.size( ) - 1 ) );
    storage->assign_value( "vector_resource.schema", tbi::TestVector__vector_resource__schema__ );

    auto archive = TestVector::open( storage );
    INFO( archive.describe( ) );
    REQUIRE( archive.is_open( ) );

    REQUIRE( archive.vector_resource( ).size( ) == 2u );
    check_signed_struct( archive.vector_resource( )[ 0 ] );
    check_signed_struct( archive.vector_resource( )[ 1 ] );
}

TEST_CASE( "Writing multivector resources layout", "[BackwardCompatibility]" )
{
    std::shared_ptr< MemoryResourceStorage > storage = MemoryResourceStorage::create( );
    auto builder = TestMultivectorBuilder::open( storage );
    REQUIRE( builder.is_open( ) );

    auto mv = builder.start_multivector_resource( );
    auto list = mv.grow( );
    fill_signed_struct( list.add< SignedStruct >( ) );
    fill_simple_struct( list.add< SimpleStruct >( ) );

    mv.grow( );  // no data
    list = mv.grow( );
    fill_simple_struct( list.add< SimpleStruct >( ) );
    fill_signed_struct( list.add< SignedStruct >( ) );

    list = mv.grow( );
    fill_simple_struct( list.add< SimpleStruct >( ) );

    mv.close( );

    REQUIRE( storage->read_resource( "multivector_resource.schema" ).char_ptr( )
             == std::string( tbi::TestMultivector__multivector_resource__schema__ ) );
    REQUIRE( storage->read_resource( "multivector_resource_index.schema" ).char_ptr( )
             == multivector_index_schema );
    compare_byte_arrays( expected_multivector_data,
                         storage->read_resource( "multivector_resource" ), *storage );
    compare_byte_arrays( expected_multivector_index,
                         storage->read_resource( "multivector_resource_index" ), *storage );
}

TEST_CASE( "Reading multivector resources layout", "[BackwardCompatibility]" )
{
    auto storage = openable_storage< TestMultivector >( );
    storage->assign_value( "multivector_resource",
                           MemoryDescriptor( expected_multivector_data.data( ),
                                             expected_multivector_data.size( ) - 1 ) );
    storage->assign_value( "multivector_resource.schema",
                           tbi::TestMultivector__multivector_resource__schema__ );

    storage->assign_value( "multivector_resource_index",
                           MemoryDescriptor( expected_multivector_index.data( ),
                                             expected_multivector_index.size( ) - 1 ) );
    storage->assign_value( "multivector_resource_index.schema", multivector_index_schema.c_str( ) );

    auto archive = TestMultivector::open( storage );
    INFO( archive.describe( ) );
    REQUIRE( archive.is_open( ) );

    auto mv = archive.multivector_resource( );
    size_t number_of_expected_structs = 0;
    mv.for_each( 0, make_overload(
                        [&]( SimpleStruct s ) {
                            check_simple_struct( s );
                            number_of_expected_structs++;
                        },
                        [&]( SignedStruct s ) {
                            check_signed_struct( s );
                            number_of_expected_structs++;
                        } ) );

    mv.for_each(
        1, make_overload( [&]( SimpleStruct ) { FAIL( ); }, [&]( SignedStruct ) { FAIL( ); } ) );

    mv.for_each( 2, make_overload(
                        [&]( SimpleStruct s ) {
                            check_simple_struct( s );
                            number_of_expected_structs++;
                        },
                        [&]( SignedStruct s ) {
                            check_signed_struct( s );
                            number_of_expected_structs++;
                        } ) );

    mv.for_each( 3, make_overload(
                        [&]( SimpleStruct s ) {
                            check_simple_struct( s );
                            number_of_expected_structs++;
                        },
                        [&]( SignedStruct ) { FAIL( ); } ) );
    REQUIRE( number_of_expected_structs == 5u );
}

TEST_CASE( "Writing raw data resources layout", "[BackwardCompatibility]" )
{
    std::shared_ptr< MemoryResourceStorage > storage = MemoryResourceStorage::create( );
    auto builder = TestRawDataBuilder::open( storage );
    REQUIRE( builder.is_open( ) );

    std::array< uint8_t, 6 > raw_data = {"\xff\xef\xbe\xad\xde"};
    builder.set_raw_data_resource(
        flatdata::MemoryDescriptor( raw_data.data( ), raw_data.size( ) - 1 ) );

    compare_byte_arrays( expected_raw_data_binary, storage->read_resource( "raw_data_resource" ),
                         *storage );
}

TEST_CASE( "Reading raw data resources layout", "[BackwardCompatibility]" )
{
    auto storage = openable_storage< TestRawData >( );
    storage->assign_value( "raw_data_resource",
                           MemoryDescriptor( expected_raw_data_binary.data( ),
                                             expected_raw_data_binary.size( ) - 1 ) );
    storage->assign_value( "raw_data_resource.schema",
                           tbi::TestRawData__raw_data_resource__schema__ );

    auto archive = TestRawData::open( storage );
    INFO( archive.describe( ) );
    REQUIRE( archive.is_open( ) );

    std::array< uint8_t, 6 > expected = {"\xff\xef\xbe\xad\xde"};
    compare_byte_arrays( expected, archive.raw_data_resource( ), *storage );
}

}  // namespace flatdata
