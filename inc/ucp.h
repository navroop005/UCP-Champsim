#include "champsim.h"
#define ATD_SETS 32

class UCP_BLOCK { //this class is for the block present in our ATD
   public:
    UCP_BLOCK(); 
    uint32_t //each block has its own set index, address, valid and lru value.
        setindex,
        address,
        valid,
        lru;
};

class ATD { //this class deals with the ATD's of all the cpu's. ATD's behave like caches only. Each core will have a 32x16 array of atd_blocks to represent a sinlge ATD 
   private:
    UCP_BLOCK*** atd_blocks;
    uint32_t ways;

   public:
    uint64_t* counter; //will count the misses for all the ways in an ATD.This information is passed to the partioning algorithm 
    ATD(uint32_t ways);
    uint32_t lru_victim(uint32_t set, uint64_t addr);
    uint32_t check_hit(uint32_t set, uint32_t addr);
    void lru_update_hit(uint32_t set, uint32_t way);
    void lru_update_miss(uint32_t set, uint32_t way);
    void access(uint32_t set, uint32_t addr);
};

class UCP { //this class contains all the required function to be used in UCP.
   private:
    // uint32_t** UCP_BLOCK;
    uint32_t* ucp_arr;
    uint32_t NUM_CPU, WAYS;
    ATD** atd;
    uint32_t TOTALSETS;
    uint64_t COUNT;

   public:
    UCP(uint32_t NUM_CPU, uint32_t WAYS, uint32_t TOTAL_SETS);
    ~UCP();
    void umon(uint32_t cpu, uint32_t set, uint64_t addr);
    uint32_t getways(uint32_t cpu);
    void ucp_algorithm();
    uint32_t get_max_mu(uint32_t p, uint32_t alloc, uint32_t balance, uint32_t& req);
    uint32_t get_mu_value(uint32_t p, uint32_t alloc, uint32_t allocii);
};
