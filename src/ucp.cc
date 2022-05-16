#include "ucp.h"
#define COUNT_LIMIT 1000

uint32_t UCP::get_mu_value(uint32_t p, uint32_t alloc, uint32_t allocii)
{
    uint32_t utility = 0;
    for (uint32_t j = alloc + 1; j <= allocii; j++) {
        utility += atd[p]->counter[j];
    }
    return utility / (allocii - alloc); //this will return thr marginal utility. 
}

uint32_t UCP::get_max_mu(uint32_t p, uint32_t alloc, uint32_t balance, uint32_t& req)
{
    uint32_t maxi = 0;
    for (uint32_t j = 1; j <= balance; j++) {
        uint32_t mu = get_mu_value(p, alloc, alloc + j);

        if (mu > maxi) {
            maxi = mu;
            req = j;
        }
    }
    return maxi; //maximum utilty of an application is returned when the number of blocks allocated to is increased from 1 to number of available blocks 
}

void UCP::ucp_algorithm()
{
    uint32_t balance = (WAYS-NUM_CPU); 
    uint32_t allocations[NUM_CPU]; //initially, each application(cores) has one ways associated with it. 

    for (uint32_t i = 0; i < NUM_CPU; i++) {
        allocations[i] = 1;
    }

    while (balance>0) {
        uint32_t max_mu[NUM_CPU]; //will store the maximum utility of all the blocks in an array
        uint32_t blocks_req[NUM_CPU]; //will store the minimum number of blocks required to reach the maximum utility of every core.
        //cout<<"BALANCE:"<<balance<<endl;
        for (uint32_t i = 0; i < NUM_CPU; i++) {
            uint32_t alloc = allocations[i];
            uint32_t req = 1;
            max_mu[i] = get_max_mu(i, alloc, balance, req);
            blocks_req[i] = req;
        }

        uint32_t maxa = 0, winner = 0;

        for (uint32_t i = 0; i < NUM_CPU; i++) { //whichever application has the maximum utiity will be declared as the winner. 
            if (maxa < max_mu[i]) {
                maxa = max_mu[i];
                winner = i;
            }
        }

        allocations[winner] += blocks_req[winner]; //the number of blocks allocated to it is increased by the minimum number of blocks. 
        balance -= blocks_req[winner]; //total blocks is reduced by number of blocks allocated to the winner core or application.
        //cout<<"Winner:"<<winner<<" block:"<<blocks_req[winner]<<endl;
    }
    for (uint32_t i = 0; i < NUM_CPU; i++) { //predcited allocations using algortihm is stored in the array containing current allocations
        ucp_arr[i] = allocations[i];
    }

    //the counter for all the ways of all cores are reduced by half
    for (uint32_t i = 0; i < NUM_CPU; i++) {
        for (uint32_t j = 0; j < WAYS; j++) {
            atd[i]->counter[j] /= 2;
        }
    }
}

//memory is allocated to the array which stores the current partioning
UCP::UCP(uint32_t NUM_CPU, uint32_t WAYS, uint32_t TOTAL_SETS)
{
    this->NUM_CPU = NUM_CPU;
    this->WAYS = WAYS;
    ucp_arr = new uint32_t[NUM_CPU];
    for (uint32_t i = 0; i < NUM_CPU; i++) {
        ucp_arr[i] = WAYS / NUM_CPU;
    }
    ucp_arr[0] += WAYS % NUM_CPU;

    atd = new ATD*[NUM_CPU];

    for (uint32_t i = 0; i < NUM_CPU; i++) {
        atd[i] = new ATD(WAYS);
    }
    TOTALSETS = TOTAL_SETS;
}

UCP::~UCP()
{
}

void UCP::umon(uint32_t cpu, uint32_t set, uint64_t addr)
{
    if (set % (TOTALSETS / ATD_SETS) == 0) {
        COUNT++;
        atd[cpu]->access(set / (TOTALSETS / ATD_SETS), addr);
    }
    //we print the current partioning in our cache
    if (COUNT >= COUNT_LIMIT) {
        cout << "CURRENT_PARTIONING\n";
        ucp_algorithm();
        COUNT = 0;
        for(int i=0;i<NUM_CPUS;i++){
            cout << "CORE" << i <<":" << ucp_arr[i] << " " ;
        }
        cout << "\n" ; 
    }

}

