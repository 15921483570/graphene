#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/blog.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

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

   class blog_post_create_evaluator : public evaluator<blog_post_create_evaluator>
   {
      public:
         typedef blog_post_create_operation operation_type;

         void_result do_evaluate( const blog_post_create_operation& o );
         object_id_type do_apply( const blog_post_create_operation& o );
   };

   class blog_post_update_evaluator : public evaluator<blog_post_update_evaluator>
   {
      public:
         typedef blog_post_update_operation operation_type;

         void_result do_evaluate( const blog_post_update_operation& o );
         void_result do_apply( const blog_post_update_operation& o );
   };

   class comment_create_evaluator : public evaluator<comment_create_evaluator>
   {
      public:
         typedef comment_create_operation operation_type;

         void_result     do_evaluate( const comment_create_operation& o );
         object_id_type do_apply( const comment_create_operation& o );
   };

} } // namespage graphene::chain

FC_REFLECT_DERIVED( graphene::chain::blog_post_object, (graphene::db::object),
                    (post)(created)(last_update) )

FC_REFLECT_DERIVED( graphene::chain::comment_object, (graphene::db::object),
                    (comment)(created) )
