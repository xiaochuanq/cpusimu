// ogehl.h
// This file contains an optimized geometric history length encoding predictor.
#ifndef OGEHL_H
#define OGEHL_H
#include <iostream>
#include <sstream>
#include <math.h>
#include <stdint.h>
#include <bitset>
#include <iomanip>

#include "predictor.h"
#include "SaturatingCounter.hpp"
#include "instruction.h"

typedef bitset<4096> bitset_t;

/* Return a string containing a base 2 representation of number with a fixed
   length */
std::string binaryString(int64_t number, int32_t length) {
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    int32_t mask = 1 << (length-1);
    for (int32_t shift = length-1; shift >= 0; shift--) {
        ss << (((number & mask) >> shift) & 1);
        mask = mask >> 1;
    }

    return ss.str();
}

//================================== Table ==================================//
class Table {
public:
    Table(int64_t idx_bits, uint64_t short_hist_bits, uint64_t long_hist_bits,
          int64_t ctr_bits, int32_t table_ind = 0) :
        _idx_bits(idx_bits), _short_hist_bits(short_hist_bits),
        _long_hist_bits(long_hist_bits), _pc_bits(idx_bits), _use_long_hist(false) {
        if (idx_bits >= 26) {
            std::cout << "Cannot have a predictor table with more than 26 index bits";
            std::cout << ".  You selected " << idx_bits << endl;
            assert(false);
        }

        // Assume we take as many PC bits as there are index bits
        int64_t tbl_size = 1 << idx_bits;
        _table = new std::vector<SaturatingCounter *>(tbl_size);
        for (int64_t i = 0; i < tbl_size; i++) {
            _table->at(i) = new SaturatingCounter(ctr_bits);
        }

        // Setup masks
        for (int32_t i = 0; i < _short_hist_bits; i++) {
            _short_hist_mask[i] = 1;
        }
        for (int32_t i = 0; i < _long_hist_bits; i++) {
            _long_hist_mask[i] = 1;
        }
        for (int32_t i = 0; i < _idx_bits; i++) {
            _pc_mask[i] = 1;
        }

        // Get parameters for index function
        _cpath_hist = 0;
        _cbranch_hist = 0;

	//  _shift = table_ind % _idx_bits;
	_shift=( (table_ind & 1)? table_ind : 1)%_idx_bits;
        for (int i = 1; i < _idx_bits; i++) {
            if ((_shift * i) % _idx_bits == 0) {
                _shift = 1;
                break;
            }
        }

        _last_dest = (_shift * short_hist_bits) % _idx_bits;
        // std::cout << binaryString(_short_hist_mask, short_hist_bits) << " ";
        // std::cout << binaryString(_long_hist_mask, long_hist_bits) << endl;
        // if (table_ind != 0) {
        //     std::cout << "shift="<< _shift << endl;
        //     std::cout << "last_dest=" << _last_dest << endl;
        // }
        _debug = true;
    }

    int32_t numElements() const {
        return _table->size();
    }

    int64_t bitSize() const {
	// number of saturating counters * size of saturating counters +
	// number of bits to store compressed history
	bool use_hist = (_long_hist_mask[0] == 0) ? 0 : 1;
        return numElements() * _table->at(0)->size() + (_idx_bits * use_hist);
    }

    int64_t predict(int64_t pc, bitset_t branch_hist, bitset_t path_hist) {
        if (_debug) {
            std::stringstream ss(std::stringstream::in | std::stringstream::out);
            ss << std::hex;
            //ss << debugString() << endl;
            //ss << "PC:         " << binaryString(pc, 32) << endl;
            //ss << "BranchHist: " << branch_hist.to_string() << endl;
            //ss << "PathHist:   " << path_hist.to_string() << endl;
            //cout << ss.str();
        }

        int64_t idx = getComplicatedIndex(pc, branch_hist, path_hist);

        return _table->at(idx)->val();
    }
  void compressHistory(bitset_t& path_hist, bitset_t& branch_hist){
    //_cpath_hist = compress(path_hist, _cpath_hist);
    _cbranch_hist = compress(branch_hist, _cbranch_hist);
    /* if( _debug ){
    bitset_t bs(_cbranch_hist);
	std::string s = bs.to_string();
	cout << s.substr( s.size() - histBits());
	cout << "="<<_cbranch_hist;
	cout <<"["<<_shift<<","<<_last_dest<<","<<histBits()<<","<<_idx_bits<<"]";
	}*/
  }

