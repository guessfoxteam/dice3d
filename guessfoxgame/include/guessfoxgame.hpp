#pragma once

// #include <eosiolib/eosio.hpp>
// #include <eosiolib/asset.hpp>
// #include <eosiolib/privileged.hpp>
// #include <eosiolib/singleton.hpp>
// #include <eosiolib/print.hpp>
// #include <string>

#include "utils.hpp"
#include "config.hpp"
#include "eosio.token.hpp"


// using namespace eosio;
// using namespace std;




namespace gfoxcontract {

   #define HISTORY_ROUND_NUM  5

   #define DICE_BET_MAX_NUM 100
   #define ROUL_BET_MAX_NUM 360 
   #define ROLL_BORDER_MIN 2
   #define ROLL_BORDER_MAX 97
   #define ROLL_TYPE_SMALL 1 
   #define ROLL_TYPE_BIG 2 

   #define DICE_FEE 2


   #define GLOBAL_ID_HISTORY_INDEX 102

   #define BET_HISTORY_LEN 50



   const uint64_t magnitude = uint64_t(1000000); // 10**6

   // const uint64_t usecond = uint64_t(1000000);
   // const uint64_t usecond_hour = 1*3600*usecond;
   // const uint64_t useconds_add_round = 5*usecond;
   // const uint64_t useconds_sub_hour = 5*60*usecond;
   // const uint64_t useconds_max_interval = 3*3600*usecond;

   const uint64_t round_max_interval = 12*3600*uint64_t(1000000); 
   // const uint64_t round_increase = 5*uint64_t(1000000);//5*usecond;
   const uint64_t round_increase = 15*uint64_t(1000000);



    enum em_color{
      CLR_START = 0,
      GREEN,
      YELLOW,
      PURPLE,
      BLUE,
      GOLD,
      RED,
      CLR_END
    };

    struct st_roul_fan{
      em_color color;
      uint64_t degree;
      double odds;
    };

    struct st_choice{
      uint64_t color;
      asset quantity;
    };





   struct [[eosio::table, eosio::contract("guessfoxgame")]] global {

      uint64_t cur_bet_id;
      uint64_t cur_round_id;
      uint64_t next_id;
      name team_account;
      name token_account;
      asset min_bet;
      asset max_bet;
      asset free_fund_pool;
      asset team_fund;
      asset resv;
      uint8_t f3d_fee;
      uint8_t p3d_fee;
      uint8_t resv2;
      uint64_t round_max_interval;
      uint64_t round_reduce;
      uint64_t round_inc;
      uint64_t resv3;
      bool active;
      vector<uint8_t> round_divi; // %: fee for last bet player, to_team, to_next_round, to_bet_top

      EOSLIB_SERIALIZE( global, (cur_bet_id)(cur_round_id)(next_id)(team_account)(token_account)
                        (min_bet)(max_bet)(free_fund_pool)(team_fund)(resv)(f3d_fee)(p3d_fee)(resv2)
                        (round_max_interval)(round_reduce)(round_inc)(resv3)(active)(round_divi) )
   };
   typedef eosio::singleton<"global"_n, global> global_singleton;


    struct [[eosio::table, eosio::contract("guessfoxgame")]] global2_item
    {
      uint64_t id;
      uint64_t val;
      uint64_t primary_key() const { return id; };
      EOSLIB_SERIALIZE(global2_item, (id)(val));
    };
    typedef eosio::multi_index<"global2"_n, global2_item> global2_table;
   


   struct [[eosio::table, eosio::contract("guessfoxgame")]] player_item {
      name account;
      name inviter;
      asset round_assets;
      asset win;
      asset gen;
      asset aff;
      asset resv;
      
      uint32_t free_times;
      uint64_t last_roundid;
      asset mask;
      uint64_t keys;

      uint64_t last_buykeys;
      uint64_t last_betid;
      uint64_t last_bettime;
      uint64_t total_free_tiems;
      

      uint64_t primary_key()const {return account.value;}
      uint64_t get_bettime()const {return last_bettime;}
      uint64_t get_betassets()const {return round_assets.amount;}

      EOSLIB_SERIALIZE( player_item, (account)(inviter)(round_assets)(win)(gen)(aff)(resv)(free_times)
                        (last_roundid)(mask)(keys)(last_buykeys)(last_betid)(last_bettime)(total_free_tiems) )
   };
   typedef eosio::multi_index< "players"_n, player_item,
                               indexed_by<"bybettime"_n, const_mem_fun<player_item, uint64_t, &player_item::get_bettime>  >,
                               indexed_by<"bybetassets"_n, const_mem_fun<player_item, uint64_t, &player_item::get_betassets>  >
                             > players_table;


