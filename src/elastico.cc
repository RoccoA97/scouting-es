#include <cstdio>


#include "elastico.h"
#include "format.h"
#include "slice.h"
#include "controls.h"
#include "log.h"

size_t dummy(char *data, size_t n, size_t l, void *s) { 
  (void)(data); // TODO: Unused variable
  (void)(s);    // TODO: Unused variable
  //  std::cout << data << std::endl;
return n*l; }

ElasticProcessor::ElasticProcessor(size_t max_size_, ctrl *c, std::string requrl,
				   uint32_t ptcut, uint32_t qualcut) : 
  tbb::filter(parallel),
  max_size(max_size_),
  control(c),
  request_url(requrl),
  c_request_url(""),
  pt_cut(ptcut),
  qual_cut(qualcut),
  handle(0),
  headers(NULL)
{ 
  LOG(TRACE) << "Created elastico filter at " << static_cast<void*>(this);
}

ElasticProcessor::~ElasticProcessor(){
  //  fprintf(stderr,"Wrote %d muons \n",totcount);
}

void ElasticProcessor::makeCreateIndexRequest(unsigned int run){
  if(handle){
    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);
    headers=NULL;
    handle=0;
  }
  std::ostringstream ost;
  ost << run;
  c_request_url = request_url+"_"+ost.str();
  std::string index_setting_run =  "{\n";
  index_setting_run += "\"settings\" : {\n";
  index_setting_run += "         \"number_of_shards\" : 2,\n"; 
  index_setting_run += "         \"number_of_replicas\" : 0,\n";       
  index_setting_run += "         \"refresh_interval\" : \"2s\"\n";
  index_setting_run += "  },\n";
  index_setting_run += "\"mappings\" : {\n";
  index_setting_run += "         \"_doc\" : {\n";
  index_setting_run += "             \"properties\" : {\n";
  index_setting_run += "                 \"orbit\" : {\"type\" : \"integer\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"bx\"    : {\"type\" : \"integer\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"eta\"   : {\"type\" : \"float\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"phi\"   : {\"type\" : \"float\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"etap\"   : {\"type\" : \"float\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"phip\"   : {\"type\" : \"float\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"pt\"    : {\"type\" : \"float\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"chrg\"  : {\"type\" : \"integer\", \"index\" : \"true\"},\n";
  index_setting_run += "                 \"qual\"  : {\"type\" : \"integer\", \"index\" : \"true\"}\n";
  index_setting_run += "                 }\n";
  index_setting_run += "          }\n";
  index_setting_run += "  }\n";
  index_setting_run += "}";
  handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_VERBOSE, 0LL);
  headers=NULL; /* init to NULL is important */
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers); 
  curl_easy_setopt(handle, CURLOPT_URL, c_request_url.c_str());  
  curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PUT"); /* !!! */

  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, index_setting_run.c_str()); /* data goes here */

  int res = curl_easy_perform(handle);
  printf("index creation returned %d\n",res);
  curl_slist_free_all(headers);
  curl_easy_cleanup(handle);
  //now set up the handle for bulk population
  handle = curl_easy_init();  
  std::string moreheaders = "Expect:";
  headers=NULL; /* init to NULL is important */
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, moreheaders.c_str());      
  
  curl_easy_setopt(handle, CURLOPT_VERBOSE, 0LL);
  curl_easy_setopt(handle, CURLOPT_HEADER, 0);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, dummy);
  
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers); 
  std::string p_request_url=c_request_url+"/_doc/_bulk";
  curl_easy_setopt(handle, CURLOPT_URL, p_request_url.c_str()); 
}

void ElasticProcessor::makeAppendToBulkRequest(std::ostringstream &particle_data, char*c){
  uint32_t *p = (uint32_t*)c;
  uint32_t header = *p++;
  int mAcount = (header&header_masks::mAcount)>>header_shifts::mAcount;
  int mBcount = (header&header_masks::mBcount)>>header_shifts::mBcount;
  
  uint32_t bx=*p++;
  uint32_t orbit=*p++;
  for(/*unsigned*/ int i = 0; i < mAcount; i++){
    uint32_t mf = *p++;
    uint32_t ms = *p++;
    uint32_t ipt = (mf >> shifts::pt) & masks::pt;
    if(ipt<pt_cut)continue;
    uint32_t qual = (mf >> shifts::qual) & masks::qual;
    if(qual<qual_cut)continue;
    float pt = (ipt-1)*gmt_scales::pt_scale;
    float phiext = ((mf >> shifts::phiext) & masks::phiext)*gmt_scales::phi_scale;
    uint32_t ietaext = ((mf >> shifts::etaext) & masks::etaextv);
    if(((mf >> shifts::etaext) & masks::etaexts)!=0) ietaext -= 512;
    float etaext = ietaext*gmt_scales::eta_scale;
    uint32_t iso = (ms >> shifts::iso) & masks::iso;
    uint32_t chrg = (ms >> shifts::chrg) & masks::chrg;
    uint32_t chrgv = (ms >> shifts::chrgv) & masks::chrgv;
    uint32_t index = (ms >> shifts::index) & masks::index;
    float phi = ((ms >> shifts::phi) & masks::phi)*gmt_scales::phi_scale;
    uint32_t ieta = (ms >> shifts::eta) & masks::etav;
    if(((mf >> shifts::eta) & masks::etas)!=0) ieta -= 512;
    float eta = ieta*gmt_scales::eta_scale;

    (void)(iso);      // TODO: Unused variable
    (void)(chrgv);    // TODO: Unused variable
    (void)(mBcount);  // TODO: Unused variable
    (void)(index);    // TODO: Unused variable

    particle_data << "{\"index\" : {}}\n" 
		  << "{\"orbit\": " <<  orbit << ',' 
		  << "\"bx\": "     <<  bx << ',' 
		  << "\"eta\": "    <<  eta << ',' 
		  << "\"phi\": "    <<  phi << ',' 
		  << "\"etap\": "   <<  etaext << ',' 
		  << "\"phip\": "   <<  phiext << ',' 
		  << "\"pt\": "     <<  pt << ',' 
		  << "\"chrg\": "   <<  chrg << ',' 
		  << "\"qual\": "   <<  qual 
		  << "}\n";

  }

}



void* ElasticProcessor::operator()( void* item ){
  Slice& input = *static_cast<Slice*>(item);
  std::ostringstream particle_data;
  char* p = input.begin();
  if(control->running){
    if(c_request_url.empty()) makeCreateIndexRequest(control->run_number);
    while(p!=input.end()){
      makeAppendToBulkRequest(particle_data,p);
    }
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE,particle_data.str().length());
    curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, particle_data.str().c_str()); /* data goes here */
    int res = curl_easy_perform(handle);

    (void)(res);    // TODO: Unused variable

  }
  if(!control->running && !c_request_url.empty()){
    c_request_url.clear();
  }
  return &input;
}