    void update(int64_t pc, bitset_t branch_hist, bitset_t path_hist, bool taken) {
        int64_t idx = getComplicatedIndex(pc, branch_hist, path_hist);
        if (taken) {
            _table->at(idx)->inc();
        } else {
            _table->at(idx)->dec();
        }
    }

    void debug(bool d) {
        _debug = d;
    }

    int64_t histBits() const {
        return (_use_long_hist) ? _long_hist_bits : _short_hist_bits;
    }

    bitset_t histMask() const {
        return (_use_long_hist) ? _long_hist_mask : _short_hist_mask;
    }

    std::string debugString() const {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << std::hex;
        ss << "Table (" << _idx_bits << " index bits,";
        ss << std::dec;
        ss << "shrth=" << setw(3) << _short_hist_bits;
        ss << ", longh=";
        ss.width(2);
        ss << _long_hist_bits;
        ss << ", long?=" << _use_long_hist << ")";
        ss << std::dec;
        for (unsigned int i = 0; i < _table->size(); i++) {
            ss << " ";
            ss.width(2);
            ss << _table->at(i)->val();
        }
        return ss.str();
    }

    bool useLong(bool o) {
        _use_long_hist = o;
        return o;
    }

    bool useLong() const {
        return _use_long_hist;
    }

    int64_t longHist() const {
        return _long_hist_bits;
    }

    int64_t shortHist() const {
        return _short_hist_bits;
    }

    /* Hash global history with previously compressed history */
    int64_t compress(bitset_t global_hist, int64_t c) const {
        if (histBits() == 0) {
           return 0;
        }

        c = (c << _shift) ^ global_hist[0];

	c ^= (global_hist[histBits()]) << _last_dest;

        c ^= (c >> _idx_bits);

        c &= (1 << _idx_bits) - 1;

        return c;
    }

    int64_t getComplicatedIndex(int64_t pc, bitset_t branch_hist, bitset_t path_hist) const {
      //  bitset_t hist(compress(path_hist, _cpath_hist));
        bitset_t cbranch(_cbranch_hist);
	//    bitset_t branch(compress(branch_hist, _cbranch_hist));

        bitset_t path_contrib = path_hist & histMask();
        bitset_t hist_contrib = cbranch & histMask();
        bitset_t pc_contrib = bitset_t(pc) & _pc_mask;

        uint64_t idx = (path_contrib ^ hist_contrib ^ pc_contrib).to_ulong();
        idx &= (1 << _idx_bits) - 1;

        if (_debug) {
            std::stringstream ss(std::stringstream::in | std::stringstream::out);
            ss << std::hex;
            std::string phs = path_contrib.to_string();
            phs = phs.substr(phs.size()-histBits(), phs.size());

            std::string bhs = hist_contrib.to_string();
            bhs = bhs.substr(bhs.size()-histBits(), bhs.size());


            std::string pcs = pc_contrib.to_string();
            pcs = pcs.substr(pcs.size()-_idx_bits, pcs.size());
            ss << "Hist contrib:  ";
            ss.width(_idx_bits);
            ss << bhs;
            ss.width(_idx_bits);
            ss << endl << "Path Contrib:  ";
            ss.width(_idx_bits);
            ss << phs << endl << "PC   Contrib:  ";
            ss.width(_idx_bits);
            ss << pcs << endl;
            ss << std::dec;
            ss << "Index: " << idx << endl;
            ss << "Value: " << _table->at(idx)->val() << endl << endl;
            cout << ss.str();
        }
        return idx;
    }

