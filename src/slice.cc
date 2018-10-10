#include "slice.h"

tbb::concurrent_bounded_queue<Slice*> Slice::free_slices = tbb::concurrent_bounded_queue<Slice*>();

Slice *Slice::preAllocate(size_t max_size, size_t nslices){
  if(Slice::free_slices.empty()){
    for(unsigned int i = 0; i < nslices; i++){
      Slice* t = Slice::allocate(max_size);
      Slice::free_slices.push(t);
    }
  }
  Slice *t;
  Slice::free_slices.pop(t);
  return t;
}

void Slice::shutDown(){
  while(!Slice::free_slices.empty()){
    Slice *t;
    Slice::free_slices.pop(t);
    t->free();
  }
}

Slice *Slice::getAllocated(){
  Slice *t;
  //  fprintf(stderr,"getAllocated called, current size %d\n",free_slices.size());
  Slice::free_slices.pop(t);
  //  fprintf(stderr,"successfully popped, current size %d\n",free_slices.size());
  return t;
}

void Slice::giveAllocated(Slice *t){
  t->set_end(t->begin());
  Slice::free_slices.push(t);
}

