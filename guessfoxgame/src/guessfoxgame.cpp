#include "guessfoxgame.hpp"
#include <eosiolib/dispatcher.hpp>

#include "dice.cpp"
#include "f3d.cpp"




namespace gfoxcontract {

   guessfoxgame::guessfoxgame(name s, name code,  datastream<const char*> ds): 
   contract(s, code, ds),
   _bets(_self, _self.value),
   _players(_self, _self.value),
   _rounds(_self, _self.value),
   _roundfee(_self, _self.value),
   _global(_self, _self.value),
   _global2(_self, _self.value)
   {
      _global_state = _global.exists() ? _global.get() : get_default_parameters();
   }

   guessfoxgame::~guessfoxgame() {
      _global.set(_global_state, _self);
   }

   global guessfoxgame::get_default_parameters() {
      global global;

      global.cur_bet_id = 0;
      global.cur_round_id = 0;
      global.next_id = 0;
      global.min_bet = asset(1000, EOS_SYMBOL);  // 0.1 EOS
      global.max_bet = asset(100000, EOS_SYMBOL);  // 10 EOS
      global.free_fund_pool = asset(4000000, EOS_SYMBOL);  // 400 EOS
      global.team_fund = asset(0, EOS_SYMBOL);
      global.resv = asset(0, EOS_SYMBOL);
      global.token_account = TOKEN_CONTRACT;
      global.team_account = TEAM_ACCOUNT;
      global.f3d_fee = 14;
      global.p3d_fee = 6;
      global.resv2 = 0;
      global.round_max_interval = 3*3600*uint64_t(1000000); // 3 hours
      global.round_reduce = 5*60*uint64_t(1000000); // 5 minutes
      global.round_inc = 5*uint64_t(1000000); // 5 seconds
      global.round_divi = {60, 5, 10, 12, 8, 5}; // last: 60%, team: 5%, next: 10%, top1: 12%, top2: 8%, top3: 5% 
      global.resv3 = 0;
      global.active = false;
      return global;
   }


   time_point guessfoxgame::current_time_point() {
      const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
      return ct;
   }

   block_timestamp guessfoxgame::current_block_time() {
      const static block_timestamp cbt{ current_time_point() };
      return cbt;
   }

   void guessfoxgame::init() {
      require_auth(_self);

      // start new f3d round
      new_round();

      //global init
      // globalinit();
      SEND_INLINE_ACTION( *this, setglobal, { {_self, "active"_n} }, {GLOBAL_ID_HISTORY_INDEX, 0});      

      // set game active
      SEND_INLINE_ACTION( *this, setactive, { {_self, "active"_n} }, {true});
   }

   void guessfoxgame::setglobal(uint64_t id, uint64_t val) {
        require_auth(_self);

        auto pos = _global2.find(id);
        if (pos == _global2.end()) {
            _global2.emplace(_self, [&](auto& info) {
                info.id = id;
                info.val = val;
            });
        } else {
            _global2.modify(pos, _self, [&](auto& info) {
                info.val = val;
            });
        }
   }


   // void guessfoxgame::globalinit() {
   //    require_auth(_self);

   //    _global2.emplace(_self, [&](auto &a) {
   //      a.id = GLOBAL_ID_HISTORY_INDEX;
   //      a.val = 0;
   //    });
   // }

   void guessfoxgame::setactive(bool active) {
      require_auth(_self);

      _global_state.active = active;
   }  

   void guessfoxgame::setfees(uint8_t f3d_fee, uint8_t p3d_fee) {
      require_auth(_self);

      _global_state.f3d_fee = f3d_fee;
      _global_state.p3d_fee = p3d_fee;
   }

   void guessfoxgame::setminbet(asset quantity) { 
      require_auth(_self);

      eosio_assert(quantity.symbol == EOS_SYMBOL, "only EOS token allowed");
      eosio_assert(quantity.is_valid(), "quantity invalid");
      eosio_assert(quantity.amount >= 1, "quantity must be greater than 0.0001");

      _global_state.min_bet = quantity;
   }