//to get number of ways with a given core, we use this function
uint32_t UCP::getways(uint32_t cpu)
{
    return ucp_arr[cpu];
}

//dynamic memory is allocated to atd_blocks and counter using this function 
ATD::ATD(uint32_t WAYS)
{
    atd_blocks = new UCP_BLOCK**[ATD_SETS];
    ways = WAYS;
    for (int i = 0; i < ATD_SETS; i++) {
        atd_blocks[i] = new UCP_BLOCK*[WAYS];
        for (uint32_t j = 0; j < WAYS; j++) {
            atd_blocks[i][j] = new UCP_BLOCK;
        }
    }
    counter = new uint64_t[ways];
    for (uint32_t i = 0; i < ways; i++) {
        counter[i] = 0;
    }
}

uint32_t ATD::lru_victim(uint32_t set, uint64_t addr)
{
    //we are finding the victim for replacement using LRU in ATD.
    uint32_t way;
    for (way = 0; way < ways; way++) {
        if (atd_blocks[set][way]->valid == 0) {
            break;
        }
    }

    // LRU victim
    if (way == ways) {
        for (way = 0; way < ways; way++) {
            if (atd_blocks[set][way]->lru == ways - 1) {
                //this condition find the victim blocks, it will always have its LRU value as ways-1, where ways are the number of ways with the LLC
                break;
            }
        }
    }

    if (way == ways) {
        cerr << "["
             << "ATD ERROR"
             << "] " << __func__ << " no victim! set: " << set << endl;
        assert(0);
    }

    return way;
}

uint32_t ATD::check_hit(uint32_t set, uint32_t addr)
{
    int match_way = -1;

    if (ATD_SETS < set) {
        cerr << "["
             << "ATD"
             << "_ERROR] " << __func__ << " invalid set index: " << set << " NUM_SET: " << ATD_SETS;
        cerr << " address: " << hex << addr << dec;
        cerr << endl;
        assert(0);
    }

    //the hit is checked by iterating through all the blocks in the present set.
    for (uint32_t way = 0; way < ways; way++) {
        if (atd_blocks[set][way]->valid && (atd_blocks[set][way]->address == addr)) {
            match_way = way;
            break;
        }
    }

    return match_way;
}

void ATD::lru_update_hit(uint32_t set, uint32_t way)
{
    // update lru replacement state in case of cache hit

    for (uint32_t i = 0; i < ways; i++) {
        if (atd_blocks[set][i]->lru < atd_blocks[set][way]->lru) {
            atd_blocks[set][i]->lru++;
        }
    }
    atd_blocks[set][way]->lru = 0;  // promote to the MRU position
}

void ATD::lru_update_miss(uint32_t set, uint32_t way)
{
    // update lru replacement state in case of cache miss

    for (uint32_t i = 0; i < ways; i++) {
        if (atd_blocks[set][i]->valid) {
            atd_blocks[set][i]->lru++;
        }
    }
    atd_blocks[set][way]->lru = 0;  // promote to the MRU position
}

void ATD::access(uint32_t set, uint32_t addr)
{
    uint32_t hit = check_hit(set, addr);

    if (hit == -1)  // miss
    {
        uint32_t victim = lru_victim(set, addr);
        //if there is a miss, then we store the incoming packet from the cache in place of victim.
        atd_blocks[set][victim]->address = addr;
        atd_blocks[set][victim]->valid = 1;
        atd_blocks[set][victim]->setindex = set;
        lru_update_miss(set, victim);
    }
    else {  // hit
        counter[atd_blocks[set][hit]->lru]++;
        lru_update_hit(set, hit);
    }
}

UCP_BLOCK::UCP_BLOCK()
{
    setindex = 0;
    address = 0;
    valid = 0;
    lru = 0;
}
