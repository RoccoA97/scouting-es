#include <system_error>
#include <fstream>

#include "output.h"
#include "slice.h"
#include "log.h"
#include "tools.h"

OutputStream::OutputStream( const char* output_file_base, ctrl& c) : 
    tbb::filter(serial_in_order),
    my_output_file_base(output_file_base),
    totcounts(0),
    current_file_size(0),
    file_count(-1),
    control(c),
    current_file(0),
    current_run_number(0),
    journal_name(my_output_file_base + "/" + "index.journal")
{
  LOG(TRACE) << "Created output filter at " << static_cast<void*>(this);
}

static void update_journal(std::string journal_name, uint32_t run_number, uint32_t index)
{
  std::string new_journal_name = journal_name + ".new";

  // Create a new journal file
  std::ofstream journal (new_journal_name);
  if (journal.is_open()) {
    journal << run_number << "\n" << index << "\n";
    journal.close();
  } else {
    LOG(ERROR) << "WARNING: Unable to open journal file";
  }

  // Replace the old journal
  if (rename(new_journal_name.c_str(), journal_name.c_str()) < 0 ) {
    LOG(ERROR) << tools::strerror("Journal file move failed");
  }
}

static bool read_journal(std::string journal_name, uint32_t& run_number, uint32_t& index)
{
    std::ifstream journal (journal_name);
    if (journal.is_open()) {
      journal >> run_number >> index;
      journal.close();
      return true;
    } 
    return false;
}

void* OutputStream::operator()( void* item ) 
{
    Slice& out = *static_cast<Slice*>(item);
    totcounts += out.get_counts();

    if ( control.running.load(std::memory_order_acquire) ) {
      if (current_file == NULL || current_file_size > control.max_file_size || current_run_number != control.run_number) {
        open_next_file();
      }
      
      size_t n = fwrite( out.begin(), 1, out.size(), current_file );
      current_file_size += n;      
      if ( n != out.size() ) {
        LOG(ERROR) << "Can't write into output file: Have to write " << out.size() << ", but write returned " << n;
      }
    }

    // If not running and we have a file then close it
    if (!control.running && current_file != NULL) {
      close_and_move_current_file();
      file_count = -1;
    }

    out.free();
    return NULL;
}

void OutputStream::close_and_move_current_file()
{
  // Close and move current file
  if (current_file) {
    fclose(current_file);
    current_file = NULL;

    // Move the file, so it can be processes by file move daemon
    char source_file_name[256];
    char target_file_name[256];
    // TODO: Check if the destination directory exists
    sprintf(source_file_name,"%s/in_progress/scout_%06d_%06d.dat", my_output_file_base.c_str(), current_run_number, file_count);
    sprintf(target_file_name,"%s/scout_%06d_%06d.dat", my_output_file_base.c_str(), current_run_number, file_count);

    LOG(INFO) << "rename: " << source_file_name << " to " << target_file_name;
    if ( rename(source_file_name, target_file_name) < 0 ) {
      LOG(ERROR) << tools::strerror("File rename failed");
    }

    current_file_size = 0; 
    file_count += 1;
  }
}

void OutputStream::open_next_file() 
{
  close_and_move_current_file();

  // We can change current_run_number only here, after the (previous) file was closed and moved
  if (current_run_number != control.run_number) {
    current_run_number = control.run_number;
    file_count = -1;
  }

  // If this is the first file then check if we have a journal file
  if (file_count < 0) {
    // Set default file index
    file_count = 0;

    uint32_t journal_run_number;
    uint32_t index;

    if (read_journal(journal_name, journal_run_number, index)) {
      LOG(INFO) << "We have journal:";
      LOG(INFO) << "  run_number: " << journal_run_number;
      LOG(INFO) << "  index:      " << index;   
    } else {
      LOG(INFO) << "No journal file.\n";
    }

    LOG(INFO) << "Current run_number: " << current_run_number;
    if (current_run_number == journal_run_number) {
      file_count = index;
    }
    LOG(INFO) << "  using index:      " << file_count;    
  }

  // Create a new file
  char run_order_stem[256];
  // TODO: Check if the destination directory exists
  sprintf(run_order_stem,"/in_progress/scout_%06d_%06d.dat",current_run_number,file_count);
  std::string file_end(run_order_stem);
  std::string filename = my_output_file_base+file_end;
  current_file = fopen( filename.c_str(), "w" );
  if (current_file == NULL) {
    throw std::system_error(errno, std::system_category(), "File open error");
  }

  // Update journal file (with the next index file)
  update_journal(journal_name, current_run_number, file_count+1);
}