    int64_t getIndex(int64_t pc, bitset_t branch_hist, bitset_t path_hist) const {
        bitset_t history_contrib = branch_hist & histMask();
        bitset_t pc_contrib = bitset_t(pc) & _pc_mask;
        bitset_t path_contrib = path_hist & histMask();

        uint64_t idx = (history_contrib ^ pc_contrib ^ path_contrib).to_ulong();
        idx &= (1 << _idx_bits) - 1;

        if (_debug) {
            std::stringstream ss(std::stringstream::in | std::stringstream::out);
            ss << std::hex;
            std::string phs = path_contrib.to_string();
            phs = phs.substr(phs.size()-histBits(), phs.size());

            std::string bhs = history_contrib.to_string();
            bhs = bhs.substr(bhs.size()-histBits(), bhs.size());


            std::string pcs = pc_contrib.to_string();
            pcs = pcs.substr(pcs.size()-_idx_bits, pcs.size());
            ss << "Hist contrib:  ";
            ss.width(_idx_bits);
            ss << bhs;
            ss.width(_idx_bits);
            ss << endl << "Path Contrib:  ";
            ss.width(_idx_bits);
            ss << phs << endl << "PC   Contrib:  ";
            ss.width(_idx_bits);
            ss << pcs << endl;
            ss << std::dec;
            ss << "Index: " << idx << endl;
            ss << "Value: " << _table->at(idx)->val() << endl << endl;
            cout << ss.str();
        }

        if (idx >= _table->size()) {
            cout << "PROBLEM" << endl;
            cout << std::hex << "pc=" << pc << " branch_hist=" << branch_hist << "path_hist=" << path_hist << std::dec << endl;
            cout << " pc_mask=       " << _pc_mask.to_string() << endl;
            cout << " use_long= " << _use_long_hist << endl;
            cout << "short_br_mask= " << _short_hist_mask.to_string() << endl;
            cout << "long_br_mask=  " << _long_hist_mask.to_string() << endl;
            cout << "path_mask=     " << histMask().to_string() << endl;
            cout << "index= " << idx;
            assert(false);
        }
        return idx;
    }
private:
    int64_t _idx_bits, _short_hist_bits, _long_hist_bits, _pc_bits;
    bitset_t _short_hist_mask, _long_hist_mask, _pc_mask;
    std::vector<SaturatingCounter *> *_table;
    bool _debug;

    bool _use_long_hist;

    int64_t _cpath_hist, _cbranch_hist;
    int32_t _shift, _last_dest;
};

//================================= Lengths =================================//
class Lengths {
public:
    Lengths(int64_t l1_sz, double alpha, int64_t num_lengths) {
        assert(alpha > 1);
        assert(l1_sz > 1);
        assert(num_lengths > 1);

        // calculate L(I)
        _l = new std::vector<int64_t>(num_lengths);
        _l->at(0) = 0;
        _l->at(1) = l1_sz;

        for (unsigned int i = 2; i < _l->size(); i++) {
            _l->at(i) = ceil(_l->at(i-1) * alpha);
        }
    }

    std::string debugString() {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);

        ss << "  i  = ";
        for (unsigned int i = 0; i < _l->size(); i++) {
            ss.width(5);
            ss << i;
        }
        ss << std::endl;
        ss << "L(i) = ";

        for (unsigned int i = 0; i < _l->size(); i++) {
            ss.width(5);
            ss << _l->at(i);
        }
        ss << endl;

        return ss.str();
    }

    int64_t at(uint64_t i) const {
        assert(i >= 0 && i < _l->size());
        return _l->at(i);
    }

    int64_t size() const {
        return _l->size();
    }

private:
    std::vector<int64_t> *_l;
};

