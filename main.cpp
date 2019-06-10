#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <math.h>

using namespace std;
bool default_access = true;

class Data {
    public:
        unsigned long Address;
        unsigned long Tag;
        unsigned long Seq_num;
        string Flag;
};

class Cache {
    public:
        unsigned long Blocksize, Cache_Size, Assoc, Sets, Tag, Offset, Index;
        string Replacement_Policy, Inclusion;
        int Index_Width, Tag_Width, Offset_Width;
        unsigned long offset_mask, tag_mask, index_mask;
        unsigned long Reads, Writes, RdMiss, WtMiss, RdHits, WtHits, Writebacks, Mem_Traffic, backInvalidation_WB;
        unsigned long Trace_Num;
        Data **block;
        Cache *L2_Cache;
        unsigned long mask(int);
        bool Hit, Evicted, WB;
        int Way;
        unsigned long EvictedAddress;

    public:
        void blockAccess(unsigned long, char);
        Data *replaceBlock(unsigned long);
};

void print_result(Cache *);
Data* searchRequiredBlockFunction(Cache* L, unsigned long add);


void Cache::blockAccess(unsigned long Address, char Op) {
    this->Way=-1;
    this->Hit=false;
    this->Evicted=false;
    this->WB=false;

    Trace_Num++;

    if (Op == 'r')
        Reads++;
    else
        Writes++;

    Data *Cell = searchRequiredBlockFunction(this,Address);
    if (Cell == NULL)
    {
        this->Hit=false;
        if (Op == 'r')
            RdMiss++;
        else
            WtMiss++;

        Mem_Traffic++;
        if (default_access)
            Cell = replaceBlock(Address);
    } else
    {
        this->Hit=true;
        if (Op == 'r')
            RdHits++;
        else
            WtHits++;


        unsigned long Index = ((Address & this->index_mask) >> (this->Offset_Width));

        if (Replacement_Policy == "LRU")
            Cell->Seq_num=Trace_Num;

        if (Replacement_Policy == "FIFO") {
            if (this->Hit)
                Cell->Seq_num=Cell->Seq_num;
            else if (this->Hit == false)
                Cell->Seq_num=Trace_Num;
        }

    }

    if (Op == 'w')
        Cell->Flag="Dirty";
}
Data *Cache::replaceBlock(unsigned long Address) {
    //unsigned long Tag=((Address & this->tag_mask) >> (this->Index_Width + this->Offset_Width));
    unsigned long Index = ((Address & this->index_mask) >> (this->Offset_Width));
    Data *evictedBlock = NULL;

    for (int i = 0; i < (int) Assoc; i++) {
        if (block[Index][i].Flag == "Invalid") {
            this->Way=i;
            evictedBlock = &(block[Index][i]);
        }
    }
    if (evictedBlock == NULL) {
        unsigned long Way;
        unsigned long temp = Trace_Num;
        if (Replacement_Policy == "LRU" || Replacement_Policy == "FIFO") {
            for (int i = 0; i < (int) Assoc; i++) {
                if (block[Index][i].Seq_num < temp) {
                    temp = block[Index][i].Seq_num;
                    Way = i;
                }
            }
        }
        this->Way=Way;
        evictedBlock = &(block[Index][Way]);
        this->Evicted=true;
        this->EvictedAddress = evictedBlock->Address;
    }

    if (evictedBlock->Flag == "Dirty") {
        this->WB=true;
        Writebacks++;
        Mem_Traffic++;
    }

    evictedBlock->Tag=((Address & this->tag_mask) >> (this->Index_Width + this->Offset_Width));
//    evictedBlock->Tag=Tag;

    evictedBlock->Address=Address;
    evictedBlock->Flag="Valid";
    Index = ((Address & this->index_mask) >> (this->Offset_Width));

    if (Replacement_Policy == "LRU")
        evictedBlock->Seq_num=Trace_Num;

    if (Replacement_Policy == "FIFO") {
        if (this->Hit)
            evictedBlock->Seq_num=evictedBlock->Seq_num;
        else if (this->Hit == false)
            evictedBlock->Seq_num=Trace_Num;
    }


    return evictedBlock;
}