   void guessfoxgame::setmaxbet(asset quantity) { 
      require_auth(_self);

      eosio_assert(quantity.symbol == EOS_SYMBOL, "only EOS token allowed");
      eosio_assert(quantity.is_valid(), "quantity invalid");
      eosio_assert(quantity.amount >= 1, "quantity must be greater than 0.0001");

      _global_state.max_bet = quantity;
   }   


   void guessfoxgame::setfundpoll(asset quantity) { 
      require_auth(_self);

      eosio_assert(quantity.symbol == EOS_SYMBOL, "only EOS token allowed");
      eosio_assert(quantity.is_valid(), "quantity invalid");
      eosio_assert(quantity.amount >= 1, "quantity must be greater than 0.0001");
 
      _global_state.free_fund_pool = quantity;      
   }

   void guessfoxgame::setroundtime(uint64_t max_interval, uint64_t sub, uint64_t add) {
      require_auth(_self);

      _global_state.round_max_interval = max_interval;
      _global_state.round_reduce = sub;
      _global_state.round_inc = add;
   }

   void guessfoxgame::setrounddivi(vector<uint8_t> round_divi) { 
      require_auth(_self);

      eosio_assert(round_divi.size() >= 3, "The number of elements must be greater than 3");

      _global_state.round_divi = round_divi;
   }


   void guessfoxgame::resetend(uint64_t id, uint64_t time) {
      require_auth(_self);

      auto pos = _rounds.find(id);
      eosio_assert(pos != _rounds.end(), "can not find this round id");
      _rounds.modify(pos, _self, [&](auto& info) {
         info.end_time = time;
      });
   }


   void guessfoxgame::erase(name player) {
      require_auth(_self);


      auto pos = _players.find(player.value);
      eosio_assert(pos != _players.end(), "player account not exit");

      _players.erase(pos);


      // _global.remove();

      // for (auto it = _global2.begin(); it != _global2.end(); ) {
      //    it = _global2.erase(it);
      // }

      // for (auto it = _players.begin(); it != _players.end(); ) {
      //    it = _players.erase(it);
      // }

      // for (auto it = _rounds.begin(); it != _rounds.end(); ) {
      //    it = _rounds.erase(it);
      // }

      // for (auto it = _roundfee.begin(); it != _roundfee.end(); ) {
      //    it = _roundfee.erase(it);
      // }

      // for (auto it = _bets.begin(); it != _bets.end(); ) {
      //    it = _bets.erase(it);
      // }
   }





/*
   void guessfoxgame::transfer(const name& from,
                           const name& to,
                           const asset& quantity,
                           const string& memo) {

      // print("[run transfer] ");
      if (from == _self || to != _self) {
         return;
      }

      if (memo == "deposit") {
         return;
      } 

      eosio_assert(quantity.symbol == EOS_SYMBOL, "only EOS token allowed");
      eosio_assert(quantity.is_valid(), "quantity invalid");
      

      std::vector<std::string> parts;
      split(parts, memo, ':');


      eosio_assert(parts.size() == 2, "Incorrect memo format");

      string game = parts[0];
      if (game == "dice") {
         dicetransfer(from, to, quantity, parts[1]);
      } else if (game == "roul") {
         roultransfer(from, to, quantity, parts[1]);
      }

   }
*/











} /// namespace gfoxcontract


extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {

        if(code==name("eosio.token").value && action== name("transfer").value) {
            eosio::execute_action( eosio::name(receiver), eosio::name(code), &gfoxcontract::guessfoxgame::transfer );
        }
        if (code == receiver){
            switch( action ) { 
                EOSIO_DISPATCH_HELPER( gfoxcontract::guessfoxgame,  
                  // guessfoxgame.cpp
                  (init)(erase)(setglobal)(setactive)(setfees)(setminbet)(setmaxbet)(setfundpoll)(setroundtime)(setrounddivi)(resetend)
                  //dice.cpp
                  (dicestart)(diceresolved)(dicereceipt)
                  //roulette.cpp
                  // (roulfree)(roulstart)(roulresolved)(roulreceipt)(roulerase)
                  //f3d.cpp
                  (buykeys)(withdraw)(keyreceipt)
                ) 
            }    
        }
        eosio_exit(0);
    }
}



// EOSIO_DISPATCH( gfoxcontract::guessfoxgame, (active)(setactived)(start)(diceresolved)(dicereceipt)(erase) )



