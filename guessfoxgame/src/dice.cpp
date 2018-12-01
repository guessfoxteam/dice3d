#include "guessfoxgame.hpp"



namespace gfoxcontract {

/*
   void guessfoxgame::add_player(name player, name referrer, bool bet_free) {


      name inviter = TEAM_ACCOUNT;
      if (referrer.value > 0) {
         inviter =referrer;
         eosio_assert(player != inviter, "inviter can not be self");
         eosio_assert(is_account(inviter), "referrer does not exist"); 
      }


      auto plyr_pos = _players.find(player.value);
      auto ref_pos = _players.find(inviter.value);

      if (plyr_pos == _players.end()) {
         _players.emplace(_self, [&](auto& info) {
            info.account = player;
            info.inviter = inviter;
            info.mask = asset(0, EOS_SYMBOL);
            info.win = asset(0, EOS_SYMBOL);
            info.gen = asset(0, EOS_SYMBOL);
            info.aff = asset(0, EOS_SYMBOL);
            info.round_assets = asset(0, EOS_SYMBOL);
            info.last_roundid = 0;
            
            if (bet_free == true) {
               info.free_times = 0;
               info.total_free_tiems = 0;
            }
            else {
               info.free_times = 1;
               info.total_free_tiems = 1;
            }
         });

         if (ref_pos == _players.end()) {
            _players.emplace(_self, [&](auto& info) {
               info.account = inviter;
               info.free_times = 0;
               info.total_free_tiems = 0;
               info.mask = asset(0, EOS_SYMBOL);
               info.win = asset(0, EOS_SYMBOL);
               info.gen = asset(0, EOS_SYMBOL);
               info.aff = asset(0, EOS_SYMBOL);
               info.round_assets = asset(0, EOS_SYMBOL);
               info.last_roundid = _global_state.cur_round_id;
               
            });
         } else {
            _players.modify(ref_pos, _self, [&](auto& info) {
               if (info.total_free_tiems < 20) {
                  info.free_times += 1;
                  info.total_free_tiems += 1;
               }
            });
         }

      } else {
         if (bet_free == true) {
            eosio_assert(plyr_pos->free_times > 0, "You don't have free times");
            _players.modify(plyr_pos, _self, [&](auto& info){
               info.free_times -= 1;
            });   
         }         
      }
      
   }
*/

/*
   void guessfoxgame::dicefree(const name player, const uint8_t& roll_type, const uint8_t& roll_border, const name& referrer) {
      require_auth(player);

      eosio_assert(_global_state.active == true, "game has not been actived!");

      eosio_assert(player != referrer, "referrer can not be self");

      bool bet_free = true;
      add_player(player, referrer, bet_free);
      
      asset quantity = _global_state.min_bet;

      dicedeferfed(player, bet_free, quantity, roll_type, roll_border);

   }
*/


   void guessfoxgame::transfer(name from, name to, asset quantity, string memo) {

      if (from == _self || to != _self) {
         return;
      }

      if (from == "eosio.stake"_n || memo == "deposit") {
         return;
      }       


      eosio_assert(_global_state.active == true, "game has not been actived!");

      eosio_assert(quantity.symbol == EOS_SYMBOL, "only EOS token allowed");
      eosio_assert(quantity.is_valid(), "quantity invalid");      
      eosio_assert(quantity.amount >= _global_state.min_bet.amount && quantity.amount <= _global_state.max_bet.amount, "transfer quantity must be greater than 0.1 and less than 100");   


      // parse memo
      memo_dice_transfer object = dice_parse_memo(memo);

      eosio_assert(object.roll_border >= ROLL_BORDER_MIN && object.roll_border <= ROLL_BORDER_MAX, "Bet border must between 2 to 97");

      name inviter = TEAM_ACCOUNT;
      if (object.referrer.value > 0) {
         inviter = object.referrer;
         eosio_assert(from != inviter, "inviter can not be self");
         eosio_assert(is_account(inviter), "referrer does not exist"); 
      }

      auto plyr_pos = _players.find(from.value);
      auto ref_pos = _players.find(inviter.value);

      if (plyr_pos == _players.end()) {
         _players.emplace(_self, [&](auto& info) {
            info.account = from;
            info.inviter = inviter;
            info.mask = asset(0, EOS_SYMBOL);
            info.win = asset(0, EOS_SYMBOL);
            info.gen = asset(0, EOS_SYMBOL);
            info.aff = asset(0, EOS_SYMBOL);
            info.round_assets = asset(0, EOS_SYMBOL);
            info.last_roundid = 0;
         });

         if (ref_pos == _players.end()) {
            _players.emplace(_self, [&](auto& info) {
               info.account = inviter;
               info.mask = asset(0, EOS_SYMBOL);
               info.win = asset(0, EOS_SYMBOL);
               info.gen = asset(0, EOS_SYMBOL);
               info.aff = asset(0, EOS_SYMBOL);
               info.round_assets = asset(0, EOS_SYMBOL);
               info.last_roundid = _global_state.cur_round_id;
               
            });
         } 

      } 


      // bool bet_free = false;
      // add_player(from, memo_dice.referrer, bet_free);
      
      dicedeferfed(from, quantity, object.roll_type, object.roll_border);
   }



