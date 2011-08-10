// gehl.h
// This file contains a geometric history length encoding predictor.
#ifndef GEHL_H
#define GEHL_H
#include <vector>
#include <bitset>
#include <string>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <iomanip>
#include <iostream>
#include "predictor.h"
using namespace std;

const int MAX_HISTORY_LENGTH = 400;//as implemented by Seznec
const int max_path_length = 16;
const bool verbose = false;
typedef bitset<MAX_HISTORY_LENGTH> BITVECTOR;

class my_folded_history
{
 public:
  unsigned long comp;
  int CLENGTH;
  int OLENGTH;
  int OUTPOINT;
  int shift;

  my_folded_history(){}

  void init (int original_length, int compressed_length, int sh)
  {
    comp = 0;
    OLENGTH = original_length;
    CLENGTH = compressed_length;
    shift = sh % CLENGTH;
    //we want shift to be prime with CLENGTH
    for (int i = 1; i < CLENGTH; i++)
      {
	if (((shift * i) % CLENGTH) == 0)
	  {
	    shift = 1;
	    break;
	  }
      }
    OUTPOINT = (shift * OLENGTH) % CLENGTH;
  }

  void update (const BITVECTOR& h)
  {
    comp = (comp << shift) ^ (h[0]?1:0);
    comp ^= ( ( h[OLENGTH]?1:0) << OUTPOINT);
    comp ^= (comp >> CLENGTH);
    comp &= (1 << CLENGTH) - 1;
  }
};

class gehl_predictor;

class gehl_update : public branch_update{
  friend class gehl_predictor;
private:
  int sum;
  vector<unsigned int> index;
};

class gehl_predictor : public branch_predictor {
public:
  gehl_predictor (int M = 4, int n = 3, int L1 = 2, float alpha = 1.8,
		  int theta = 4, int nCounterBit = 5, int max_plen=max_path_length)
 : m_n(n),  m_nL1(L1),  m_fAlpha(alpha), m_nThresh(theta),
    m_nCounterBit(nCounterBit), m_nMaxPathLength(max_plen), m_numOps(0)
  /* M: # of tables
     n: 2^n is the number of entris in each prediction Table
     L1: Table 1 index bit lenght
     alpha: the factor of index bit length increment
     theta: threshhold of predictor updating
     nCounterBit: the bit length of the saturation counter
  */
 {
    m_u.direction_prediction(true);
    m_u.index.resize(M, 0);//index to each prediction table
    
    m_Tables.resize(M); //M prediction tables
    for( int i = 0; i < M; ++i)
      m_Tables[i].assign( 1<<m_n, 0);//clear to zeros
    m_L.resize(M);// the index bit-length of each table
    m_L[0] = 0;// T0 is indexed by PC
    m_L[1] = m_nL1;
    for( int i = 2; i < M; ++i ){
      m_L[i] = ceil(m_L[i-1] * alpha);
      //  m_L[i] = int( ai_1 * L1 + 0.5);
      //     cout << "L"<<i<<": "<<m_L[i]<<endl;
    }
    m_ziphist.resize(M);
    for( int i = 1; i < M; ++i )
      m_ziphist[i].init(m_L[i], m_n, ((i & 1)) ? i : 1);

    int nCounterLB = - ( 1<< (nCounterBit - 1) );// counter lower bound
    int nCounterUB = ( 1<<(nCounterBit - 1) ) - 1;// and upper bound
    m_vecCounterLB.assign( 1<<m_n, nCounterLB);
    m_vecCounterUB.assign( 1<<m_n, nCounterUB);

    m_nSizeInBits = m_nCounterBit * (1<<m_n) * m_Tables.size();
    // above is the bit size of prediction tables
    m_nSizeInBits += m_nMaxPathLength + m_L.back();// add phist and ghist length.
 }

