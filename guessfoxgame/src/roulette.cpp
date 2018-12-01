// #include "guessfoxgame.hpp"


// namespace gfoxcontract {



//    void guessfoxgame::roulfree(const name& player, const uint8_t& color, const name& referrer) { 
//       require_auth(player);

//       print(" [roulfree] ");

//       eosio_assert(player != referrer, "referrer can not be self");
//       eosio_assert(color >= 1 && color <= 6, "wrong color");
//       eosio_assert(_global_state.active == true, "game has not been actived!");


//       auto plyr_pos = _players.find(player.value);
//       auto ref_pos = _players.find(referrer.value);

//       if (plyr_pos == _players.end()) {
//          // new player
//          _players.emplace(_self, [&](auto& info) {
//             info.account = player;
//             info.free_times = 0;
//             info.inviter = referrer;     
//          });

//          if (ref_pos != _players.end()) {
//             _players.modify(ref_pos, _self, [&](auto& info) {
//                info.free_times += 1;
//             });
//          }

//       } else {
//          eosio_assert(plyr_pos->free_times > 0, "You don't have free times");
//          _players.modify(plyr_pos, _self, [&](auto& info){
//             info.free_times -= 1;
//          });
//       }
      
//       bool bet_free = true;
//       asset quantity = _global_state.min_bet;
//       vector<st_choice> choices;

//       choices.push_back({color, quantity});

//       rouldeferfed(player, bet_free, quantity, choices);

//    }


//    void guessfoxgame::roultransfer(const name& from,
//                                    const name& to,
//                                    const asset& quantity,
//                                    const string& memo) {

//       if (from == _self || to != _self) {
//          return;
//       }

//       eosio_assert(_global_state.active == true, "game has not been actived!");
//       eosio_assert(quantity.amount >= _global_state.min_bet.amount, "transfer quantity must be greater than min bet");   

//       memo_roul_transfer memo_roul = roul_parse_memo(memo);

//       //check referrer
//       eosio_assert(memo_roul.referrer != from, "referrer can not be self");
//       eosio_assert(memo_roul.agent != from, "agent can not be self");

//       bool bet_free = false;
//       vector<st_choice> choices;
//       get_roul_choice(quantity, memo_roul.colors, memo_roul.keys, choices);

//       auto plyr_pos = _players.find(from.value);
//       auto ref_pos = _players.find(memo_roul.referrer.value);
//       auto agt_pos = _players.find(memo_roul.agent.value);

//       if (plyr_pos == _players.end()) {
//          // new player
//          _players.emplace(_self, [&](auto& info) {
//             info.account = from;
//             info.free_times = 1;

//             if (memo_roul.referrer.value != 0 && ref_pos != _players.end()) {
//                info.inviter = memo_roul.referrer;   
//             }
            
//             if (memo_roul.referrer.value == 0 && memo_roul.agent.value != 0) {
//                info.inviter = memo_roul.agent;
//             }    
//          });

//          // add referrer free times
//          if (ref_pos != _players.end()) {
//             _players.modify(ref_pos, _self, [&](auto& info) {
//                info.free_times += 1;
//             });
//          }

//          // if not find agent, have a name registered
//          if (memo_roul.referrer.value == 0 && memo_roul.agent.value != 0 && agt_pos == _players.end()) {
//             _players.emplace(_self, [&](auto& info) {
//                info.account = memo_roul.agent;
//                info.aff = asset(0, EOS_SYMBOL);
//             });
//          }
//       }

//       rouldeferfed(from, bet_free, quantity, choices);
//    }


//    void guessfoxgame::rouldeferfed(const name& player, const bool& bet_free, const asset& amount, const vector<st_choice>& choices) {

//       transaction t;
//       t.actions.emplace_back( permission_level{_self, "active"_n},
//                               _self, "roulstart"_n,
//                               std::make_tuple( player, bet_free, amount, choices )
//       );
//       t.delay_sec = 1;
//       t.send( next_id(), _self );

//    }


//    void guessfoxgame::roulstart(const name& player, const bool& bet_free, const asset& amount, const vector<st_choice>& choices) {
//       require_auth(_self);

//       eosio_assert(_global_state.active == true, "game has not been actived!");
      
//       transaction t;
//       t.actions.emplace_back( permission_level{_self, "active"_n},
//                               _self, "roulresolved"_n,
//                               std::make_tuple( player, bet_free, amount, choices )
//       );
//       t.delay_sec = 0;
//       t.send( next_id(), _self );

//    }


//    // compute roul random
//    uint64_t guessfoxgame::roul_random(const name& player) {
//       capi_checksum256 result;
   
//       size_t hash = 0;
//       hash_combine(hash, std::string("586123 - http://dice3d.win/roul " + (name{player}).to_string()));
      
