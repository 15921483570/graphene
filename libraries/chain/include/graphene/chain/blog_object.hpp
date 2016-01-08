
#pragma once

#include <graphene/chain/protocol/types.hpp>

#include <graphene/db/generic_index.hpp>
#include <graphene/db/object.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {

using namespace graphene::db;

class blog_post_object : public abstract_object<blog_post_object> {
   public:
      blog_post          post;

      account_id_type author() const   { return post.author;   }
      string          permlink() const { return post.permlink; }

      fc::time_point_sec created;
      fc::time_point_sec last_update;
};

class comment_object : public abstract_object<comment_object> {
   public:
      account_id_type author() const   { return comment.author;   }

      comment_data         comment;
      fc::time_point_sec   created;
};

/**
 *  This purpose of this record is to track a bidirectional index on what
 *  each account is voting for or against and with what asset types.
 */
struct vote_object : public graphene::db::abstract_object<vote_object>
{
    account_id_type  voter;
    object_id_type   weight_type; ///< may be an asset id, or some other unique identifier for the type of weight
    string           tag;

    time_point_sec   vote_time;
    time_point_sec   last_decay_update;
    object_id_type   voting_on;
    int64_t          weight = 0;  ///< negative weight means voting against
    int32_t          sequence = 0; ///< used in conjunction with voter to ensure a
                                   ///maximum number of outstanding vote objects can be created per account
};

/**
 *  This index needs to be effecient for the following operations:
 *
 *  1. Looking up all votes by voter in a sequence
 *  2. Looking up all votes by voting_on (for quick tally)
 *  3. Looking up a votes by voter/voting_on (for quick updates)
 */
struct by_voter_sequence;
struct by_last_decay_update;
struct by_voting_on;
struct by_voter_voting_on;
typedef multi_index_container<
   vote_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_voter_sequence>,
         composite_key< vote_object,
            member< vote_object, account_id_type, &vote_object::voter>,
            member< vote_object, int32_t, &vote_object::sequence>
         >
      >,
      ordered_unique< tag<by_voting_on>,
         composite_key< vote_object,
            member< vote_object, object_id_type, &vote_object::voting_on>,
            member< object, object_id_type, &object::id >
         >
      >,
      ordered_unique< tag<by_voter_voting_on>,
         composite_key< vote_object,
            member< vote_object, account_id_type, &vote_object::voter>,
            member< vote_object, object_id_type, &vote_object::voting_on>
         >
      >
   >
> vote_multi_index_type;

struct by_author;
struct by_author_tagline;
struct by_author_date;
struct by_date;
typedef multi_index_container<
   blog_post_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_date>, member< blog_post_object, fc::time_point_sec, &blog_post_object::created > >,
      ordered_unique< tag<by_author_tagline>,
         composite_key< blog_post_object,
            const_mem_fun< blog_post_object, account_id_type, &blog_post_object::author>,
            const_mem_fun< blog_post_object, string, &blog_post_object::permlink>
         >
      >,
      ordered_unique< tag<by_author_date>,
         composite_key< blog_post_object,
            const_mem_fun< blog_post_object, account_id_type, &blog_post_object::author>,
            member< blog_post_object, fc::time_point_sec, &blog_post_object::created>
         >,
         composite_key_compare< std::less<account_id_type>, std::greater<fc::time_point_sec> >
      >
   >
> blog_post_multi_index_type;

typedef multi_index_container<
   comment_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_date>, member< comment_object, fc::time_point_sec, &comment_object::created > >,
      ordered_unique< tag<by_author_date>,
         composite_key< comment_object,
            const_mem_fun< comment_object, account_id_type, &comment_object::author>,
            member< comment_object, fc::time_point_sec, &comment_object::created>
         >,
         composite_key_compare< std::less<account_id_type>, std::greater<fc::time_point_sec> >
      >
   >
> comment_multi_index_type;

typedef generic_index<blog_post_object, blog_post_multi_index_type>   blog_post_index;
typedef generic_index<comment_object, comment_multi_index_type>       comment_index;
typedef generic_index<vote_object, vote_multi_index_type>             vote_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::blog_post_object, (graphene::db::object),
   (post)
   (created)
   (last_update)
   )

FC_REFLECT_DERIVED( graphene::chain::comment_object, (graphene::db::object),
   (comment)
   (created)
   )

FC_REFLECT_DERIVED( graphene::chain::vote_object, (graphene::db::object),
   (voter)
   (weight_type)
   (tag)
   (vote_time)
   (last_decay_update)
   (voting_on)
   (weight)
   (sequence)
   )