  void printConfig(){
    cout <<"theta\talpha\tL1\tTables\tIndex\tcnterbit\t size"<<endl;
    cout << m_nThresh<<"\t"<<m_fAlpha<<"\t"<<m_nL1<<"\t"<< m_Tables.size();
    cout << "\t" << m_n << "\t" << m_nCounterBit<< "\t";
    cout << m_nSizeInBits << endl;
      /*
      cout <<"Number of Tables: "<< m_Tables.size()<<endl;
      cout <<"Table bits:       "<< m_n;
      cout <<"L1 size:          "<< m_nL1 << endl;
      cout <<"Alpha:            "<< m_fAlpha << endl;
      cout <<"Threshhold theta: "<< m_nThresh<<endl;
      cout <<"Counter Bit:      "<< m_nCounterBit<<endl;
      */
  }

  void printTrace(bool taken){
    if(_debug){
      m_numOps++;
      cout << setw(8) << setfill(' ') << dec << m_numOps<<" ";
      cout << hex << m_bi.address <<" "<< (m_bi.br_flags?'B':'N')<<" ";
      cout << (m_bi.br_flags ? (m_bi.br_flags==BR_CONDITIONAL? 'C': 'U') : '-') <<" ";
      cout <<(  m_bi.br_flags == BR_CONDITIONAL ? (m_u.direction_prediction()?'T':'N') : (m_bi.br_flags? 'T' : '-'))<<" ";
      cout << ( m_bi.br_flags & BR_CONDITIONAL ?(taken? 'T' : 'N' ) : (m_bi.br_flags?'T':'-'))<<" ";

      //BITVECTOR phist = m_phist & BITVECTOR( (1<<min(m_nMaxPathLength, m_L.back()) ) - 1);
      //BITVECTOR ghist = m_ghist & BITVECTOR( string( 64, '1'));
      //cout << setw(16)<< setfill('0') << hex << phist.to_ulong()<<" ";
      //cout << setw(24)<< setfill('0') << hex << ghist.to_ulong()<<" ";
      string ps = m_phist.to_string();
      string gs = m_ghist.to_string();
      cout <<  ps.substr(ps.size() - min(m_nMaxPathLength, m_L.back()))<<" ";
      cout <<  gs.substr(gs.size() - m_L.back());
      cout << " " <<  dec << m_u.sum << endl;
    }
  }

  ~gehl_predictor(){ }

  void printTables()
  {
    if(_debug && verbose){
    for( int i = 0; i < m_Tables.size(); ++i){
      cout<<"T"<<dec<<i;
      for( int j = 0; j < m_Tables[i].size(); ++j){
	if( j%5 == 0 ) cout <<"| ";
	cout<< dec <<showpos << m_Tables[i][j]<<" ";
      }
      int idx = m_u.index[i];
      cout << "| index = "<<noshowpos<< idx <<" value = "<<m_Tables[i][idx]<<endl;
    }
  }
  }

  branch_update *predict (const branch_info & bi) {
    m_bi = bi;
    if( bi.br_flags & BR_CONDITIONAL ){
      if(_debug && verbose)
	  cout <<"On CB: "<<hex<<bi.address<<dec<< ">>>>>>>>>>>>>>>>>>"<<endl;
      //--------------------------------------------------------------
      m_u.sum = m_Tables.size()/2;
      for(unsigned int i = 0; i < m_Tables.size(); ++i){
      // accumulate indexed counters from each table T
	unsigned int idx = gehl_index(i, bi.address);
	// use address to index Ti
	m_u.index[i] = idx;
	m_u.sum += m_Tables[i][idx];
      }
      // assert( m_u.sum );// in the paper, there are only two cases: +/-
      m_u.direction_prediction( m_u.sum >= 0);
      // if sum >= 0 then taken (true); else not taken (false)
      //----------------------------------------------------------------
        if(_debug && verbose)
		printTables();
    }
    else{
      m_u.direction_prediction(true);
    }
    m_u.target_prediction(0);
    return &m_u;
  }