//=============================== Table maker ===============================//
std::vector<Table *> * makeTables(const Lengths &lengths,
                                  int32_t idx_bits,
                                  int32_t num_tables,
                                  int32_t addtl_hist_length_vals,
                                  int32_t ctr_bits) {
    assert(addtl_hist_length_vals <= num_tables-2);

    std::vector<Table *> *tables = new std::vector<Table *>(num_tables);

    tables->at(0) = new Table(idx_bits, lengths.at(0), lengths.at(0), ctr_bits);

    // std::cout << "Short Length Idx/Long Length Idx: ";
    // fprintf(stdout, "%3d/%3d\n", lengths.at(0), lengths.at(0));

    int32_t long_counter = 0;
    int32_t jump_size = (addtl_hist_length_vals > 0) ? (num_tables-2) / addtl_hist_length_vals : 0;
    for (int32_t i = 1; i < num_tables; i++) {
        int32_t long_length_idx = i;
        if (addtl_hist_length_vals > 0) {
            assert(num_tables >= 3);
            if (addtl_hist_length_vals == 1) {
                /* if you are the center element, set long length accordingly*/
                int32_t index = (int32_t) num_tables / 2;
                if (i == index) {
                    long_counter++;
                    long_length_idx = long_counter + num_tables - 1;
                }
            } else if (i != num_tables-1) { /* exclude largest table */
                if (((i - jump_size) % jump_size) == 0) {
                    long_counter++;
                    long_length_idx = long_counter + num_tables - 1;
                }
            }
        }

        // std::cout << "Short Length Idx/Long Length Idx: ";
        // fprintf(stdout, "%3d/%3d\n", i, long_length_idx);
     
        int32_t short_length = lengths.at(i);
        int32_t long_length = lengths.at(long_length_idx);
        tables->at(i) = new Table(idx_bits, short_length, long_length, ctr_bits, i);
    }

    return tables;
}


//================================ Predictor ================================//
class ogehl_predictor : public branch_predictor {
public:
    branch_update u;

    ogehl_predictor(int32_t theta, std::vector<Table *> *tables, int32_t idx_bits,
                    int32_t thresh_bits, int32_t a_ctr_bits,
                    bool dynamic_theta, bool dynamic_hist_length,
                    bool verbose=false) :
        _theta(theta), _idx_bits(idx_bits),
        _nu_miss(0), _nu_correct(0), _dynamic_theta(dynamic_theta),
        _dynamic_hist_length(dynamic_hist_length),
        _threshold_counter(thresh_bits),_alias_counter(a_ctr_bits),
        _verbose(verbose), _numOps(0) {
        u.direction_prediction(true);

        _tables = tables;
	if (_tables->back()->histBits() > _path_mask_bs.size()) {
	  std::cerr << _tables->back()->histBits();
	  std::cerr << " is too many history bits.  The most the code supports is ";
	  std::cerr << _path_mask_bs.size() << ".  Change bitset_t" << endl;
	  assert(false);
	}
        for (int32_t i = 0; i < _tables->back()->histBits(); i++) {
            _branch_mask_bs[i] = 1;
        }
	int64_t phist = (_tables->back()->histBits() > 16) ? 16 : _tables->back()->histBits();
        for (int32_t i = 0; i < phist; i++) {
            _path_mask_bs[i] = 1;
        }

        _branch_hist_bs = 0;
    }
  ~ogehl_predictor(){
    for( vector<Table*>::iterator tpit = _tables->begin(); tpit != _tables->end(); ++tpit)
      delete (*tpit);
    delete _tables;
  }
    /* Must pass instruction to predict */
    branch_update *predict (const branch_info &) {assert(false);};
    branch_update *predict (const branch_info & b, const Instruction &insn) {
        if (_verbose && (insn.IsCondBranch() || insn.IsUCondBranch())) {
            cout << "Predicting PC: " << std::hex << insn.PC() << std::dec;
            cout << "(low bit=" << (insn.PC() & 1) << ") Type: ";
            if (insn.IsCondBranch()) {
                cout << "C";
            } else if (insn.IsUCondBranch()) {
                cout << "U";
            } else {
                cout << "-";
            }
            cout << endl;
            if (insn.IsCondBranch()) {
                cout << debugString() << endl;
            }
        }
        if (insn.IsCondBranch()) {
            _S = _sumCounters(insn);
            u.direction_prediction((_S >= 0) ? true : false);
        }

	if (insn.IsUCondBranch()) {
	    u.direction_prediction(true);
	}
        return &u;
    }


