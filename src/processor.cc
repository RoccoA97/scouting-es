#include "processor.h"
#include "format.h"
#include "slice.h"
#include "log.h"

StreamProcessor::StreamProcessor(size_t max_size_, bool noZS_) : 
  tbb::filter(parallel),
  max_size(max_size_),
  nbPackets(0),
  noZS(noZS_)
{ 
  LOG(TRACE) << "Created transform filter at " << static_cast<void*>(this);
}  


StreamProcessor::~StreamProcessor(){
  //  fprintf(stderr,"Wrote %d muons \n",totcount);
}

Slice* StreamProcessor::process(Slice& input, Slice& out)
{
  nbPackets++;
  int bsize = sizeof(block1);
  if((input.size()-constants::orbit_trailer_size)%bsize!=0){
    LOG(WARNING)
      << "Frame size not a multiple of block size. Will be skipped. Size="
	    << input.size() << " - block size=" << bsize;
    return &out;
  }
  char* p = input.begin();
  char* q = out.begin();
  uint32_t counts = 0;

  
  while(p!=input.end()){
    bool endoforbit = false;
    block1 *bl = (block1*)p;
    int mAcount = 0;
    int mBcount = 0;
    uint32_t bxmatch=0;
    uint32_t orbitmatch=0;

    bool AblocksOn[8];
    bool BblocksOn[8];

    for(unsigned int i = 0; i < 8; i++){
      if(bl->bx[i]==constants::deadbeef){
	p += constants::orbit_trailer_size;
	endoforbit = true;
	break;
      }
      uint32_t bx = (bl->bx[i] >> shifts::bx) & masks::bx;
      uint32_t interm = (bl->bx[i] >> shifts::interm) & masks::interm;
      bl->mu1f[i] |= interm == constants::intermediate_marker ?
	(constants::intermediate & masks::interm)<<shifts::interm :
	(constants::final        & masks::interm)<<shifts::interm;
      bl->mu2f[i] |= interm == constants::intermediate_marker ?
	(constants::intermediate & masks::interm)<<shifts::interm :
	(constants::final        & masks::interm)<<shifts::interm;
      bxmatch += (bx==((bl->bx[0] >> shifts::bx) & masks::bx))<<i;
      uint32_t orbit = bl->orbit[i];
      orbitmatch += (orbit==bl->orbit[0])<<i;
      uint32_t pt = (bl->mu1f[i] >> shifts::pt) & masks::pt;
      AblocksOn[i]=(pt>0 || noZS);
      if(pt>0){
	mAcount++;
      }
      pt = (bl->mu2f[i] >> shifts::pt) & masks::pt;
      BblocksOn[i]=(pt>0 || noZS);
      if(pt>0){
	mBcount++;
      }
    }
    if(endoforbit) continue;
    uint32_t bxcount = std::max(mAcount,mBcount);
    if(bxcount == 0) {
      p+=bsize;
      LOG(WARNING) << '#' << nbPackets << ": Detected a bx with zero muons, this should not happen. Packet is skipped."; 
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


    out.set_end(q);
    out.set_counts(counts);
    return &out;  
}

void* StreamProcessor::operator()( void* item ){
  Slice& input = *static_cast<Slice*>(item);
  Slice& out = *Slice::allocate( 2*max_size);

  process(input, out);

  Slice::giveAllocated(&input);
  return &out;
}