   void guessfoxgame::dicedeferfed(name player, asset amount, 
                                       uint8_t roll_type, uint64_t roll_border) {

      transaction t;
      t.actions.emplace_back( permission_level{_self, "active"_n},
                              _self, "dicestart"_n,
                              std::make_tuple( player, amount, roll_type, roll_border )
      );
      t.delay_sec = 1;
      t.send( next_id(), _self ); 
      
   }


   void guessfoxgame::dicestart(name player, asset amount, 
                                    uint8_t roll_type, uint64_t roll_border) {
      require_auth(_self);

      eosio_assert(_global_state.active == true, "game has not been actived!");
      
      transaction t;
      t.actions.emplace_back( permission_level{_self, "active"_n},
                              _self, "diceresolved"_n,
                              std::make_tuple( player, amount, roll_type, roll_border )
      );
      t.delay_sec = 0;
      t.send( next_id(), _self );
   }

   // compute roul random
   uint64_t guessfoxgame::dice_random(const name& player) {
      capi_checksum256 result;
   
      size_t hash = 0;
      hash_combine(hash, std::string(" http://dice3d.win/dice " + (name{player}).to_string() + " 3256781# "));
      
      auto rnd = _rounds.find(_global_state.cur_round_id);
      auto mixedBlock = tapos_block_prefix() + tapos_block_num() + (rnd->amount.amount>>1) + hash + current_time();

      const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
      sha256((char *)mixedChar, sizeof(mixedChar), &result);
      const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&result);

      _seed = result;