    /* Must pass instruction to update */
    void update(branch_update *u, bool taken, unsigned int target) {
        assert(false);
    }

    void update(branch_update *u, bool taken, unsigned int target, const Instruction & ins) {
        _numOps++;

        if (_debug) {
            char branch = (ins.TNnotBranch() != '-') ? 'B' : 'N';
            char condUcond = '-';
            char pred = '-';
            char actual = '-';
            if (branch == 'B') {
                condUcond = ins.IsUCondBranch() ? 'U' : 'C';
                pred = u->direction_prediction() ? 'T' : 'N';
                actual = ins.TNnotBranch();;
            }

            cout << setw(8) << setfill(' ') << std::dec << _numOps;
            cout << " " << std::hex << ins.PC() << std::dec;
            cout << " " << branch;
            cout << " " << condUcond;
            cout << " " << pred;
            cout << " " << actual;
            cout << " " << _pathHistString();
            cout << " " << _branchHistString();
            cout << " " << _S << endl;
        }
        if (ins.IsCondBranch()) {
            bool incorrect_predict = (u->direction_prediction() != taken);
            bool low_threshold= abs(_S) <= _theta;
            bool update_correct = (!incorrect_predict && low_threshold);

            if (incorrect_predict || update_correct) {
                if (incorrect_predict) {
                    _nu_miss += 1;
                } else {
                    _nu_correct += 1;
                }

                for (unsigned int i = 0; i < _tables->size(); i++) {
                    _tables->at(i)->update(ins.PC(), _branch_hist_bs, _path_hist_bs, taken);
                }

                int32_t index = _tables->back()->getComplicatedIndex(ins.PC(), _branch_hist_bs, _path_hist_bs);
                _dynamicHistoryFit(ins, incorrect_predict, low_threshold, index);
            }

            // Update branch & path history
            bitset_t t_bs(taken ? 1 : 0);

	    // _branch_hist_bs = ((_branch_hist_bs << 1) | t_bs) & _branch_mask_bs;
	    _branch_hist_bs = (_branch_hist_bs << 1) | t_bs;
	 
            if (0 == _branch_mask_bs[0]) {
                assert(false);
            }

            /* Maybe update on an unconditional branch as well */
            _dynamicThresholdFit(incorrect_predict, update_correct);

        } else if (ins.IsUCondBranch()){
	  //   _branch_hist_bs = ((_branch_hist_bs << 1) | bitset_t(1)) & _branch_mask_bs;
	  _branch_hist_bs = (_branch_hist_bs << 1) | bitset_t(1);
        }

        if (ins.IsCondBranch() || ins.IsUCondBranch()) {
            _path_hist_bs = ((_path_hist_bs << 1) | (bitset_t(ins.PC()) & bitset_t(1))) & _path_mask_bs;
	    for( unsigned int i = 1; i < _tables->size(); ++i){
	      if(_debug && _verbose)
	        cout <<endl<<"Compressed History "<< i <<":";
	      _tables->at(i)->compressHistory(_path_hist_bs, _branch_hist_bs);
	      
	    }

        }
        if (_verbose && (ins.IsCondBranch() || ins.IsUCondBranch())) {
            cout << debugString() << endl;
            cout << "//=======================================================//";
            cout << endl;
        }
    }

