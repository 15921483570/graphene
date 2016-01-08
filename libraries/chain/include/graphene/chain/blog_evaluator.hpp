#pragma once
#include <graphene/chain/evaluator.hpp>

namespace graphene { namespace chain {

class blog_post_create_operation;
class blog_post_update_operation;
class comment_create_operation;
class vote_operation;

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
      object_id_type  do_apply( const comment_create_operation& o );
};

class vote_evaluator : public evaluator<vote_evaluator>
{
   public:
      typedef vote_operation operation_type;

      void_result     do_evaluate( const vote_operation& o );
      object_id_type  do_apply( const vote_operation& o );
};

} } // graphene::chain