  unsigned int fetch_pathbits(int size, int ti)
  {
    int SH = (ti % m_n);
    int A = ( m_phist & BITVECTOR((1 << size) - 1) ).to_ulong();
    int A1 = (A & ((1 << m_n) - 1));
    int A2 = (A >> m_n);
    A2 = ((A2 << SH) & ((1 << m_n) - 1)) + (A2 >> (m_n - SH));
    A = A1 ^ A2;
    A = ((A << SH) & ((1 << m_n) - 1)) + (A >> (m_n - SH));
    return (A);
  }
    
  virtual unsigned int gehl_index( unsigned int ti, unsigned int address){
    if( !ti )
      return address & ( (1 << m_n) - 1);
    int maxlen = min( m_nMaxPathLength, m_L[ti]);
    unsigned int index = (address& ( (1 << m_n) - 1));
 /*^( address >>(( m_L[ti] % m_n)+1))*/ 
    index ^= (m_ziphist[ti].comp&( (1<<min(m_n,m_L[ti]))-1));
    index ^= (m_phist & BITVECTOR( (1<<maxlen)-1)).to_ulong() ;
                       // fetch_pathbits(maxlen, ti);
if( _debug && verbose){
   int effective_len = min( m_L[ti], m_n); // at most m_n bits from ghist
    int path_len = min( m_L[ti], m_nMaxPathLength);
   
    BITVECTOR g = BITVECTOR(m_ziphist[ti].comp) & BITVECTOR( (1<< effective_len) - 1);
      BITVECTOR p = m_phist & BITVECTOR( (1<< path_len) - 1);
      BITVECTOR a( address & ((1 << m_n) - 1));
      BITVECTOR i = (g ^ p ^ a)&BITVECTOR((1 << m_n) - 1);
      string sg = g.to_string();
      string sp = p.to_string();
      string sa = a.to_string();
      string si = i.to_string();
      cout <<"T"<<ti<<"\tglist:" << sg.substr(sg.size() - m_n) <<" ";
      cout << setw(m_n) << hex << g.to_ulong() << endl;
      cout <<"\tplist:" << sp.substr(sp.size() - m_n) <<" ";
      cout << setw(m_n) << hex << g.to_ulong() << endl;
      cout <<"\t   pc:" << sa.substr(sa.size() - m_n)<< " ";
      cout << setw(m_n) << hex << a.to_ulong() <<endl;
      cout <<"\t3xor :" << si.substr(si.size() - m_n)<< " ";
      cout << setw(m_n) << dec << i.to_ulong() <<endl;
    }
    return (index & (( 1 << m_n) - 1));
  }

  virtual unsigned int gehl_index_old(unsigned int ti, unsigned int address){
    if( ti == 0) // T0 is indexed using brach address ( PC )
      return address & ( (1<<m_n) - 1);
    int effective_len = min( m_L[ti], m_n); // at most m_n bits from ghist
    int path_len = min( m_L[ti], m_nMaxPathLength);
    //at most max_path_len bits from phist
    BITVECTOR Index;
    Index =  m_ghist & BITVECTOR( (1<< effective_len) - 1);
    Index ^= m_phist & BITVECTOR( (1<< path_len) - 1);
    unsigned int idx = Index.to_ulong();
    //return ( address ^ ( address >> ( effective_len + 1 ))  ^ idx ) & ( (1<<m_n) - 1);
    //------------------------------------------------------
    if( _debug){
      /*      BITVECTOR g = m_ghist & BITVECTOR( (1<< effective_len) - 1);
      BITVECTOR p = m_phist & BITVECTOR( (1<< path_len) - 1);
      BITVECTOR a( address & ((1 << m_n) - 1));
      BITVECTOR i = (g ^ p ^ a)&BITVECTOR((1 << m_n) - 1);
      string sg = g.to_string();
      string sp = p.to_string();
      string sa = a.to_string();
      string si = i.to_string();
      cout <<"T"<<ti<<"\tglist:" << sg.substr(sg.size() - m_n) <<" ";
      cout << setw(m_n) << hex << g.to_ulong() << endl;
      cout <<"\tplist:" << sp.substr(sp.size() - m_n) <<" ";
      cout << setw(m_n) << hex << g.to_ulong() << endl;
      cout <<"\t   pc:" << sa.substr(sa.size() - m_n)<< " ";
      cout << setw(m_n) << hex << a.to_ulong() <<endl;
      cout <<"\t3xor :" << si.substr(si.size() - m_n)<< " ";
      cout << setw(m_n) << dec << i.to_ulong() <<endl;*/
    }
    //------------------------------------------------------
    return ( address ^ idx ) & ( (1<<m_n) - 1);
  }