Cache* initL1(Cache* L1,unsigned long Blocksize, unsigned long Cache_Size, unsigned long Assoc, string Replacement_Policy, string Inclusion){
    L1=new Cache();
    L1->Blocksize = Blocksize;
    L1->Cache_Size = Cache_Size;
    L1->Assoc = Assoc;
    L1->Replacement_Policy = Replacement_Policy;
    L1->Inclusion = Inclusion;
    L1->Sets = (Cache_Size) / (Assoc * Blocksize);

    L1->Reads = L1->Writes = L1->RdMiss = L1->WtMiss = L1->RdHits = L1->WtHits = L1->Writebacks = L1->Mem_Traffic = 0;
    L1->Trace_Num = 0;

    L1->Offset_Width = (int) log2(Blocksize);
    L1->Index_Width = (int) log2(L1->Sets);
    L1->Tag_Width = 64 - L1->Offset_Width - L1->Index_Width;

    L1->offset_mask = 0;
    for (int i = 0; i < L1->Offset_Width; i++) {
        L1->offset_mask <<= 1;
        L1->offset_mask |= 1;
    }
    L1->index_mask = 0;
    for (int i = 0; i < (L1->Index_Width + L1->Offset_Width); i++) {
        L1->index_mask <<= 1;
        L1->index_mask |= 1;
    }

    L1->tag_mask = 0;
    for (int i = 0; i < (L1->Tag_Width + L1->Index_Width + L1->Offset_Width); i++) {
        L1->tag_mask <<= 1;
        L1->tag_mask |= 1;
    }

    L1->block = new Data *[L1->Sets];
    for (long long i = 0; i < L1->Sets; i++)
        L1->block[i] = new Data[Assoc];

    L1->L2_Cache = NULL;

    return L1;
}
Cache* initL2(Cache* L2,unsigned long Blocksize, unsigned long Cache_Size, unsigned long Assoc, string Replacement_Policy, string Inclusion){
    L2=new Cache();
    L2->Blocksize = Blocksize;
    L2->Cache_Size = Cache_Size;
    L2->Assoc = Assoc;
    L2->Replacement_Policy = Replacement_Policy;
    L2->Inclusion = Inclusion;
    L2->Sets = (Cache_Size) / (Assoc * Blocksize);

    L2->Reads = L2->Writes = L2->RdMiss = L2->WtMiss = L2->RdHits = L2->WtHits = L2->Writebacks = L2->Mem_Traffic = 0;
    L2->Trace_Num = 0;

    L2->Offset_Width = (int) log2(Blocksize);
    L2->Index_Width = (int) log2(L2->Sets);
    L2->Tag_Width = 64 - L2->Offset_Width - L2->Index_Width;

    L2->offset_mask = 0;
    for (int i = 0; i < L2->Offset_Width; i++) {
        L2->offset_mask <<= 1;
        L2->offset_mask |= 1;
    }
    L2->index_mask = 0;
    for (int i = 0; i < (L2->Index_Width + L2->Offset_Width); i++) {
        L2->index_mask <<= 1;
        L2->index_mask |= 1;
    }

    L2->tag_mask = 0;
    for (int i = 0; i < (L2->Tag_Width + L2->Index_Width + L2->Offset_Width); i++) {
        L2->tag_mask <<= 1;
        L2->tag_mask |= 1;
    }

    L2->block = new Data *[L2->Sets];
    for (long long i = 0; i < L2->Sets; i++)
        L2->block[i] = new Data[Assoc];

    L2->L2_Cache = NULL;

    return L2;
}
Data* searchRequiredBlockFunction(Cache* L, unsigned long add){
    int i=0;
    int limit=(int) L->Assoc;
    while (i< limit){
        if ((L->block[((add & L->index_mask) >> (L->Offset_Width))][i].Tag == ((add & L->tag_mask) >> (L->Index_Width + L->Offset_Width))) && (L->block[((add & L->index_mask) >> (L->Offset_Width))][i].Flag != "Invalid")) {
            L->Way=i;
            return &(L->block[((add & L->index_mask) >> (L->Offset_Width))][i]);
        }
        i++;
    }
    return NULL;
}

bool equals(bool a, bool b){
    return !a^b;
}

bool equals(string a,string b){
    return a==b;
}

