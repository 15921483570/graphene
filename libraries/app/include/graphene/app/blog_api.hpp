/**
 * Copyright (c) Cryptonomex, Inc., All Rights Reserved
 * See LICENSE.md for full license.
 */
#pragma once

#include <graphene/app/api.hpp>

#include <graphene/chain/protocol/blog.hpp>

#include <fc/time.hpp>

namespace graphene{ namespace app {

using namespace graphene::chain;
using namespace std;

struct post
{
   string            author;
   string            permlink;
   string            title;
   string            summary;
   vector<string>    tags;
};

struct comment
{
   string            author;
   string            body;

   vector<comment>   children;
};

class blog_api
{
   public:
      blog_api( application _app );
      
   /**
    * Each post can be uniquely as a tuple of the author's name and it's permalink.
    * Within the author's doman, each permalink is unique.
    * 
    * @param author Author's name
    * @param perma_link unique perma_link to post
    */
   post get_post_by_perma_link( const string& author, const string& permalink );
   
   /**
    * Gets all posts with a specific tag and sorts the results.
    * 
    * @param tag The tag to search by.
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit The maximum limit of posts to return. Max is 100.
    *
    * @return A list of posts matching the criteria.
    */
   vector< post > get_posts_by_tag( const string& tag, fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 ) const;
   
   /**
    * Gets all posts containing multiple tags and sorts the results.
    *
    * @param tags The tages to filter by.
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit The maximum limit of posts to return. Max is 100.
    *
    * @return A list of posts mathic the criteria.
    */
   vector< post > get_posts_by_tags( const vector< string >& tags, fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 ) const;
      
   /**
    * Get current top posts by category.
    * 
    * @param category The post category.
    * @param limit Maximum number of top posts to return. Max is 100.
    * 
    * @return A list of the current top posts for the category.
    */
   vector< blog_post > get_top_posts_by_category( const string& category, int limit = 100 );
   
   /**
    * Get historic top posts by category. 
    * 
    * @param category The post category
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit Maximum number of posts to return. Max is 100.
    * 
    * @return A list of the historic top posts for the category.
    */
   vector< blog_post > get_historic_top_posts_by_category( const string& category, fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 );
   
   /**
    * Get current top users.
    *
    * @param limit Maximum number of users to return. Max is 100.
    *
    * @return A list of the current top users.
    */
   vector< string > get_top_users( int limit=100 );
   
   /**
    * Get historic top users.
    * 
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit Maximum numver of posts to return. Max is 100.
    * 
    * @return A list of the history top users.
    */
   vector< string > get_historic_top_users( fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 );
   
   /**
    * Get current global top comments.
    * 
    * @param limit Maximum number of comments to return. Max is 100.
    *
    * @return A list of the current top comments.
    */
   vector< comment > get_global_top_comments( int limit = 100 );
   
   /**
    * Get histoic top comments.
    * 
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit Maximum number of comments. Max is 100.
    *
    * @return A list of historic top comments.
    */
   vector< comment > get_historic_top_comments( fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 );
   
   /**
    * Get top comments on an object.
    * 
    * @param obj_id The ID of the object being commented on.
    * @param limit Maximum number of comments. Max is 100.
    *
    * @return A list of the current top comments on an object.
    */
   vector< comment > get_top_comments_on_object( object_id_type obj_id, int limit = 100 );
   
   /**
    * Get historic top comments on an object.
    * 
    * @param obj_id The ID of the object being commented on.
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit Maximum number of comments. Max is 100.
    * 
    * @return A list of historic top comments on an object.
    */
   vector< comment > get_historic_top_comments_on_object( object_id_type obj_id, fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 );
   
   /**
    * Gets comments on an object by time posted.
    *
    * @param obj_id The object commented on.
    * @param start The time to start looking from.
    * @param stop The time to stop looking.
    * @param limit The maximum limit of comments to return. Max is 100.
    *
    * @return A list of comments matching the criteria.
    */
   vector<comment> get_comments_by_date( const object_id_type& obj_id, fc::time_point_sec start, fc::time_point_sec stop, int limit = 100 );
   
   /**
    * Gets comments in an tree based on replies.
    * 
    * @param obj_id The object commented on.
    * @param depth The depth of the comment tree. Default is 3.
    *
    * @return A tree of comments on the particular object.
    */
   vector<comment> get_comment_tree( const object_id_type& obj_id, int depth = 3 );
   
   private:
      application _app;
};

} }