   struct [[eosio::table, eosio::contract("guessfoxgame")]] round_item {

      uint64_t id;
      asset amount;
      asset pot;
      uint64_t keys;
      asset mask;
      uint64_t start_time;
      uint64_t end_time;
      uint64_t resv;
      uint64_t resv2;
      name last_bet_player;
      bool ended;
      vector<name> bet_top_player;
      uint64_t primary_key()const {return id;}

      EOSLIB_SERIALIZE( round_item, (id)(amount)(pot)(keys)(mask)(start_time)(end_time)(resv)(resv2)(last_bet_player)(ended)(bet_top_player) )
   };
   typedef eosio::multi_index<"rounds"_n, round_item> rounds_table;


   struct [[eosio::table, eosio::contract("guessfoxgame")]] roundfee_item {
      uint64_t id;
      asset to_winner; 
      asset to_team; 
      asset to_next_round;
      asset resv;
      vector<asset> to_bet_top;
      uint64_t primary_key()const {return id;}

      EOSLIB_SERIALIZE( roundfee_item, (id)(to_winner)(to_team)(to_next_round)(resv)(to_bet_top) )
   };
   typedef eosio::multi_index<"roundfee"_n, roundfee_item> roundfee_table;


   struct [[eosio::table, eosio::contract("guessfoxgame")]] bet_item {
      uint64_t id;
      uint64_t bet_id;
      bool is_free;
      name player;
      name inviter;
      asset bet_asset;
      asset payout;
      uint8_t roll_type;
      uint64_t roll_border;
      uint64_t roll_value;
      capi_checksum256 seed;
      time_point time;
      uint64_t primary_key()const {return id;}

      EOSLIB_SERIALIZE( bet_item, (id)(bet_id)(is_free)(player)(inviter)(bet_asset)(payout)(roll_type)
                        (roll_border)(roll_value)(seed)(time) )
   }; 
   typedef eosio::multi_index<"bets"_n, bet_item> bet_table;


   class [[eosio::contract]] guessfoxgame : public eosio::contract {

      private:
         global_singleton _global;
         global _global_state;

         global2_table _global2;

         bet_table _bets;

         players_table _players;

         rounds_table _rounds;
         roundfee_table _roundfee;


         vector<em_color> _roul_color_order;
         vector<st_roul_fan> _roul_color_fan;

         capi_checksum256 _seed;





      public:
        using contract::contract;

        // functions defined in guessfoxgame.cpp        
        guessfoxgame(name receiver, name code, datastream<const char*> ds);

        ~guessfoxgame();
 
        void transfer(name from, name to, asset quantity, string memo);        

        [[eosio::action]]
        void init();

        // void globalinit();
        [[eosio::action]]
        void setglobal(uint64_t id, uint64_t val);

        [[eosio::action]]
        void setactive(bool active);

        [[eosio::action]]        
        void erase(name player);

        [[eosio::action]]
        void setfees(uint8_t f3d_fee, uint8_t p3d_fee);
        // void setstagefees(const double& team_fee, const double& round_fee);

        [[eosio::action]]
        void setminbet(asset quantity);

        [[eosio::action]]
        void setmaxbet(asset quantity);

        [[eosio::action]]
        void setfundpoll(asset quantity);

        [[eosio::action]]
        void setroundtime(uint64_t max_interval, uint64_t sub, uint64_t add);

        [[eosio::action]]
        void setrounddivi(vector<uint8_t> divides);

        [[eosio::action]]
        void resetend(uint64_t id, uint64_t time);




        // [[eosio::action]]
        // void testdivi();


        uint64_t get_random(uint64_t max);


        // functions defined in dice.cpp
        // void add_player(name player, name referrer, bool bet_free); 
        
        // [[eosio::action]]
        // void dicefree(name player, uint8_t roll_type, uint8_t roll_border, name referrer); 
        
        // void dicetransfer(name from, name to, asset quantity, string memo);

        
        uint64_t dice_random(const name& player);

        void dicedeferfed(name player, asset amount, uint8_t roll_type, uint64_t roll_border);
        
        [[eosio::action]]
        void dicestart(name player, asset amount, uint8_t roll_type, uint64_t roll_border);