void print_l2_contents(Cache* L2){
    cout << "\n ===== L2 contents ===== ";
    for (int i = 0; i < L2->Sets; i++) {
        cout << "\n Set   " << i << ":   ";
        for (int j = 0; j < L2->Assoc; j++) {
            cout << hex << L2->block[i][j].Tag << dec;
            if (L2->block[i][j].Flag == "Dirty")
                cout << " D  ";
            else
                cout << "    ";
        }
    }

}
void print_l1_contents(Cache* L1){
    cout << "===== L1 contents =====";
    for (int i = 0; i < L1->Sets; i++) {
        cout << "\n Set     " << i << ":      ";
        for (int j = 0; j < L1->Assoc; j++) {
            cout << hex << L1->block[i][j].Tag << dec;
            if (L1->block[i][j].Flag == "Dirty")
                cout << " D  ";
            else
                cout << "    ";
        }
    }
}
void print_result(Cache *L1) {
    cout << "\n===== Simulation Results (raw) =====" << endl;
    cout << "a. number of L1 reads:             " << L1->Reads << endl;
    cout << "b. number of L1 read misses:       " << L1->RdMiss << endl;
    cout << "c. number of L1 writes:            " << L1->Writes << endl;
    cout << "d. number of L1 write misses:      " << L1->WtMiss << endl;
    cout << "e. L1 miss rate:                   " << fixed << setprecision(6)
         << (float) (((float) (L1->RdMiss + L1->WtMiss)) / (L1->Reads + L1->Writes)) << endl;
    cout << "f. number of L1 writebacks:        " << L1->Writebacks << endl;

    Cache *L2 = L1->L2_Cache;

    if (L2 == NULL) {
        cout << "g. number of L2 reads:             " << 0 << endl;
        cout << "h. number of L2 read misses:       " << 0 << endl;
        cout << "i. number of L2 writes:            " << 0 << endl;
        cout << "j. number of L2 write misses:      " << 0 << endl;
        cout << "k. L2 miss rate:                   " << 0 << endl;
        cout << "l. number of L2 writebacks:        " << 0 << endl;
        cout << "m. total memory traffic:           " << L1->Mem_Traffic << endl;
    } else {
        cout << "g. number of L2 reads:             " << L2->Reads << endl;
        cout << "h. number of L2 read misses:       " << L2->RdMiss << endl;
        cout << "i. number of L2 writes:            " << L2->Writes << endl;
        cout << "j. number of L2 write misses:      " << L2->WtMiss << endl;
        cout << "k. L2 miss rate:                   " << fixed << setprecision(6)
             << (float) (((float) (L2->RdMiss)) / (L2->Reads)) << endl;
        cout << "l. number of L2 writebacks:        " << L2->Writebacks << endl;
        cout << "m. total memory traffic:           " << L1->backInvalidation_WB + L2->Mem_Traffic << endl;
    }
}
void print_inputs(unsigned long blocksize,unsigned long l1_Size,unsigned long l1_associativity ,unsigned long l2_Size,unsigned long l2_associativity,string replacement_Policy,string inclusion,string tracefile){
    cout << "===== Simulator configuration =====" << endl;
    cout << "BLOCKSIZE:              " << blocksize << endl;
    cout << "L1_Size:                " << l1_Size << endl;
    cout << "L1_Assoc:               " << l1_associativity << endl;
    cout << "L2_Size:                " << l2_Size << endl;
    cout << "L2_assoc:               " << l2_associativity << endl;
    cout << "REPLACEMENT POLICY:     " << replacement_Policy << endl;
    cout << "INCLUSION PROPERTY:     " << inclusion << endl;
    cout << "trace_file:             " << tracefile << endl;
}

