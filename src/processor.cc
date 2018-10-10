#include <cstdio>
#include "processor.h"
#include "format.h"
#include "slice.h"
#include <iostream>

StreamProcessor::StreamProcessor(size_t max_size_) : 
  max_size(max_size_),
  tbb::filter(parallel)
{ fprintf(stderr,"Created transform filter at 0x%llx \n",(unsigned long long)this);}  
StreamProcessor::~StreamProcessor(){
  //  fprintf(stderr,"Wrote %d muons \n",totcount);
}

void* StreamProcessor::operator()( void* item ){
  Slice& input = *static_cast<Slice*>(item);
  
  if(input.size()<max_size){
    //  fprintf(stderr,"transform operator gets slice of %d size\n",input.size());

    
  }
  // 				 (unsigned long long)input.end());
  char* p = input.begin();
  Slice& out = *Slice::allocate( 2*max_size);
  char* q = out.begin();
  uint32_t counts = 0;
  int bsize = sizeof(block1);

  while(p!=input.end()){

    block1 *bl = (block1*)p;
    int mAcount = 0;
    int mBcount = 0;

    uint32_t bxmatch=0;
    uint32_t orbitmatch=0;
    bool AblocksOn[8];
    bool BblocksOn[8];
    for(unsigned int i = 0; i < 8; i++){
      uint32_t bx = bl->bx[i];
      bxmatch += (bx==bl->bx[0])<<i;
      uint32_t orbit = bl->orbit[i];
      orbitmatch += (orbit==bl->orbit[0])<<i;

      //      uint32_t phiext = (bl->mu1f[i] >> shifts::phiext) & masks::phiext;
      uint32_t pt = (bl->mu1f[i] >> shifts::pt) & masks::pt;
      // uint32_t qual = (bl->mu1f[i] >> shifts::qual) & masks::qual;
      // uint32_t etaext = (bl->mu1f[i] >> shifts::etaext) & masks::etaext;
      // uint32_t iso = (bl->mu1s[i] >> shifts::iso) & masks::iso;
      // uint32_t chrg = (bl->mu1s[i] >> shifts::chrg) & masks::chrg;
      // uint32_t chrgv = (bl->mu1s[i] >> shifts::chrgv) & masks::chrgv;
      // uint32_t index = (bl->mu1s[i] >> shifts::index) & masks::index;
      // uint32_t phi = (bl->mu1s[i] >> shifts::phi) & masks::phi;
      // uint32_t eta = (bl->mu1s[i] >> shifts::eta) & masks::eta;
      // uint32_t rsv = (bl->mu1s[i] >> shifts::rsv) & masks::rsv;
      AblocksOn[i]=(pt>0);
      if(pt>0){
	mAcount++;
      }
      //      phiext = (bl->mu2f[i] >> shifts::phiext) & masks::phiext;
      pt = (bl->mu2f[i] >> shifts::pt) & masks::pt;
      // qual = (bl->mu2f[i] >> shifts::qual) & masks::qual;
      // etaext = (bl->mu2f[i] >> shifts::etaext) & masks::etaext;
      // iso = (bl->mu2s[i] >> shifts::iso) & masks::iso;
      // chrg = (bl->mu2s[i] >> shifts::chrg) & masks::chrg;
      // chrgv = (bl->mu2s[i] >> shifts::chrgv) & masks::chrgv;
      // index = (bl->mu2s[i] >> shifts::index) & masks::index;
      // phi = (bl->mu2s[i] >> shifts::phi) & masks::phi;
      // eta = (bl->mu2s[i] >> shifts::eta) & masks::eta;
      // rsv = (bl->mu2s[i] >> shifts::rsv) & masks::rsv;
      BblocksOn[i]=(pt>0);
      if(pt>0)mBcount++;
    }

    uint32_t bxcount = std::max(mAcount,mBcount);
    if(bxcount == 0) {
      p+=sizeof(block1);
      continue;
    }

    uint32_t header = (bxmatch<<24)+(mAcount << 16) + (orbitmatch<<8) + mBcount;

    counts += mAcount;
    counts += mBcount;
    memcpy(q,(char*)&header,4); q+=4;
    memcpy(q,(char*)&bl->bx[0],4); q+=4;
    memcpy(q,(char*)&bl->orbit[0],4); q+=4;
    for(unsigned int i = 0; i < 8; i++){
      if(AblocksOn[i]){
	memcpy(q,(char*)&bl->mu1f[i],4); q+=4;
	memcpy(q,(char*)&bl->mu1s[i],4); q+=4;
      }
    }
    for(unsigned int i = 0; i < 8; i++){
      if(BblocksOn[i]){
	memcpy(q,(char*)&bl->mu2f[i],4); q+=4;
	memcpy(q,(char*)&bl->mu2s[i],4); q+=4;
      }
    }
    p+=sizeof(block1);

  }


    //here do the processing 
    // for(;;) {
    //     if( p==input.end() ) 
    //         break;
    //     // Note: no overflow checking is needed here, as we have twice the 
    //     // input string length, but the square of a non-negative integer n 
    //     // cannot have more than twice as many digits as n.
    //     m(q,"%ld",y);
    //     q = strchr(q,0);
    // }
    out.set_end(q);
    Slice::giveAllocated(&input);
    out.set_counts(counts);
    return &out;
}
