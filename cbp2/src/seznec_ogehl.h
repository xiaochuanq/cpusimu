/* 
   Code has been largely inspired by the tagged PPM predictor simulator from Pierre Michaud, the OGEHL predictor simulator from by André Seznec, the TAGE predictor simulator from André Seznec and Pierre Michaud
*/
#ifndef SEZNEC_OGEHL_H
#define SEZNEC_OGEHL_H

// my_predictor.h
#include <inttypes.h>
#include <math.h>
#include "predictor.h"

#define LOGL 19 //log of the number of entries in the loop predictor
#ifndef LOGB
// base 2 logarithm of number of entries in Meta predictor
#define LOGB 20
// base 2 logarithm of number of entries on each TAGE tagged component
#define LOGG (LOGB)
#endif

#ifndef NHIST
//number of tagged components in TAGE
#define NHIST 19
#endif

#define CBITS 5
//width of the prediction counters in TAGE

#ifndef NHISTGEHL
#define NHISTGEHL 96
#endif

#ifndef LOGGEHL
// base 2 logarithm of number of entries on each GEHL component
#define LOGGEHL 23
#endif

#ifndef MAXHIST
// history lengths on TAGE
#define MINHIST 2
#define MAXHIST 100000
#endif

#ifndef MAXHISTGEHL
// history lengths on GEHL
#define MINHISTGEHL 1
#define MAXHISTGEHL 400
#endif
#define MAXLENGTH 100000

#define BUFFERHIST 128 *1024
// length of history buffer for managing the history

#define MINSTEP 2 //minimum difference between two consecutive history lengths on GEHL
#define TBITS 16 // tag width on the TAGE tables

typedef uint32_t address_t;


// folded_history is a class to manage (accelerate) index computation manipulating hundreds or thousands of bits
// this is the cyclic shift register for folding 
// a long global (branch + path) history into a smaller number of bits
class folded_history
{
 public:
  unsigned comp;
  int CLENGTH;
  int OLENGTH;
  int OUTPOINT;
  int SAVE;
  int shift;

  folded_history ()
    {
    }

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

  void update (uint8_t * h)
  {
    comp = (comp << shift) ^ (h[0]);
    comp ^= (h[OLENGTH] << OUTPOINT);
    comp ^= (comp >> CLENGTH);
    comp &= (1 << CLENGTH) - 1;
  }
  void save ()
  {
    SAVE = comp;
  }
  void restore ()
  {
    comp = SAVE;
  }
};


class lentry //loop predictor entry
{
 public:

  int NbIter;
  int8_t confid;
  int CurrentIter;
  int tag;
  uint8_t age;
  bool Dir;


  lentry ()
    {
      confid = 0;
      CurrentIter = 0;
      NbIter = -1;
      Dir = false;
      age = 0;

    }
};

class mentry // meta table entry
{
 public:
  int8_t choice;
  mentry ()
    {
      choice = -1;
    }
};

class gentry // TAGE global table entry
{
 public:
  int8_t ctr;
  uint16_t tag;
  int8_t u;
  gentry ()
    {
      ctr = 0;
      tag = 0;
      u = 0;
    }
};


static mentry *mtable[4]; //meta predictor
static gentry *gtable[NHIST + 1]; // TAGE tagged tables
static lentry *ltable; // loop predictor table
static int8_t GEHL[1 << LOGGEHL][NHISTGEHL + 1]; //GEHL tables
int mgehl[NHISTGEHL + 1]; //GEHL history lengths
int m[NHIST + 1]; //TAGE history lengths
folded_history chgehl_i[NHISTGEHL + 1]; //utility for computing GEHL indices
folded_history ch_i[NHIST + 1]; //utility for computing TAGE indices
folded_history ch_t[2][NHIST + 1]; //utility for computing TAGE tags

int GEHLINDEX[NHISTGEHL + 1]; // indexes to the different GEHL tables are computed only once
int GI[NHIST + 1]; // indexes to the different TAGE tables are computed only once 