int main(int argc, char *argv[]) {
    unsigned long l1_Size;
    unsigned long l1_associativity;
    unsigned long l2_Size;
    unsigned long l2_associativity;
    int r_policy_value;
    int incl_value;
    unsigned long add;
    unsigned long blocksize;
    char op;
    string replacement_Policy;
    string inclusion;
    char *tracefile = (char *) malloc(100);
    blocksize = atol(argv[1]);
    l1_Size = atol(argv[2]);
    l1_associativity = atol(argv[3]);
    l2_Size = atol(argv[4]);
    l2_associativity = atol(argv[5]);
    r_policy_value = atoi(argv[6]);
    incl_value = atoi(argv[7]);
    strcpy(tracefile, argv[8]);

    if (r_policy_value == 0) {
        replacement_Policy = "LRU";
    } else if (r_policy_value == 1) {
        replacement_Policy = "FIFO";
    } else if (r_policy_value == 2) {
        replacement_Policy = "OPTIMAL";
    } else {
        cout << "Invalid input : " << endl;
    }

    if (incl_value == 0) {
        inclusion = "non-inclusive";
    } else if (incl_value == 1) {
        inclusion = "inclusive";
    } else if (incl_value == 2) {
        inclusion = "exclusive";
    } else {
        cout << "Invalid input : " << endl;
    }


    Cache* L1;
    Cache* L2;
    L1=initL1(L1,blocksize, l1_Size, l1_associativity, replacement_Policy, inclusion);
    if (l2_Size != 0) {
        L2 = initL2(L2,blocksize, l2_Size, l2_associativity, replacement_Policy, inclusion);
        L1->L2_Cache=L2;
    }

    ifstream fin;
    fin.open(tracefile);
    while (fin >> op >> hex >> add) {
        L1->blockAccess(add, op);
        if (l2_Size != 0) {
            if (inclusion == "non-inclusive"){
                if (L1->WB)
                    L2->blockAccess(L1->EvictedAddress,'w');
                if (L1->Hit == false)
                    L2->blockAccess(add,'r');
            }

            if (inclusion == "inclusive"){
                if (L1->WB){
                    L2->blockAccess(L1->EvictedAddress,'w');

                    if (L2->Evicted ) {
                        if (searchRequiredBlockFunction(L1, L2->EvictedAddress) != NULL && L1->Inclusion == "inclusive" && L1->L2_Cache && searchRequiredBlockFunction(L1, L2->EvictedAddress)->Flag == "Dirty") {
                            L1->WB = true;
                            L1->backInvalidation_WB++;
                        }
                        if (searchRequiredBlockFunction(L1, L2->EvictedAddress) != NULL)
                            searchRequiredBlockFunction(L1, L2->EvictedAddress)->Flag = "Invalid";

                    }
                }
                if (L1->Hit ==false){
                    L2->blockAccess(add, 'r');
                    if (L2->Evicted){
                        if (searchRequiredBlockFunction(L1, L2->EvictedAddress) != NULL && L1->Inclusion == "inclusive" && L1->L2_Cache && searchRequiredBlockFunction(L1, L2->EvictedAddress)->Flag == "Dirty") {
                            L1->WB = true;
                            L1->backInvalidation_WB++;
                        }
                        if (searchRequiredBlockFunction(L1, L2->EvictedAddress) != NULL)
                            searchRequiredBlockFunction(L1, L2->EvictedAddress)->Flag = "Invalid";
                    }
                }
            }

            if (inclusion == "exclusive")
            {
                if (L1->Hit){
                    if (searchRequiredBlockFunction(L2, add) != NULL && L2->Inclusion == "inclusive" && L2->L2_Cache && searchRequiredBlockFunction(L2, add)->Flag == "Dirty") {
                        L2->WB = true;
                        L2->backInvalidation_WB++;
                    }
                    if (searchRequiredBlockFunction(L2, add) != NULL)
                        searchRequiredBlockFunction(L2, add)->Flag = "Invalid";
                }
                if (L1->Evicted)
                {
                    L2->blockAccess(L1->EvictedAddress,'w');
                    if ( !(L1->WB)){
                        searchRequiredBlockFunction(L2,L1->EvictedAddress)->Flag="Valid";
                    }
                }
                if ( !(L1->Hit )) {
                    default_access = false;
                    L2->blockAccess(add, 'r');

                    default_access = true;
                }
                if (L2->Hit)
                {
                    if(searchRequiredBlockFunction(L2,add)->Flag == "Dirty" ){
                        searchRequiredBlockFunction(L1,add)->Flag="Dirty";
                    }

                    if (searchRequiredBlockFunction(L2, add) != NULL && L2->Inclusion == "inclusive" && L2->L2_Cache && searchRequiredBlockFunction(L2, add)->Flag == "Dirty") {
                        L2->WB = true;
                        L2->backInvalidation_WB++;
                    }
                    if (searchRequiredBlockFunction(L2, add) != NULL)
                        searchRequiredBlockFunction(L2, add)->Flag = "Invalid";

                }
            }
        }
    }

    fin.close();

    print_inputs(blocksize,l1_Size,l1_associativity ,l2_Size,l2_associativity,replacement_Policy,inclusion,tracefile);
    print_l1_contents(L1);
    if (l2_Size >0) {
        print_l2_contents(L2);
    }
    print_result(L1);
    return 1;
}




