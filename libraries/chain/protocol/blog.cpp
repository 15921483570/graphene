#include <graphene/chain/protocol/blog.hpp>
#include <fc/utf8.hpp>

namespace graphene { namespace chain {

   void blog_post::validate()const {
      FC_ASSERT( title.size() < 256 );
      FC_ASSERT( tagline.size() < 1024 );
      FC_ASSERT( permlink.size() < 256 );
      FC_ASSERT( summary.size() < body.size() / 4 );
      FC_ASSERT( body.size() && summary.size() );
      for( uint32_t i = 0; i < tags.size(); ++i )
      {
         FC_ASSERT( tags[i].size() < 16 );
         if( i > 0 ) FC_ASSERT( tags[i-1] < tags[i] );
      }
      if( meta_data.size() > 0 ) {
         /// TODO: verify meta_data is JSON
      }
      FC_ASSERT( fc::to_lower( permlink ) == permlink );
      FC_ASSERT( fc::trim_and_normalize_spaces( permlink ) == permlink );
      FC_ASSERT( fc::is_utf8( permlink ) );
      FC_ASSERT( fc::is_utf8( body ) );
      FC_ASSERT( fc::is_utf8( summary ) );
      FC_ASSERT( fc::is_utf8( tagline ) );
      FC_ASSERT( fc::is_utf8( title ) );
      for( auto c : permlink ) {
         FC_ASSERT( c > 0  );
         switch ( c ) {
           case ' ':
           case '\t':
           case '\n':
           case ':':
           case '#':
           case '/':
           case '\\':
           case '%':
           case '=':
           case '@':
           case '~':
           case '.':
              FC_ASSERT( !"invalid permlink character:", "${s}", ("s", std::string()+c ) );
         }
      }
   }

   void comment_data::validate()const {
      FC_ASSERT( body.size() > 0 );
      FC_ASSERT( body.size() < 1024*8 );
      FC_ASSERT( topic != object_id_type() );
      if( meta_data.size() > 0 ) {
         /// TODO: verify meta_data is JSON
      }
   }

} } // namespae graphene::chain