class seznec_ogehl_predictor:public branch_predictor
{
 public:
  branch_update u;
  branch_info bi;

  int USE_ALT_ON_NA; // "Use alternate prediction on newly allocated": a 4-bit counter to determine whether the newly allocated entries should be considered as valid or not for delivering the prediction
  int TICK;
  int LOGTICK; //control counter for the smooth resetting of useful counters
  int phist; // (limited) path history 
  uint8_t *GHIST;
  uint8_t *ghist; // global history
  int ptghist;
  int ptghistsave;

  int phistsave; // to save (limited) path history when jumping to kernel mode
  uint8_t *ghistsave; // to save global history when jumping to kernel mode


  // indexes to the different tables are computed only once 

  int COUNTTHRESHOLD;
  int Theta;
  int SUMGEHL;

  int BI; // index of the meta predictor
  int LI; //index of the loop predictor
  bool LVALID; // validity of the loop predictor prediction
  bool FIRSTOCC; //firt occurence of the branch : static prediction
  bool pred_taken; // prediction
  bool predgehl; //GEHL prediction
  bool tage_pred; // TAGE prediction
  bool alttaken; // alternate prediction
  bool predloop; // loop predictor prediction
  bool predwithoutloop; //TAGE prediction
  int8_t WITHLOOP; // counter to monitor whether or not loop prediction is beneficial
  int8_t WORTHHYBRID; // counter to monitor whether or not using TAGE prediction is beneficial
  int HitBank; // longest matching bank

  int AltBank; // alternate matching bank

  int p0, p1, p2; // meta predictor components predictions
  bool predmeta; // meta predictor prediction

  int Seed, NRAND; // for the pseudo-random number generator 

  bool LASTMODE; // user or kernel mode on the last encountered branch
  bool LASTTAKEN; // direction of the last encountered branch
  int LASTPATHBIT; // LSB of the address of the last encountered branch;
  //André Seznec: For my own stats
  //#define MYOWNSTAT
  //int Nbr,Nbmtage, Nbmwithoutloop, Nbmgehl, Nbmiss;

  seznec_ogehl_predictor (void)
    {
      WITHLOOP = 0;
      COUNTTHRESHOLD = 0;
      Seed = 0;
      Theta = (2 * (NHISTGEHL) + 1);

      USE_ALT_ON_NA = 0;
      LOGTICK = 23; //log of the period for useful tag smooth resetting
      TICK = (1 << (LOGTICK - 1)); //initialize the resetting counter to the half of the threshold 
      GHIST = (uint8_t *) malloc (BUFFERHIST);
      for (int i = 0; i < BUFFERHIST; i++)
	GHIST[i] = 0;

      ghist = GHIST;
      phist = 0;

      for (int i = 0; i < BUFFERHIST; i++)
	ghist[0] = 0;

      ptghist = 0;

      // computes the geometric history lengths for the TAGE predictor 
      m[1] = MINHIST;
      m[NHIST] = MAXHIST;
      for (int i = 2; i <= NHIST; i++)
	{
	  m[i] =
	    (int) (((double) MINHIST *
		    pow ((double) (MAXHIST) / (double) MINHIST,
			 (double) (i - 1) / (double) ((NHIST - 1)))) + 0.5);

	}
      // computes the geometric history lengths for the GEHL predictor 
      m[1] = MINHIST;
      mgehl[0] = 0;
      mgehl[1] = MINHISTGEHL;
      mgehl[NHISTGEHL] = MAXHISTGEHL;
      for (int i = 2; i <= NHISTGEHL; i++)
	{
	  mgehl[i] =
	    (int) (((double) MINHISTGEHL *
		    pow ((double) (MAXHISTGEHL) / (double) MINHISTGEHL,
			 (double) (i - 1) / (double) ((NHISTGEHL - 1)))) + 0.5);

	}
      // just guarantee that all history lengths are distinct
      for (int i = 1; i <= NHISTGEHL; i++)
	if (mgehl[i] <= mgehl[i - 1] + MINSTEP)
	  mgehl[i] = mgehl[i - 1] + MINSTEP;


      //initialisation of index and tag computation functions for TAGE
      for (int i = 1; i <= NHIST; i++)
	{
	  ch_i[i].init (m[i], LOGG, i + 2);
	  ch_t[0][i].init (ch_i[i].OLENGTH, TBITS, i);
	  ch_t[1][i].init (ch_i[i].OLENGTH, TBITS - 1, i + 1);
	}
      //initialisation of index computation functions for GEHL
      for (int i = 1; i <= NHISTGEHL; i++)
	{
	  chgehl_i[i].init (mgehl[i], LOGGEHL, ((i & 1)) ? i : 1);

	}

      //allocation of the loop predictor table
      ltable = new lentry[1 << LOGL];
      //allocation of the TAGE predictor tables

      for (int i = 0; i < 4; i++)
	mtable[i] = new mentry[1 << LOGB];


      for (int i = 1; i <= NHIST; i += 1)
	{
	  gtable[i] = new gentry[1 << LOGG];
	}

      // initialization of GEHL tables
      for (int j = 0; j < (1 << LOGGEHL); j++)
	for (int i = 0; i <= NHISTGEHL; i++)
	  GEHL[j][i] = (i & 1) ? -4: 3;
      //slightly better than -1 and 0 (0.002 misp/KI)
    }

