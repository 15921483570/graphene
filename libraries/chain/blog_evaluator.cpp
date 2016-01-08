#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/blog_evaluator.hpp>
#include <graphene/chain/blog_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {
void_result blog_post_create_evaluator::do_evaluate( const blog_post_create_operation& o )
{
   return void_result();
}
object_id_type blog_post_create_evaluator::do_apply( const blog_post_create_operation& o )
{
   /// TODO: add the data size to the leaky bucket of "Data Rate", perhaps this can be done at
   /// a higher level so it universally applies to all transactions.
   const auto& new_post = db().create<blog_post_object>([&](blog_post_object& obj){
       obj.post = o.post;
       obj.created = db().head_block_time();
       obj.last_update = db().head_block_time();
   });
   return new_post.id;
}

void_result blog_post_update_evaluator::do_evaluate( const blog_post_update_operation& o )
{
   return void_result();
}
void_result blog_post_update_evaluator::do_apply( const blog_post_update_operation& o )
{
   /// TODO: add the data size to the leaky bucket of "Data Rate"
   db().modify( o.post_id(db()), [&](blog_post_object& obj){
       FC_ASSERT( obj.post.author == o.post.author );
       obj.post = o.post;
       obj.last_update = db().head_block_time();
   });
   return void_result();
}

void_result comment_create_evaluator::do_evaluate( const comment_create_operation& o )
{
   return void_result();
}

object_id_type comment_create_evaluator::do_apply( const comment_create_operation& o )
{
   const auto& new_comment = db().create<comment_object>([&](comment_object& obj){
       obj.comment = o.comment;
       obj.created = db().head_block_time();
   });
   return new_comment.id;
}

void_result vote_evaluator::do_evaluate( const vote_operation& o ) {
   FC_ASSERT( db().find_object( o.voting_on ) );
   return void_result();
}

object_id_type vote_evaluator::do_apply( const vote_operation& o )
{
   auto& vote_idx = db().get_index_type<vote_index>().indices().get<by_voter_voting_on>();
   auto itr = vote_idx.find( boost::make_tuple( o.voter, o.voting_on ) );
   if( itr == vote_idx.end() ) {
      FC_ASSERT( o.weight != 0, "new votes must have a non-zero weight" );
      const auto& new_vote = db().create<vote_object>([&](vote_object& obj){
          obj.voter = o.voter;
          obj.tag   = o.tag;
          obj.vote_time = db().head_block_time();
          obj.last_decay_update = obj.vote_time;
          obj.voting_on = o.voting_on;
          obj.weight = o.weight;
          obj.weight_type = o.weight_type;
      });
      return new_vote.id;
   }
   if( o.weight == 0 ) {
      db().remove( *itr );
      return object_id_type();
   }

   db().modify( *itr, [&]( vote_object& obj ) {
       obj.voter = o.voter;
       obj.tag   = o.tag;
       obj.vote_time = db().head_block_time();
       obj.last_decay_update = obj.vote_time;
       obj.voting_on = o.voting_on;
       obj.weight = o.weight;
       obj.weight_type = o.weight_type;
   });
   return itr->id;
}

} } // namespace graphene::chain