      return p64[1] % DICE_BET_MAX_NUM;
   }



   void guessfoxgame::diceresolved(name player, asset amount, 
                                       uint8_t roll_type, uint64_t roll_border) {
      require_auth(_self);

      eosio_assert(_global_state.active == true, "game has not been actived!");

      uint64_t roll_value = dice_random(player);
      // print(" roll_value: ", roll_value);

      auto bet_id = _global_state.cur_bet_id + 1;
      asset payout = asset(0, EOS_SYMBOL);

      if ( (roll_type == ROLL_TYPE_SMALL && roll_value < roll_border) || (roll_type == ROLL_TYPE_BIG && roll_value > roll_border) ) {
         payout = compute_payout_dice(roll_type, roll_border, amount);


         char str[128];
         sprintf(str, "bet id: %lld. winner! [https://dice3d.win]", bet_id);

         INLINE_ACTION_SENDER(eosio::token, transfer)(
            "eosio.token"_n, { {_self, "active"_n} },
            { _self, player, payout, std::string(str) }
         );

      }


      auto ct = current_time();
      auto ctp = current_time_point();


      // modify player table with new bet time
      auto plyr_pos = _players.find(player.value);
      eosio_assert(plyr_pos != _players.end(), "can not find this account");

      _players.modify(plyr_pos, _self, [&](auto& info) {
         info.last_betid = bet_id;
         info.last_bettime = ct;

         if (info.last_roundid != _global_state.cur_round_id) {
            info.round_assets = amount;
         } else {
            info.round_assets += amount;
         }

      });

      name inviter = plyr_pos->inviter;
      // print(" inviter: ", inviter);

  
      _global_state.cur_bet_id = bet_id;
      bool is_free = false;

      save_bet(bet_id, is_free, player, inviter, amount, payout, roll_type, roll_border, roll_value, _seed, ctp);


      // send dicereceipt to player
      SEND_INLINE_ACTION( *this, dicereceipt, { {_self, "active"_n} },
                          { bet_id, player, amount, payout, roll_type, roll_border, roll_value, _seed, ctp }
      );




      // 1.4% of amount to f3d
      asset to_f3d = amount * _global_state.f3d_fee / 1000;
      // print(" to_f3d: ", to_f3d);

      // 0.6% of amount to p3d
      asset to_p3d = amount * _global_state.p3d_fee / 1000;
      // print(" to_p3d: ", to_p3d);

      // f3d: buykeys
      transaction t;
      t.actions.emplace_back( permission_level{_self, "active"_n},
                              _self, 
                              "buykeys"_n,
                              std::make_tuple( player, to_f3d )
      );
      t.delay_sec = 0;
      t.send( next_id(), _self );
     
     
      // p3d: minetokens
      if (to_p3d.amount > 0) {

         transaction tx;
         tx.actions.emplace_back( permission_level{_self, "active"_n},
                                 "eosio.token"_n, 
                                 "transfer"_n,
                                 std::make_tuple( _self, BANK_ACCOUNT, to_p3d, std::string("mine;") + (name{player}).to_string() + ';' + (name{inviter}).to_string() + ';' )
         );
         tx.delay_sec = 1;
         tx.send( next_id(), _self );          
      } 

      // if (to_p3d.amount > 0) {

      //    transaction tx;
      //    tx.actions.emplace_back( permission_level{_self, "active"_n},
      //                            "eosio.token"_n, 
      //                            "transfer"_n,
      //                            std::make_tuple( _self, FUND_ACCOUNT, to_p3d, std::string("mine;") + (name{player}).to_string() + ';' + (name{inviter}).to_string() + ';' )
      //    );
      //    tx.delay_sec = 1;
      //    tx.send( next_id(), _self );          
      // }


   }


   void guessfoxgame::dicereceipt(uint64_t bet_id, name player, asset bet_asset, asset payout, 
                                    uint8_t roll_type, uint64_t roll_border, uint64_t roll_value, capi_checksum256 seed, time_point bet_time){
      require_auth(_self);
      require_recipient(player);
   }  



    void guessfoxgame::init_bet(bet_item& a, uint64_t id, uint64_t bet_id, bool is_free, name player, name inviter, 
                  asset bet_asset, asset payout, uint8_t roll_type, uint64_t roll_border,
                  uint64_t roll_value, capi_checksum256 seed, time_point time
                  )
    {
      a.id = id;
      a.bet_id = bet_id;
      a.is_free = is_free,
      a.player = player;
      a.inviter = inviter;
      
      a.bet_asset = bet_asset;
      a.payout = payout;
      a.roll_type = roll_type;
      a.roll_border = roll_border;
      a.roll_value = roll_value;
      a.seed = seed;
      a.time = time;
    }


    void guessfoxgame::save_bet( 
                  uint64_t bet_id, bool is_free, name player, name inviter, 
                  asset bet_asset, asset payout, uint8_t roll_type, uint64_t roll_border, uint64_t roll_value,
                  capi_checksum256 seed, time_point time
                  )
    {

      auto global_itr = _global2.find(GLOBAL_ID_HISTORY_INDEX);

      uint64_t history_index = global_itr->val % BET_HISTORY_LEN + 1;

      auto bet_itr = _bets.find(history_index);

      if (bet_itr != _bets.end())
      {
        // auto lambda_func = [&](auto& a) {};
        _bets.modify(bet_itr, _self, [&](auto& a) {
          init_bet(a, a.id, bet_id, is_free, player, 
                  inviter, bet_asset, payout, roll_type, 
                  roll_border, roll_value, seed, time);
        });
      }
      else
      {
        _bets.emplace(_self, [&](auto &a) {
          init_bet(a, history_index, bet_id, is_free, player, 
                  inviter, bet_asset, payout, roll_type, 
                  roll_border, roll_value, seed, time);
        });
      }

      _global2.modify(global_itr, _self, [&](auto& a) {
        a.val = history_index;
      });
    }



   // void guessfoxgame::diceerase(uint64_t limit) {
   //    require_auth(_self);

   //    uint64_t count = 0;

   //    for (auto it = _dice_bets.begin(); it != _dice_bets.end() && count <= limit; ) {
   //       it = _dice_bets.erase(it);
   
   //       count++;
   //    }

   // }




} /// namespace gfoxcontract