  // index function for the loop predictor
  int lindex (address_t pc)
  {
    return ((pc & ((1 << LOGL) - 1)));
  }
  // index function for the global tables for TAGE: 
  //F serves to mix path history

  int F (int A, int size, int bank)
  {
    int A1, A2;
    A = A & ((1 << size) - 1);
    A1 = (A & ((1 << LOGG) - 1));
    A2 = (A >> LOGG);
    A2 = ((A2 << bank) & ((1 << LOGG) - 1)) + (A2 >> (LOGG - bank));
    A = A1 ^ A2;
    A = ((A << bank) & ((1 << LOGG) - 1)) + (A >> (LOGG - bank));
    return (A);
  }
  int gindex (address_t pc, int bank)
  {
    int index;
    int M = (m[bank] > 16) ? 16 : m[bank];
    index =
      pc ^ (pc >> (abs (LOGG - bank) + 1)) ^
      ch_i[bank].comp ^ F (phist, M, bank);
    return (index & ((1 << LOGG) - 1));
  }

  // index function for the GEHL tables
  //FGEHL serves to mix path history
  int FGEHL (int A, int size, int bank)
  {
    int A1, A2;
    int SH = (bank % LOGGEHL);
    A = A & ((1 << size) - 1);
    A1 = (A & ((1 << LOGGEHL) - 1));
    A2 = (A >> LOGGEHL);
    A2 = ((A2 << SH) & ((1 << LOGGEHL) - 1)) + (A2 >> (LOGGEHL - SH));
    A = A1 ^ A2;
    A = ((A << SH) & ((1 << LOGGEHL) - 1)) + (A >> (LOGGEHL - SH));
    return (A);
  }
  int gehlindex (address_t pc, int bank)
  {
    int index;
    int M = (mgehl[bank] > 16) ? 16 : mgehl[bank];
    index =
      pc ^ (pc >> ((mgehl[bank] % LOGGEHL) + 1)) ^ chgehl_i[bank].
      comp ^ FGEHL (phist, M, bank);
    return (index & ((1 << LOGGEHL) - 1));
  }

  // tag computation for TAGE
  uint16_t gtag (address_t pc, int bank)
  {
    uint16_t tag = pc ^ ch_t[0][bank].comp ^ (ch_t[1][bank].comp << 1);
    return (tag & ((1 << TBITS) - 1));
  }
  // an up-down saturating counter function
  void ctrupdate (int8_t & ctr, bool taken, int nbits)
  {
    if (taken)
      {
	if (ctr < ((1 << (nbits - 1)) - 1))
	  ctr++;
      }
    else if (ctr > -(1 << (nbits - 1)))
      ctr--;
  }
 