  void update (branch_update *pu, bool taken, unsigned int target) {
    
       printTrace(taken);
    //------------------------------------------------------------
    gehl_update* p = static_cast<gehl_update*>(pu);
    if( m_bi.br_flags & BR_CONDITIONAL){
      if( (p->direction_prediction() != taken) ||( abs( p->sum ) <= m_nThresh) ){
	// update only on misspredictions or S is not significant
	assert( p->index.size() == m_Tables.size() );
	for(unsigned int i = 0; i < p->index.size(); ++i){ //u.index.size() == M, the # of tables
	  if( taken )
	    m_Tables[i][ p->index[i] ] = min( m_vecCounterUB[i], m_Tables[i][ p->index[i] ] + 1 );
	  else
	    m_Tables[i][ p->index[i] ] = max( m_vecCounterLB[i], m_Tables[i][ p->index[i] ] - 1 );
	}
      }
      if(_debug ){
	printTables();
	if(verbose)
	  cout << "Updated! <<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;
      }
    }
    if( m_bi.br_flags){//if flag is zero, it means BR_NOTABRANCH
      m_ghist <<= 1;
      m_ghist.set(0, taken);
      m_phist <<= 1;
      m_phist.set(0, m_bi.address & 1);//TODO: do we have to truncate m_phist here immediately or postpone to predict?
      for(int i = 1; i < p->index.size(); ++i){
	m_ziphist[i].update(m_ghist);
	if(_debug && verbose){
	  BITVECTOR cg(m_ziphist[i].comp);
	string scg = cg.to_string();
	cout <<"compressed hist "<<i<<": "<< scg.substr(scg.size() - sizeof(unsigned int))<<"="<<m_ziphist[i].comp;
	cout <<"["<<m_ziphist[i].shift<<","<<m_ziphist[i].OUTPOINT<<",";
	cout <<m_ziphist[i].OLENGTH<<","<<m_ziphist[i].CLENGTH<<"]" << endl;
      }
      }
    }
  }

  int size(){ return m_nSizeInBits; }

 protected: // for potential inheritence in ogehl
  branch_info m_bi;
  gehl_update m_u;

  int m_n; // the number n in the paper. 2^n = # of entries in each Table
  vector< vector<int> > m_Tables;
  vector<int> m_L;

  int m_nL1;
  float m_fAlpha;
  int m_nCounterBit;
  int m_nMaxPathLength;
  int m_nThresh;
  // In GEHL, the same counter width is used for all counters.
  // In O-GEHL, counters bit width may vary over counters.
  vector<int> m_vecCounterBit;// satuation bits of all counters
  vector<int> m_vecCounterLB; // lower bound of a counter - 1 << (m_nCounterbit-1)
  vector<int> m_vecCounterUB; // upper bound of a counter 1 << (m_nCounterBit-1) - 1
  
  vector<my_folded_history> m_ziphist;

  BITVECTOR m_ghist; // global history vector
  BITVECTOR m_phist; // path history vector
  int64_t m_numOps;
  int m_nSizeInBits;
};

#endif