    std::string debugString() const {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << std::hex;
        ss << "OGEHL Predictor" << endl;
        ss << "theta=" << _theta << " IdxBits=" << _idx_bits;
        ss << " DynTheta=" << _dynamic_theta << " DynHL=" << _dynamic_hist_length;
        ss << std::dec;
        ss << " AC=" << _alias_counter.val();
        ss << " TC=" << _threshold_counter.val() << endl;
        ss << "NU Miss=" << _nu_miss << " NU Correct=" << _nu_correct << endl;
        ss << "Branch history: " << _branchHistString() << endl;
        ss << "Path History:   " << _pathHistString()  << endl;

        for (unsigned int i = 0; i < _tables->size(); i++) {
            ss << _tables->at(i)->debugString() << endl;
        }
        return ss.str();
    }

    int64_t bitSize() const {
	int64_t size = 0;
	// Size of tables
	for (unsigned int i = 0; i < _tables->size(); i++) {
	    size += _tables->at(i)->bitSize();
	}

	// Cost of storing global branch and path histories
	size += _path_mask_bs.count();
	size += _branch_mask_bs.count();

	if (_dynamic_theta) {
	    size += _threshold_counter.size();
	}

	if (_dynamic_hist_length) {
	    size += _alias_counter.size();
	    // number of entries in last table for tag bits
	    size += _tables->back()->numElements();
	}

        return size;
    }
private:
    int32_t _theta;
    int32_t _idx_bits;

    bitset_t _path_hist_bs, _path_mask_bs;
    bitset_t _branch_hist_bs, _branch_mask_bs;

    int32_t _S;
    std::vector<Table *> *_tables;

    int64_t _nu_miss, _nu_correct; /* number of updates on miss/correct */

    bool _dynamic_theta, _dynamic_hist_length;
    // Dynamic theta fitting
    SaturatingCounter _threshold_counter;
    // Dynamic history length fitting
    int32_t _addtl_hist_length_vals;
    SaturatingCounter _alias_counter;
    bitset_t _tag_bits;

    bool _verbose;

    int64_t _numOps;

    int32_t _sumCounters(const Instruction& ins) {
        int32_t sum = _tables->size() / 2;
        for (uint32_t i = 0; i < _tables->size(); i++) {
            _tables->at(i)->debug(_verbose);
            sum += _tables->at(i)->predict(ins.PC(), _branch_hist_bs, _path_hist_bs);
            _tables->at(i)->debug(false);
        }
        return sum;
    }

    void _dynamicThresholdFit(bool incorrect_predict, bool update_correct) {
        if (_dynamic_theta) {
            if (incorrect_predict) { /* update because of incorrect prediction */
                _threshold_counter.inc();
                if (_threshold_counter.isSaturated()) {
                    _theta = _theta + 1;
                    _threshold_counter.reset();
                }
            } else if (update_correct) { /* correctly predicted branch, low vote*/
                _threshold_counter.dec();
                if (_threshold_counter.isSaturated()) {
                    _theta = _theta - 1;
                    _threshold_counter.reset();
                }
            }
        }
    }

    void _dynamicHistoryFit(const Instruction &insn, bool incorrect_prediction,
                            bool low_threshold, int32_t index) {
        if (_dynamic_hist_length && (low_threshold || incorrect_prediction)) {
            int8_t low_bit = 1 & insn.PC();
            bool use_long;
            if (low_bit == _tag_bits[index]) {
                _alias_counter.inc();
                if (_alias_counter.isSaturated()) {
                    use_long = true;
                }
            } else {
                _alias_counter.dec();
                _alias_counter.dec();
                _alias_counter.dec();
                _alias_counter.dec();

                if (_alias_counter.isSaturated()) {
                    use_long = false;
                }
            }

            for (unsigned int i = 0; i < _tables->size(); i++) {
                _tables->at(i)->useLong(use_long);
            }

            _tag_bits[index] = low_bit;
        }
    }


    std::string _pathHistString() const {
        std::string phs = _path_hist_bs.to_string();
        phs = phs.substr(phs.size()-_path_mask_bs.count(), phs.size());
        return phs;
    }

    std::string _branchHistString() const {
        std::string bhs = _branch_hist_bs.to_string();
        bhs = bhs.substr(bhs.size() - _branch_mask_bs.count(), bhs.size());
        return bhs;
    }

};

#endif