  // an up-down saturating counter function + a second increment/decrement when counter is between -8 and 7 
  //improves GEHL accuracy by a marginal 0.01 misp/KI
  void ctrupdatebis (int8_t & ctr, bool taken, int nbits)
  {
    if (abs(2*ctr+1) >=15) { if (taken)
	{
	  if (ctr < ((1 << (nbits - 1)) - 1))
	    ctr++;
	}
      else if (ctr > -(1 << (nbits - 1)))
	ctr--;
    }
    else 
      { if (taken)
	  {

	    ctr+=2;
	  }
	else
	  ctr-=2;
      }
  }
  //loop prediction: only used if high confidence

  void predict_loop (address_t pc)
  {
    //at the end: LI is the index, if LVALID is true then loop prediction can be used, FIRSTOCC is set if this the occurence of the static branch
    LI = lindex (pc);

    if (ltable[LI].tag == 0)
      FIRSTOCC = true;
    else
      FIRSTOCC = false;

    ltable[LI].tag = pc;

    LVALID = (ltable[LI].confid == 7);
    if (ltable[LI].CurrentIter + 1 == ltable[LI].NbIter)
      predloop = (ltable[LI].Dir == 0);
    else
      predloop = (ltable[LI].Dir != 0);
  }

  //computes TAGE prediction
  void predict_tage (address_t pc)
  {
    //Computes the table addresses
    for (int i = 1; i <= NHIST; i++)
      {
	GI[i] = gindex (pc, i);
	GI[i] = (((GI[i] >> 1) ^ ((GI[i] & 1) << 7)) << 1) + (pc & 1);
      }
    HitBank = 0;
    AltBank = 0;

    //if no hit GEHL prediction
    alttaken = predgehl;
    tage_pred = predgehl;
    //Look for the bank with longest matching history
    for (int i = NHIST; i > 0; i--)
      if (gtable[i][GI[i]].tag == gtag (pc, i))
	{
	  HitBank = i;
	  break;
	}
    //Look for the alternate bank
    for (int i = HitBank - 1; i > 0; i--)
      if (gtable[i][GI[i]].tag == gtag (pc, i))
	if ((USE_ALT_ON_NA < 0) || (abs (2 * gtable[i][GI[i]].ctr + 1) != 1))
	  {
	    AltBank = i;
	    break;
	  }

    //computes the TAGE prediction and alternate prediction
    if (HitBank > 0)
      {
	if (AltBank > 0)
	  alttaken = (gtable[AltBank][GI[AltBank]].ctr >= 0);

	//if the entry is recognized as a newly allocated entry and 
	// USE_ALT_ON_NA is negative use the alternate prediction
	if ((USE_ALT_ON_NA < 0)
	    || (abs (2 * gtable[HitBank][GI[HitBank]].ctr + 1) != 1))

	  tage_pred = (gtable[HitBank][GI[HitBank]].ctr >= 0);
	else
	  tage_pred = alttaken;

      }
    //END OF TAGE PREDICTION COMPUTATION

  }

  // compute GEHL prediction
  void predict_gehl (address_t pc)
  {
    //index computation 
    for (int i = 1; i <= NHISTGEHL; i++)
      GEHLINDEX[i] = gehlindex (pc, i);
    GEHLINDEX[0] = (pc & ((1 << LOGGEHL) - 1));

    // SUMGEHL is centered
    SUMGEHL = 0;
    for (int i = 0; i <= NHISTGEHL; i++)
      {
           
	SUMGEHL += 2 * GEHL[GEHLINDEX[i]][i] + 1;
      }
    predgehl = (SUMGEHL >= 0);
  }

