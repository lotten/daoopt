// (c) 2010 Alexander Ihler under the FreeBSD license; see license.txt for details.
#ifndef __MEX_INDEXEDHEAP
#define __MEX_INDEXEDHEAP

#include <stdexcept>
#include "assert.h"
#include "vector.h"

#ifdef MEX
#include "mex.h"
#endif

namespace mex {

/* Indexed Heap : a "reversible" heap from key (double) to unique values (uint, 0..N-1; some can be missing) 
 *   for which the value can also be used to look up (access) or remove an entry
 *
 * Mainly this data structure is used in factor- or edge- priority scheduling; the factor or edge index
 *   is used as the unique value and its priority is the key.  This gives a lightweight priority queue
 *   that still enables edges to be easily "reprioritized".
 *
 */

class indexedHeap : virtual public mxObject {

private:
	mex::vector<double>   _p;					// store the key (a double), typically a priority
	mex::vector<uint32_t> _id;				// store the identifying value (uint) of this key
	mex::vector<uint32_t> _rev;				// reverse lookup from value to position in the heap


public:
	indexedHeap() : _p(), _id(), _rev() { };

	template <class InputPIterator, class InputIIterator>
	indexedHeap(InputPIterator pBegin, InputPIterator pEnd, InputIIterator iBegin) : _p(), _id(), _rev() {
		size_t dist = distance(pBegin,pEnd);
		uint32_t mxi;
		_p.resize(dist); _id.resize(dist);
		for (size_t i=0;i<_p.size();++i,++pBegin,++iBegin) {
			_p[i] = *pBegin; _id[i] = *iBegin;
			mxi = std::max(mxi,*iBegin);
		}
		_rev.resize(mxi+1);
		uint32_t ii=1;
		for (vector<uint32_t>::iterator i=_id.begin();i!=_id.end();++i) _rev[*i]=ii;

		for (size_t i=size()/2;i!=0;--i) maxHeapify(i);
	}

	void clear() { _p.clear(); _id.clear(); _rev.clear(); }

	void insert(double P, uint32_t R) {
		if (_rev.size()<=R) _rev.resize(R+1,0);
		size_t i;
		if (_rev[R] != 0) {
		  i=_rev[R]; _p[i-1]=P;
			maxHeapify(i);
		} else {
			_p.push_back(P); _id.push_back(R); _rev[R]=size();
			i=size();
		}
		for (;;) {
			size_t parent=i/2;
			if (parent>0 && _p[parent-1] < _p[i-1]) { 
				heapSwap(parent,i);
				i=parent;
			} else return;
		}
	}

	void erase(size_t R) {
		size_t I=_rev[R];
		if (I==0) return;		// already gone
		if (I==size()) { 
			_rev[R]=0; _p.pop_back(); _id.pop_back(); 
		} else {
			heapSwap(I,size()); 
			_rev[R]=0; _p.pop_back(); _id.pop_back();
			maxHeapify(I);
		}
	}

  void debug() {
		std::cout<<"P: "; for (int i=0;i<_p.size();++i) std::cout<<_p[i]<<" "; std::cout<<"\n";
		std::cout<<"I: "; for (int i=0;i<_id.size();++i) std::cout<<_id[i]<<" "; std::cout<<"\n";
		std::cout<<"R: "; for (int i=0;i<_rev.size();++i) std::cout<<_rev[i]<<" "; std::cout<<"\n";
	}

	void pop() { 
		assert(size()>0);
		erase(_id[0]); 
	}

	std::pair<double,uint32_t> top() {
		assert(size()>0);
		return std::pair<double,uint32_t>(_p[0],_id[0]);
	}

	size_t size() { return _p.size(); }
	bool empty() { return _p.empty(); }

#ifdef MEX
	virtual bool mxCheckValid(const mxArray* M) { 
			return _p.mxCheckValid(mxGetField(M,0,"p")) &&
							_id.mxCheckValid(mxGetField(M,0,"id")) &&
							_rev.mxCheckValid(mxGetField(M,0,"rev")); 
	}
	virtual void mxSet(mxArray* M) {
		_p.mxSet(mxGetField(M,0,"p"));
		_id.mxSet(mxGetField(M,0,"id"));
		_rev.mxSet(mxGetField(M,0,"rev")); 
		M_=M;
	}
	virtual mxArray* mxGet() {
		if (!mxAvail()) {
			mxArray* m;
			int retval = mexCallMATLAB(1,&m,0,NULL,"indexedHeap");
			if (retval) mexErrMsgTxt("Error creating new indexedHeap");
			indexedHeap H; H.mxSet(m);
			//H=*this;
			H._p=_p; H._id=_id; H._rev=_rev;
			H._p.mxGet(); H._id.mxGet(); H._rev.mxGet();
			mxSwap(H);
		}
		return M_;		
	}

	void test() {
		std::cout<<"M: "<<M_<<" - "<<_p.mxGet()<<" @ "<<_p.begin()<<"\n";
		mxArray* m = mxCreateSharedDataCopy(M_);
		std::cout<<"M: "<<M_<<" - "<<mxGetField(M_,0,"p")<<" @ "<<mxGetPr(mxGetField(M_,0,"p"))<<"\n";
		std::cout<<"m: "<<m<<" - "<<mxGetField(m,0,"p")<<" @ "<<mxGetPr(mxGetField(m,0,"p"))<<"\n";
		mxArray* p = mxCreateSharedDataCopy(mxGetField(m,0,"p"));
		std::cout<<"          p:"<<p<<" @ "<<mxGetPr(p)<<"\n";
	}


	virtual mxArray* mxGetRef() {
		if (!mxAvail()) return mxGet();		// if this will be new in matlab, create it
		else return mxCreateSharedDataCopy(mxGet());	// otherwise create a shared version
	}

	virtual void mxRelease()   { throw std::runtime_error("Not implemented"); }
	virtual void mxDestroy()   { throw std::runtime_error("Not implemented"); }
	void mxSwap(indexedHeap& H) {
		_p.mxSwap(H._p);
		_id.mxSwap(H._id);
		_rev.mxSwap(H._rev);
		std::swap(M_,H.M_);
	}
#endif
	
private:
	void heapSwap(uint32_t i, uint32_t j) {
		std::swap(_p[i-1]  ,_p[j-1]);
		std::swap(_id[i-1] ,_id[j-1]);
		std::swap(_rev[_id[i-1]],_rev[_id[j-1]]);
	}

	void maxHeapify(uint32_t i) {
		for (;;) {
			uint32_t left=2*i, right=2*i+1, largest=i;
			if (left<=_p.size() && _p[left-1]>_p[largest-1]) largest=left;
			if (right<=_p.size() && _p[right-1]>_p[largest-1]) largest=right;
			if (largest == i) return;
			heapSwap(largest,i);
			i=largest;
		}
	}

};

}

#endif
