#pragma once
#include <graphene/chain/protocol/base.hpp>

namespace graphene { namespace chain {

   struct blog_post {
      blog_post_id_type response_to;
      account_id_type   author;
      string            permlink;
      string            title;
      string            tagline;
      string            summary;
      string            body; ///< limited by maximum transaction size.
      vector<string>    tags; 
      string            meta_data; ///< JSON Object containing extra, non-standard properties

      const string& category()const { static string s; return tags.size() ? tags[0] : s; }
      void validate()const;
   };

   struct comment_data {
      object_id_type   topic; ///< a blog post, user account, or other object in the system
      comment_id_type  reply_to; ///< the comment this is a reply to.
      account_id_type  author; ///< the person writing the comment
      string           body; ///< UTF-8 string
      string           meta_data; ///< JSON Object containing extra, non-standard properties
      void validate()const;
   };

   struct blog_post_create_operation : public base_operation {
      struct fee_parameters_type { 
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; 
         uint32_t price_per_kbyte = 10;
      };

      asset               fee;
      blog_post           post;

      account_id_type fee_payer()const { return post.author; }
      void validate()const { post.validate(); }
   };

   struct blog_post_update_operation : public base_operation {
      struct fee_parameters_type { 
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; 
         uint32_t price_per_kbyte = 10;
      };

      asset               fee;
      blog_post_id_type   post_id;
      blog_post           post;

      account_id_type fee_payer()const { return post.author; }
      void            validate()const { post.validate(); }
   };

   struct comment_create_operation : public base_operation {
      struct fee_parameters_type { 
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; 
         uint32_t price_per_kbyte = 10;
      };

      asset         fee;
      comment_data  comment;

      account_id_type fee_payer()const { return comment.author; }
      void            validate()const { comment.validate(); }
   };

   struct vote_operation : public base_operation {
      struct fee_parameters_type { 
         uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; 
      };

      asset            fee;
      account_id_type  voter;
      string           category;
      object_id_type   voting_on;
      object_id_type   weight_type;
      int64_t          weight = 0;

      account_id_type fee_payer()const { return voter; }
      void            validate()const { }
   };

} } // namespae graphene::chain

FC_REFLECT( graphene::chain::blog_post, (response_to)(author)(permlink)(title)(tagline)(summary)(body)(tags)(meta_data) );
FC_REFLECT( graphene::chain::comment_data, (topic)(reply_to)(author)(body)(meta_data) );

FC_REFLECT( graphene::chain::blog_post_create_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::blog_post_update_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::comment_create_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( graphene::chain::vote_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::blog_post_create_operation, (fee)(post) )
FC_REFLECT( graphene::chain::blog_post_update_operation, (fee)(post_id)(post) )
FC_REFLECT( graphene::chain::comment_create_operation, (fee)(comment) )
FC_REFLECT( graphene::chain::vote_operation, (fee)(voter)(category)(voting_on)(weight_type)(weight) )