  bool predstat (const branch_info & b)
  {
    switch (b.opcode)
      {
      case 0:
      case 1:
      case 9:
      case 12:
      case 13:
      case 15:
	return (true);
      default:
	return (false);
      }

  }

  void predict_meta (address_t pc)
  {
    //an e-gskew-like meta predictor
    BI = (pc & ((1<<LOGG)-1));
       
    p0 = (mtable[0][BI].choice >= 0);
    p1 = (mtable[1][GI[3] ^((GI[3] & 63) << 4)].choice >= 0);
    p2 = (mtable[2][GI[5]].choice >= 0);
    
    predmeta = ((p0+p1+p2) >=2);
    
  }
  void update_meta (bool correct)
  {
    // total update 
    ctrupdate (mtable[0][BI].choice, (correct), 6);
    ctrupdate (mtable[1][GI[3] ^((GI[3] & 63) << 4)].choice, (correct), 6);
    ctrupdate (mtable[2][GI[5]].choice, (correct), 6);
  }


  // PREDICTION
  branch_update *predict (const branch_info & b)
  {
    bi = b;
    if (b.br_flags & BR_CONDITIONAL)
      {
	address_t pc = bi.address;
	predict_gehl (pc);
	predict_tage ((pc << 1) + (uint32_t) predgehl);
	predict_loop (pc);
	predict_meta (pc);
	//Select the prediction

	if (predmeta & (WORTHHYBRID >= 0))
	  predwithoutloop = tage_pred;
	else
	  predwithoutloop = predgehl;
	pred_taken = ((WITHLOOP >= 0)
		      && (LVALID)) ? predloop : predwithoutloop;
	pred_taken = (FIRSTOCC) ? predstat (b) : pred_taken;
      }


    u.direction_prediction (pred_taken);
    u.target_prediction (0);
    return &u;
  }

  void loopupdate (address_t pc, bool Taken)
  {
    if (ltable[LI].NbIter == -1)
      {
	ltable[LI].Dir = Taken ? 1 : 0;
	ltable[LI].NbIter = 0;
	ltable[LI].CurrentIter = 1;
      }
    else
      {
	ltable[LI].CurrentIter++;
	if (ltable[LI].CurrentIter > ltable[LI].NbIter)
	  ltable[LI].confid = 0;
	if (Taken != ltable[LI].Dir)
	  {
	    if (ltable[LI].CurrentIter == ltable[LI].NbIter)
	      {
		if (ltable[LI].confid < 7)
		  ltable[LI].confid++;
		if (ltable[LI].NbIter < 3)
		  ltable[LI].confid = 0;
		//just do not predict when the loop count is 1 or 2 
	      }
	    else
	      {
		ltable[LI].confid = 0;
		ltable[LI].NbIter = ltable[LI].CurrentIter;
	      }
	    ltable[LI].CurrentIter = 0;
	  }
      }
  }

  void gehlupdate (address_t pc, bool taken)
  {

    bool updategehl = (((SUMGEHL >= 0) != taken) || (abs (SUMGEHL) < Theta));
    if (updategehl)
      {
	//Dynamic threshold fitting management
	if ((SUMGEHL >= 0) == taken)
	  {
	    COUNTTHRESHOLD++;
	    if (COUNTTHRESHOLD > 127)
	      {
		Theta -= 2;
		COUNTTHRESHOLD = 0;
	      }
	  }
	else
	  {
	    COUNTTHRESHOLD--;
	    if (COUNTTHRESHOLD < -128)
	      {
		Theta += 2;
		COUNTTHRESHOLD = 0;
	      }
	  }
	//update the GEHL predictor tables
	for (int i = NHISTGEHL; i >= 0; i--) //reverse order to minimize TLB miss count 
	  ctrupdatebis (GEHL[GEHLINDEX[i]][i], taken, 8);
      }
  }


