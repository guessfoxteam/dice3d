#include "guessfoxgame.hpp"


namespace gfoxcontract {

   /**
    * @dev test
    */


   /**
   * @dev logic runs whenever a buy order is executed.  determines how to handle 
   * incoming eth depending on if we are in an active round or not
   */
   void guessfoxgame::buykeys(name player, asset quantity) {
      require_auth(_self);

      auto rnd_pos = _rounds.find(_global_state.cur_round_id);
      eosio_assert(rnd_pos != _rounds.end(), "can not find this round");


      auto ct = current_time();

      if (ct > rnd_pos->start_time && (ct <= rnd_pos->end_time || (ct > rnd_pos->end_time && rnd_pos->last_bet_player.value == 0))) {

         // call core
         core(player, quantity);

      } else if (ct > rnd_pos->end_time && rnd_pos->ended == false) {

         // end the round (distributes pot) & start new round
         _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
            rnd.ended = true;
         });

         // end round
         end_round();

         // start new round
         new_round();

         // put amount in players vault 
         auto plyr_pos = _players.find(player.value);

         _players.modify(plyr_pos, _self, [&](auto& plyr) {
            plyr.gen += quantity;
         });
         
      }

   }

   /**
   * @dev this is the core logic for any buy/reload that happens while a round 
   * is live.
   */
   void guessfoxgame::core(name player, asset quantity) {

      // mint the new keys
      uint64_t keys = keys_receive(quantity);
      // print(" keys: ", keys);

      // update time
      update_time();

 
      // update player
      auto plyr_pos = _players.find(player.value);
      if (plyr_pos->last_roundid != _global_state.cur_round_id) {

         auto earnings = asset(0, EOS_SYMBOL);
         //calculate last round earnings
         auto pos = _rounds.find(plyr_pos->last_roundid);
         if (pos != _rounds.end()) {
            earnings = calc_unmasked_earnings(plyr_pos->last_roundid, player); 
         }     

         _players.modify(plyr_pos, _self, [&](auto& plyr) {
            
            plyr.last_roundid = _global_state.cur_round_id;
            plyr.keys = keys; 
            // plyr.mask = asset(0, EOS_SYMBOL);
            // plyr.win = asset(0, EOS_SYMBOL);
            // plyr.aff = asset(0, EOS_SYMBOL);
 
            if (earnings.amount > 0) {
               plyr.gen += earnings / magnitude;
            }

         });
      } else {

         _players.modify(plyr_pos, _self, [&](auto& plyr) {
            plyr.keys += keys;
         });
      }

      // update round
      auto rnd_pos = _rounds.find(_global_state.cur_round_id);
      _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
         rnd.amount += quantity;
         rnd.keys += keys;
         rnd.last_bet_player = player;
      });

      // distribute quantity
      distributes(player, quantity, keys);


      auto ctp = current_time_point();

      // send receipt to player
      SEND_INLINE_ACTION( *this, keyreceipt, { {_self, "active"_n} },
                          { player, quantity, keys, ctp }
      );

   }

   void guessfoxgame::keyreceipt(name player, asset buy_asset, uint64_t keys, time_point time) {
      require_auth(_self);
      require_recipient(player);
   }  


   /**
   * @dev calculates number of keys received given X eos 
   * @return amount of ticket purchased
   */
   uint64_t guessfoxgame::keys_receive(asset quantity) {
      auto rnd_pos = _rounds.find(_global_state.cur_round_id);

      double keys_rec = keys(rnd_pos->amount + quantity) - keys(rnd_pos->amount);

      return keys_rec;
   }

   
   double guessfoxgame::keys(asset quantity) {

      double b = 0.0002;
      // double k = 0.0000000001;
      double k = 0.000000000006;
      double y = (double)quantity.amount / 10000.0;

      double keys = ( sqrt( pow(b, 2) + 2*k*y ) - b ) / k;

      return keys;
   }


   /**
   * @dev withdraws all of your earnings.
   */
   void guessfoxgame::withdraw(name player) {

      auto ct = current_time();

      auto rnd_pos = _rounds.find(_global_state.cur_round_id);

      // check to see if round has ended and no one has run round end yet
      if (ct > rnd_pos->end_time && rnd_pos->ended == false && rnd_pos->last_bet_player.value != 0) {


         _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
            rnd.ended = false;
         });

         // end the round
         end_round();


         // get their earnings
         auto earnings = withdraw_earnings(player);

         if (earnings.amount > 0) {
            // TODO: transfer 
            INLINE_ACTION_SENDER(eosio::token, transfer)(
               "eosio.token"_n, { { _self, "active"_n } },
               { _self, player, earnings, std::string("dice3d reward, [https://dice3d.win]") }
            );   
         }
         
         // start new round
         new_round();
      
      // in any other situation   
      } else {

         // get their earnings
         auto earnings = withdraw_earnings(player);

         if (earnings.amount > 0) {
            // TODO: transfer
            INLINE_ACTION_SENDER(eosio::token, transfer)(
               "eosio.token"_n, { { _self, "active"_n } },
               { _self, player, earnings, std::string("dice3d reward, [https://dice3d.win]") }
            );
         }
      }

   }

   /**
   * @dev ends the round. manages paying out winner/splitting up pot
   */
  