        [[eosio::action]]
        void diceresolved(name player, asset amount, uint8_t roll_type, uint64_t roll_border);

        [[eosio::action]]
        void dicereceipt(uint64_t bet_id, name player, asset bet_asset, asset payout, uint8_t roll_type, uint64_t roll_border, uint64_t roll_value, capi_checksum256 seed, time_point bet_time);
    

        void init_bet(bet_item& a, uint64_t id, uint64_t bet_id, bool is_free, name player, name inviter, asset bet_asset, asset payout, uint8_t roll_type, uint64_t roll_border, uint64_t roll_value, capi_checksum256 seed, time_point time);

        void save_bet( uint64_t bet_id, bool is_free, name player, name inviter, asset bet_asset, asset payout, uint8_t roll_type, uint64_t roll_border, uint64_t roll_value, capi_checksum256 seed, time_point time);


        // [[eosio::action]] 
        // void diceerase(uint64_t limit);



        // functions defined in f3d.cpp
        [[eosio::action]]
        void buykeys(name player, asset quantity);
        
        void core(name player, asset quantity);

        [[eosio::action]]
        void keyreceipt(name player, asset buy_asset, uint64_t keys, time_point time);

        uint64_t keys_receive(asset quantity);

        double keys(asset quantity);

        [[eosio::action]] 
        void withdraw(name player);

        void end_round();

        void new_round();

        asset withdraw_earnings(name player);

        void update_gen_value(name player);

        asset calc_unmasked_earnings(uint64_t id, name player);

        void distributes(name player, asset quantity, uint64_t keys);

        void update_masks(name player, asset gen, uint64_t keys);

        void update_time();


      private:

         struct memo_dice_transfer {
            uint8_t roll_type;
            uint64_t roll_border;
            name referrer;
            // name agent;
         };

         struct memo_roul_transfer {
            vector<uint64_t> colors;
            vector<uint64_t> keys;
            name referrer;
            name agent;
         };

         //defined in guessfoxgame.cpp
         static global get_default_parameters();
         static time_point current_time_point();
         static block_timestamp current_block_time();

       
         memo_dice_transfer dice_parse_memo(std::string memo) {
            auto res = memo_dice_transfer();

            std::vector<std::string> parts; 
            split(parts, memo, ';'); 

            // print(" dice_size: ", parts.size());
            eosio_assert(parts.size() == 3, "Incorrect memo format");

            res.roll_type = stoi(parts[0]);
            res.roll_border = stoi(parts[1]);
            res.referrer = name(parts[2].c_str());
            // res.agent = name(parts[3].c_str());

            return res;
         }



         asset compute_payout_dice(const uint8_t& roll_type, const uint64_t& roll_border, const asset& offer) {
            return min(max_payout_dice(roll_type, roll_border, offer), max_bonus());
         }

         asset max_payout_dice(const uint8_t& roll_type, const uint64_t& roll_border, const asset& offer) {  
            // const double DDS = 98.0 / ((double)roll_under - 1.0);

            uint8_t fit_num = 98;

            if (roll_type == ROLL_TYPE_SMALL) {
               fit_num = roll_border;
            } else if (roll_type == ROLL_TYPE_BIG) {
               fit_num = DICE_BET_MAX_NUM - 1 - roll_border;
            }
            const double ODDS = 98.0 / (double)fit_num; // FEE: 2%
            // print(" ODDS: ", ODDS);

            return asset(ODDS * offer.amount, offer.symbol);
         }


         asset max_bonus() { return available_balance() / 20; } // 5%

         asset available_balance() {
            // auto token = eosio::token(N(eosio.token));
            // const asset balance = token.get_balance(_self, symbol_type(EOS_SYMBOL).name());
            const asset balance = eosio::token::get_balance(name("eosio.token"), _self, symbol(EOS_SYMBOL).code());
            // const asset locked = get_fund_pool().locked;
            // const asset available = balance - locked;
            const asset available = balance;
            eosio_assert(available.amount >= 0, "fund pool overdraw");
            return available;
         }

         // asset compute_bucket(const asset& offer) { return asset(offer.amount * _global_state.round_fee / 100.0, offer.symbol); }
         asset compute_bucket(const asset& offer) { return asset(offer.amount * 1 / 100, offer.symbol); }

         uint64_t next_id() {
            uint64_t current_id = _global_state.next_id + 1;
            _global_state.next_id = current_id;
            return current_id;
         }



   };
} /// namespace gfoxcontract

