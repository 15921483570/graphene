#pragma once

namespace graphene { namespace chain {

   /** @defgroup voting Voting Subsystem
    *
    *  The idea behind the voting subsystem is to allow any user to vote on any object weighted
    *  by some metric. These metrics could be:
    *
    *     1. stake in a MAS
    *     2. tokens on a blockchain
    *     3. stake commitment 
    *
    *  The system is designed to keep vote tallies simple and updated in real time. For systems
    *  like MAS or STAKE COMMITMENT it isn't possible for the user to transfer their voting weight
    *  to another party before the votes automatically decay.
    *
    *  All votes should decay with a relatively quick half life to ensure that opinion stays fresh 
    *  on what the "current priorities" are.  This means that all votes are answering the question
    *  what should we do "today" or "this week".  When voting is being used to allocate funds or
    *  resources those who get the benefit of those resources have greater incentive to keep their
    *  vote current.  
    *
    */
   ///@{

   /**
    *  This purpose of this record is to track a bidirectional index on what
    *  each account is voting for or against and with what asset types. 
    */
   struct vote_object : public graphene::db::abstract_object<vote_object>
   {
       account_id_type  voter;
       object_id_type   weight_type; ///< may be an asset id, or some other unique identifier for the type of weight
       
       time_point_sec   vote_time;
       string           tag;
       object_id_type   voting_on;
       int64_t          weight = 0;  ///< negative weight means voting against
       int32_t          sequence = 0; ///< used in conjunction with voter to ensure a 
                                      ///maximum number of outstanding vote objects can be created per account

   };

   /**
    *  This table tracks the cumlative votes that each object has sorted by
    *
    *  object type, tag, net_votes, and object id. 
    */
   struct vote_total : public graphene::db::abstract_object<vote_total>
   {
      object_id_type   weight_type; ///< the asset doing the voting
      uint8_t          space = 0; ///< the space of the object being voted on
      uint8_t          type = 0; ///< the type of the object being voted on
      string           tag;  ///< the tag being vote on
      int64_t          votes_for = 0;
      int64_t          votes_against = 0;
      uint64_t         instance; ///< the instance of the object being voted on
      int64_t          net_votes_high_water_mark = 0;
      int64_t          net_votes_low_water_mark = 0;

      int64_t          net_votes()const { return votes_for - votes_against;               }
      object_id_type   voting_on()const { return object_id_type( space, type, instance ); }
   };


   struct vote_operation : public base_operation
   {
      object_id_type  voter;
      object_id_type  voting_on;
      object_id_type  weight_type; ///< which asset/class is voting 
      string          tag;
      int64_t         weight=0; ///< must be less than or equal to maximum voting weight for voter, ~0 means max weight
   };
   



   ///@}

} }