//       auto rnd = _rounds.find(_global_state.cur_round_id);
//       auto mixedBlock = tapos_block_prefix() + tapos_block_num() + rnd->amount.amount + hash + current_time();

//       const char *mixedChar = reinterpret_cast<const char *>(&mixedBlock);
//       sha256((char *)mixedChar, sizeof(mixedChar), &result);
//       const uint64_t *p64 = reinterpret_cast<const uint64_t *>(&result);

//       _seed = result;

//       return (p64[1] % ROUL_BET_MAX_NUM) + 1;
//    }


//    void guessfoxgame::roulresolved(const name& player, const bool& bet_free, const asset& amount, const vector<st_choice>& choices) {
//       require_auth(_self);

//       eosio_assert(_global_state.active == true, "game has not been actived!");

//       uint64_t random_roll = roul_random(player);
//       // print(" random roll: ", random_roll);

//       auto bet_id = _global_state.cur_bet_id + 1;

//       uint64_t lucky_color;
//       asset quantity;
//       asset payout = asset(0, EOS_SYMBOL);

//       if(is_player_win(random_roll, choices, lucky_color, quantity)) {
//          payout = compute_payout_roul(lucky_color, quantity);
//          // print(" payout: ", payout);

//          INLINE_ACTION_SENDER(eosio::token, transfer)(
//             "eosio.token"_n, { {_self, "active"_n} },
//             { _self, player, payout, uint64_string(bet_id) + std::string(" winner! - [https://gfox.win] ") }
//          );

//          // subtract rwawrd from the free fund pool
//          if (bet_free == true && _global_state.free_fund_pool > payout) {
//             _global_state.free_fund_pool -= payout;
//          }
//       }
//       print(" lucky_color: ", lucky_color, " quantity: ", quantity);

//       // subtract bet amount from the free fund pool
//       if (bet_free == true && _global_state.free_fund_pool > payout) {
//          _global_state.free_fund_pool -= amount;   
//       }
      
//       auto ct = current_time();
//       auto ctp = current_time_point();
      
//       _roul_bets.emplace(_self, [&](auto& info) {
//          info.id = bet_id;
//          info.bet_free = bet_free;
//          info.player = player;
//          // info.referrer = referrer;
//          info.amount = amount;
//          info.payout = payout;
//          info.lucky_color = lucky_color;
//          info.choices = choices;
//          info.random_roll = random_roll;
//          info.seed = _seed;
//          info.time = ctp;
//       });


//       // modify player table last bet time
//       auto plyr_pos = _players.find(player.value);
//       eosio_assert(plyr_pos != _players.end(), "roul player not found");

//       _players.modify(plyr_pos, _self, [&](auto& info) {
//          // info.round_assets += amount;
//          info.last_betid = bet_id;
//          info.last_bettime = ct;

//          if (info.last_roundid != _global_state.cur_round_id) {
//             info.round_assets = amount;
//          } else {
//             info.round_assets += amount;
//          }
//       });

//       name inviter = plyr_pos->inviter;

//       _global_state.cur_bet_id = bet_id;


//       // send roulreceipt to player
//       SEND_INLINE_ACTION( *this, roulreceipt, { {_self, "active"_n} },
//                           { bet_id, player, amount, payout, choices, lucky_color, random_roll, _seed, ctp }
//       );

//       // free bet no keys and tokens
//       if (bet_free == true) {
//          return;
//       }

//       // 1.4% of amount to f3d
//       asset to_f3d = amount * 14 / 1000;

//       // 0.6% of amount to p3d
//       asset to_p3d = amount * 6 / 1000;


//       transaction t;
//       t.actions.emplace_back( permission_level{_self, "active"_n},
//                               _self, 
//                               "buykeys"_n,
//                               std::make_tuple( player, to_f3d )
//       );
//       t.delay_sec = 0;
//       t.send( next_id(), _self );


//       transaction tx;
//       tx.actions.emplace_back( permission_level{_self, "active"_n},
//                               "dappminebank"_n, 
//                               "minetokens"_n,
//                               std::make_tuple( player, to_p3d, (name{inviter}).to_string() )
//       );
//       tx.delay_sec = 1;
//       tx.send( next_id(), _self );

//    }


//    void guessfoxgame::roulreceipt(uint64_t bet_id, name player, asset bet_asset, asset payout, vector<st_choice> choices, uint64_t lucky_color, uint64_t random_roll, capi_checksum256 seed, time_point bet_time) {
//       require_auth(_self);
//       require_recipient(player);
//    }   



//    void guessfoxgame::roulerase(uint64_t limit) {
//       require_auth(_self);

//       uint64_t count = 0;

//       for (auto it = _roul_bets.begin(); it != _roul_bets.end() && count <= limit; ) {
//          it = _roul_bets.erase(it);
   
//          count++;
//       }

//    }



// } /// namespace gfoxcontract