  //TAGE predictor update
  void tageupdate (address_t pc, bool taken)
  {
    // try to allocate a new entries only if TAGE prediction was wrong
    bool ALLOC = ((tage_pred != taken) & (HitBank < NHIST));
    if (HitBank > 0)
      {
	// Manage the selection between longest matching and alternate matching
	// for "pseudo"-newly allocated longest matching entry
	bool LongestMatchPred = (gtable[HitBank][GI[HitBank]].ctr >= 0);
	bool PseudoNewAlloc =
	  (abs (2 * gtable[HitBank][GI[HitBank]].ctr + 1) == 1);
	// an entry is considered as newly allocated if its prediction counter is weak
	if (PseudoNewAlloc)
	  {
	    if (LongestMatchPred == taken)
	      ALLOC = false;
	    // if it was delivering the correct prediction, no need to allocate a new entry
	    //even if the overall prediction was false
	    if (LongestMatchPred != alttaken)
	      {
		// update USE_ALT_ON_NA
		if (alttaken == taken)
		  {
		    if (USE_ALT_ON_NA < 7)
		      USE_ALT_ON_NA++;
		  }
		else if (USE_ALT_ON_NA > -8)
		  USE_ALT_ON_NA--;
	      }
	    if (USE_ALT_ON_NA >= 0)
	      tage_pred = LongestMatchPred;
	  }
      }
    if (ALLOC)
      {
	// is there some "unuseful" entry to allocate
	int8_t min = 1;
	for (int i = NHIST; i > HitBank; i--)
	  if (gtable[i][GI[i]].u < min)
	    min = gtable[i][GI[i]].u;
	// we allocate an entry with a longer history
	//to avoid ping-pong, we do not choose systematically the next entry, but among the 3 next entries
	int Y = NRAND & ((1 << ((NHIST - HitBank - 1))) - 1);
	int X = HitBank + 1;
	if (Y & 1)
	  {
	    X++;
	    if (Y & 2)
	      X++;
	  }
	//NO ENTRY AVAILABLE: JUST ENFORCES ONE TO BECOME AVAILABLE:-)
	if (min > 0)
	  gtable[X][GI[X]].u = 0;
	//Allocate all available entries
	for (int i = X; i <= NHIST; i++)
	  if ((gtable[i][GI[i]].u == min))
	    {
	      gtable[i][GI[i]].tag = gtag (pc, i);
	      gtable[i][GI[i]].ctr = (taken) ? 0 : -1;
	      gtable[i][GI[i]].u = 0;
	    }
      } //END TAGE ENTRIES ALLOCATION

    //periodic reset of u: reset is not complete but bit by bit
    TICK++;
    if ((TICK & ((1 << LOGTICK) - 1)) == 0)
      // reset least significant bit
      // most significant bit becomes least significant bit
      for (int i = 1; i <= NHIST; i++)
	for (int j = 0; j < (1 << LOGG); j++)
	  gtable[i][j].u = gtable[i][j].u >> 1;
    //update TAGE prediction counters
    if (HitBank > 0)
      {
	ctrupdate (gtable[HitBank][GI[HitBank]].ctr, taken, CBITS);
	//if the provider entry is not certified to be useful also update the alternate prediction
	if (gtable[HitBank][GI[HitBank]].u == 0)
	  {
	    if (AltBank > 0)
	      ctrupdate (gtable[AltBank][GI[AltBank]].ctr, taken, CBITS);
	  }
      }

    // update the u counter
    if (tage_pred != alttaken)
      {
	if (tage_pred == taken)
	  {
	    if (gtable[HitBank][GI[HitBank]].u < 3)
	      gtable[HitBank][GI[HitBank]].u++;
	  }
	else
	  {
	    if (USE_ALT_ON_NA < 0)
	      if (gtable[HitBank][GI[HitBank]].u > 0)
		gtable[HitBank][GI[HitBank]].u--;
	  }
      }
    //END OF TAGE UPDATE
  }