/*  
   void guessfoxgame::end_round() {

      auto rnd_pos = _rounds.find(_global_state.cur_round_id);

      // grab our pot amount
      auto pot = rnd_pos->pot;

      // calculate shares
      auto to_win = pot * 60 / 100;
      auto to_com = pot * 5 / 100;
      auto to_gen = pot * 25 / 100;
      auto to_res = pot * 10 / 100;

      // pay our winner
      auto winner = rnd_pos->last_bet_player;

      auto plyr_win = _players.find(winner.value); 
      _players.modify(plyr_win, _self, [&](auto& plyr) {
         plyr.win += to_win;
      });
 
      // calculate ppt for round mask
      auto ppt = to_gen * magnitude / rnd_pos->keys;

      // distribute gen portion to key holders
      _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
         rnd.mask += ppt;
      });

      _roundfee.emplace(_self, [&](auto& info) {
         info.id = _global_state.cur_round_id;
         info.to_winner = to_win;
         info.to_gen = to_gen;
         info.to_com = to_com;
         info.to_next_round = to_res;
      });
   }
*/

   void guessfoxgame::end_round() {

      round_item rnd = _rounds.get( _global_state.cur_round_id );

      eosio_assert(_global_state.round_divi.size() >= 3, "round fee size must be greater than 3");

      auto to_winner = rnd.pot * _global_state.round_divi[0] / 100;
      auto to_team = rnd.pot * _global_state.round_divi[1] / 100;
      auto to_next_round = rnd.pot * _global_state.round_divi[2] / 100;

      // get to bet player fees
      vector<asset> to_top_fees;
      for (auto i=3; i<_global_state.round_divi.size(); i++) {
         auto fee = rnd.pot * _global_state.round_divi[i] / 100;
         to_top_fees.push_back(fee);
      }

      auto time_idx = _players.get_index<"bybettime"_n>();

      // index by last bet time
      map<asset, name> asset_player;
      for (auto itr = time_idx.lower_bound(rnd.start_time); itr != time_idx.upper_bound(rnd.end_time); itr++) {

         asset assets = itr->round_assets;
         name player = itr->account;
         asset_player.insert(pair<asset, name>(assets, player)); 
      }   


      // get top players
      uint8_t count = 0;
      vector<name> bet_top_players;
      for ( auto itr = asset_player.rbegin(); itr != asset_player.rend() && to_top_fees.size() != count; itr++, count++ ) {

         bet_top_players.push_back(itr->second);
      }

      // add winner reward
      auto winner = rnd.last_bet_player;

      auto plyr_win = _players.find(winner.value); 
      _players.modify(plyr_win, _self, [&](auto& plyr) {
         plyr.win += to_winner;
      });  

      // add top player reward
      for (auto i=0; i<bet_top_players.size(); i++) {
         auto player = bet_top_players[i];
         auto fee = to_top_fees[i];

         auto pos = _players.find(player.value);
         _players.modify(pos, _self, [&](auto& plyr) {
            plyr.gen += fee;
         });
      }

      _global_state.team_fund += to_team;
      
      // modify rounds
      auto rnd_pos = _rounds.find(_global_state.cur_round_id);
      _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
         rnd.bet_top_player = bet_top_players;
      });

      // emplace new roundsfee
      _roundfee.emplace(_self, [&](auto& rnd) {
         rnd.id = _global_state.cur_round_id;
         rnd.to_winner = to_winner;
         rnd.to_team = to_team;
         rnd.to_next_round = to_next_round;
         rnd.to_bet_top = to_top_fees;
      });

   }


   /**
   * @dev start new round. 
   */
   void guessfoxgame::new_round() {

      uint64_t next = _global_state.cur_round_id + 1;
      eosio_assert(next > _global_state.cur_round_id, "new round number is smaller than or equal with the old!");

      asset next_bucket = asset(0, EOS_SYMBOL);
      auto rnd_pos = _rounds.find( _global_state.cur_round_id);
      auto fee_pos = _roundfee.find(_global_state.cur_round_id);
      if (rnd_pos != _rounds.end() && fee_pos != _roundfee.end()) {
         next_bucket = fee_pos->to_next_round;
      }

      auto ct = current_time();
      _rounds.emplace(_self, [&](auto& info) {
         info.id            = next;
         info.amount        = asset(0, EOS_SYMBOL);
         info.pot           = next_bucket;
         info.keys          = 0;
         info.mask          = asset(0, EOS_SYMBOL);          
         info.start_time    = ct;
         info.end_time      = ct + round_max_interval;
         info.ended         = false;
      });

      _global_state.cur_round_id = next;

   }

   /**
   * @dev distributes eth based on fees to gen and pot
   */
   void guessfoxgame::distributes(name player, asset quantity, uint64_t keys) {


      // auto to_gen = quantity * 60 / 100; // for general
      auto to_gen = quantity * 35 / 100; // for general
      auto to_pot = quantity * 30 / 100;
      auto to_aff = quantity * 5 / 100;

      auto plyr_pos = _players.find(player.value);
      if (plyr_pos->inviter.value > 0) {
         auto invr_pos = _players.find(plyr_pos->inviter.value);

         if (invr_pos != _players.end()) {
            _players.modify(invr_pos, _self, [&](auto& info) {
               info.aff += to_aff;
            });   
         } else {
            to_aff = asset(0, EOS_SYMBOL);   
         }
         
      } else {
         to_aff = asset(0, EOS_SYMBOL);
      }
      

      auto to_team = quantity - to_gen - to_pot - to_aff;
      // print(" to_gen: ", to_gen, " to_pot: ", to_pot, " to_aff: ", to_aff, " to_team: ", to_team);

      // _global_state.team_fund += to_team;

      update_masks(player, to_gen, keys);

      auto rnd_pos = _rounds.find(_global_state.cur_round_id);
      _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
         // rnd.gen += to_gen;
         rnd.pot += to_pot;
         // rnd.com += to_team;
      });
   }


   /**
   * @dev adds up unmasked earnings, & vault earnings, sets them all to 0
   * @return earnings in wei format
   */   
   asset guessfoxgame::withdraw_earnings(name player) {

      update_gen_value(player);

      auto plyr_pos = _players.find(player.value);

      auto earnings = plyr_pos->win + plyr_pos->gen + plyr_pos->aff;

      if (earnings.amount > 0) {
         _players.modify(plyr_pos, _self, [&](auto& plyr) {
            plyr.win = asset(0, EOS_SYMBOL);
            plyr.gen = asset(0, EOS_SYMBOL);
            plyr.aff = asset(0, EOS_SYMBOL);
         });         
      }

      return earnings;
   }


   /**
   * @dev moves any unmasked earnings to gen vault.  updates earnings mask
   */
   void guessfoxgame::update_gen_value(name player) {

      auto earnings = calc_unmasked_earnings(_global_state.cur_round_id, player);

      if (earnings.amount > 0) {
         auto plyr_pos = _players.find(player.value);

         _players.modify(plyr_pos, _self, [&](auto& plyr) {
            plyr.gen += earnings / magnitude;
            plyr.mask += earnings;
         });
      } 
   }

   /**
   * @dev calculates unmasked earnings (just calculates, does not update mask)
   * @return earnings in wei format
   */   
   asset guessfoxgame::calc_unmasked_earnings(uint64_t id, name player) {
      auto rnd_pos = _rounds.find(id);
      auto plyr_pos = _players.find(player.value);

      auto earnings = rnd_pos->mask * plyr_pos->keys - plyr_pos->mask; 

      return earnings;
   }


   /**
   * @dev updates masks for round and player when keys are bought
   * @return dust left over 
   */
   void guessfoxgame::update_masks(name player, asset gen, uint64_t keys) {

      auto rnd_pos = _rounds.find(_global_state.cur_round_id);
      auto plyr_pos = _players.find(player.value);

      asset round_mask = rnd_pos->mask;
      asset player_mask = plyr_pos->mask;

      // calc profit per key & round mask based on this buy
      asset ppt = (gen * magnitude) / rnd_pos->keys;
      round_mask += ppt;

      // calculate player earning from their own buy (only based on the keys
      // they just bought).  & update player earnings mask
      asset pearn = ppt * keys;
      player_mask += (round_mask * keys - pearn);

      // print(" round_mask2: ", round_mask, " player_mask2: ", player_mask);

      _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
         rnd.mask = round_mask;
      });

      _players.modify(plyr_pos, _self, [&](auto& plyr) {
         plyr.mask = player_mask;
      });
   }

   /**
   * @dev updates round timer based on number of whole keys bought.
   */
   void guessfoxgame::update_time() {

      auto rnd_pos = _rounds.find(_global_state.cur_round_id);
      eosio_assert(rnd_pos != _rounds.end(), "can not find this round");

      auto ct = current_time();

      uint64_t new_time;
      uint64_t end_time;
      if (ct > rnd_pos->end_time && rnd_pos->last_bet_player.value == 0)
         new_time = ct + round_max_interval;
      else 
         new_time = rnd_pos->end_time + round_increase;

      // compare to max and set end time
      if (new_time < ct + round_max_interval)
         end_time = new_time;
      else
         end_time = ct + round_max_interval;

      _rounds.modify(rnd_pos, _self, [&](auto& rnd) {
         rnd.end_time = end_time;
      });

   }


} /// namespace gfoxcontract