  //a " pseudo random number generator" for TAGE update: just a 2-bit counter
  int MYRANDOM ()
  {
    Seed++;
    return (Seed & 3);
  };
  //manage history
  void updateghist (uint8_t * &h, uint8_t dir, uint8_t * tab, int &PT)
  {
    if (PT == 0)
      {
	for (int i = 0; i < MAXLENGTH; i++)
	  tab[BUFFERHIST - MAXLENGTH + i] = tab[i];
	PT = BUFFERHIST - MAXLENGTH;
	h = &tab[PT];
      }
    PT--;
    h--;
    h[0] = dir;
  }
  // PREDICTOR UPDATE
  void update (branch_update * u, bool taken, unsigned int target)
  {
    NRAND = MYRANDOM ();
    address_t pc = bi.address;
    if (bi.br_flags & BR_CONDITIONAL)
      {
#ifdef MYOWNSTAT
	//just to check TAGE and GEHL performance
	Nbr++;
	if ((SUMGEHL >= 0) != taken)
	  Nbmgehl++;
	if (tage_pred != taken)
	  Nbmtage++;
	if (predwithoutloop != taken)
	  Nbmwithoutloop++;
	if (pred_taken != taken)
	  Nbmiss++;
	/* if (!(Nbr & 0x3ffff))
	   fprintf (stderr,
	   " (%d %d %d %d %d)",
	   Nbr >> 20, Nbmgehl, Nbmtage, Nbmwithoutloop, Nbmiss);*/
#endif
	if (LVALID)
	  if (predwithoutloop != predloop)
	    ctrupdate (WITHLOOP, (predloop == taken), 6);
	//Meta predictor update
	if (predgehl != tage_pred)
	  {
	    if (predmeta)
	      {
		ctrupdate (WORTHHYBRID, (tage_pred == taken), 7);
	      }
	    update_meta ((tage_pred == taken));
	  }
	loopupdate (pc, taken);
	tageupdate ((pc << 1) + (uint32_t) predgehl, taken);
	gehlupdate (pc, taken);
      }

    //collect info to update the history and path history 
    uint8_t TAKEN = ((!(bi.br_flags & BR_CONDITIONAL)) | (taken));
    // predict and predict4 uncomment
    TAKEN |= ((bi.address & 127) << 1);
    bool PATHBIT = (bi.address & 1);
    //check user and kernel mode to detect transition
    bool KERNELMODE = ((bi.address & 0xc0000000) == 0xc0000000);
    if ((!KERNELMODE) && (LASTMODE))
      {
	//return to user mode: restore user history
	ptghist = ptghistsave;
	ghist = &GHIST[ptghist];
	phist = phistsave;
	for (int i = 1; i <= NHIST; i++)
	  {
	    ch_i[i].restore ();
	    ch_t[0][i].restore ();
	    ch_t[1][i].restore ();
	  }
	for (int i = 1; i <= NHISTGEHL; i++)
	  {
	    chgehl_i[i].restore ();
	  }
      }
    if ((!LASTMODE) && KERNELMODE)
      {
	//jump in system mode
	//save user mode history

	ptghistsave = ptghist;
	phistsave = phist;
	for (int i = 1; i <= NHIST; i++)
	  {
	    ch_i[i].save ();
	    ch_t[0][i].save ();
	    ch_t[1][i].save ();
	  }
	for (int i = 1; i <= NHISTGEHL; i++)
	  {
	    chgehl_i[i].save ();
	  }
      }

    //update global history and path history
    updateghist (ghist, TAKEN, GHIST, ptghist);
    phist = (phist << 1) + PATHBIT;
    phist = (phist & ((1 << 16) - 1));
    //prepare next index and tag computations
    for (int i = 1; i <= NHIST; i++)
      {
	ch_i[i].update (ghist);
	ch_t[0][i].update (ghist);
	ch_t[1][i].update (ghist);
      }
    for (int i = 1; i <= NHISTGEHL; i++)
      {
	chgehl_i[i].update (ghist);
      }
    LASTMODE = KERNELMODE;
    //memorize the old mode
  }
};

#endif
