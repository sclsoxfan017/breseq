/*****************************************************************************
 
 AUTHORS
 
 Jeffrey E. Barrick <jeffrey.e.barrick@gmail.com>
 David B. Knoester
 
 LICENSE AND COPYRIGHT
 
 Copyright (c) 2011-20122010 Michigan State University
 
 breseq is free software; you can redistribute it and/or modify it under the  
 terms the GNU General Public License as published by the Free Software 
 Foundation; either version 1, or (at your option) any later version.
 
 *****************************************************************************/

#include "libbreseq/fastq.h"
#include "libbreseq/reference_sequence.h"

using namespace std;

namespace breseq {
  
  /*
   normalize_fastq
   
   correct common errors in input fastq and normalize to standard SANGER format

   */

  Summary::AnalyzeFastq normalize_fastq(
                                        const string &file_name, 
                                        const string &convert_file_name, 
                                        const uint32_t file_index, 
                                        const int32_t trim_end_on_base_quality, 
                                        const bool filter_reads,
                                        uint64_t current_read_file_bases,
                                        const uint64_t read_file_base_limit
                                        ) 
  {
    cerr << "    Converting/filtering FASTQ file..." << endl;
    
    // Set up maps between formats
    map<string,uint8_t> format_to_chr_offset;
    format_to_chr_offset["SANGER"] = 33;
    format_to_chr_offset["SOLEXA"] = 64;
    format_to_chr_offset["ILLUMINA_1.3+"] = 64;
    
    // Summary information that will be printed at the end
    uint32_t max_read_length = 0;
    uint8_t min_quality_score = 255;
    uint8_t max_quality_score = 0;
    uint64_t original_num_bases = 0;
    uint64_t num_bases = 0;
    uint64_t original_num_reads = 0;
    uint64_t num_reads = 0;
    uint64_t homopolymer_filtered_reads = 0;
    uint64_t N_filtered_reads = 0;
    
    // Predict the format (and count original stats)
    string quality_format = cFastqQualityConverter::predict_fastq_file_format(file_name, original_num_reads, original_num_bases, max_read_length, min_quality_score, max_quality_score);
    
    
    //debug
    //cerr << "min_quality_score "     << (int)min_quality_score  << endl;
    //cerr << "max_quality_score "     << (int)max_quality_score  << endl;

    string converted_fastq_name(file_name);
    
    //std::cout << m_quality_format << std::endl;

    cerr << "    Original format: " << quality_format << " New format: SANGER"<< endl;
    cerr << "    Original reads: " << original_num_reads << " bases: "<< original_num_bases << endl;

    cFastqQualityConverter fqc(quality_format, "SANGER");
    cFastqSequence on_sequence;
    
    // re-open input for another pass
    cFastqFile input_fastq_file(file_name.c_str(), fstream::in);
    
    //open output converted file
    cFastqFile output_fastq_file(convert_file_name.c_str(), fstream::out);

    // recalculate min and max quality scores from table
    cFastqSequence min_max_sequence;
    min_max_sequence.m_qualities.append(1,min_quality_score);
    min_max_sequence.m_qualities.append(1,max_quality_score);
    fqc.convert_sequence(min_max_sequence);
    min_quality_score = (uint8_t)min_max_sequence.m_qualities[0];
    max_quality_score = (uint8_t)min_max_sequence.m_qualities[1];
    
    // (much faster than looking through all qualities again)
    
    uint32_t on_read = 1;
    while (input_fastq_file.read_sequence(on_sequence, fqc)) {
      
      if ( filter_reads ) {

        // Discard sequences that are 50% or more N.
        if ( 0.5 * static_cast<double>(on_sequence.length()) <= static_cast<double>(on_sequence.m_base_counts[base_list_N_index]) ) {
          N_filtered_reads++;
          continue;
        }
        
        // Ignore heavily homopolymer reads, as these are a common type of machine error
        // Discard sequences that are 90% or more of a single base or N.
        bool homopolymer_filtered = false;
        for (uint8_t i=0; i<base_list_including_N_size; i++) {
          if ( 0.9 * static_cast<double>(on_sequence.length()) <= 
              static_cast<double>(on_sequence.m_base_counts[i] + on_sequence.m_base_counts[base_list_N_index]) ) {
            homopolymer_filtered = true;
            break;
          }
        }
        
        if (homopolymer_filtered)  {
          homopolymer_filtered_reads++;
          continue;
        }
        
      } // end filter read block
    
      // truncate second name
      on_sequence.m_name_plus = "";
      
      // fastq quality convert
      fqc.convert_sequence(on_sequence);
      
      // trim bad quality scores from the end and ignore if we have cut to half or less of the original length
      if (trim_end_on_base_quality) {
        double original_size = static_cast<double>(on_sequence.length());
        fastq_sequence_trim_end_on_base_quality(on_sequence, trim_end_on_base_quality);
        if ( 0.5 * original_size <= static_cast<double>(on_sequence.length()) )
          continue;
      }
      
      // uniformly name, to prevent problems drawing alignments
      // and allows us to know the input order
      
      on_sequence.m_name = to_string(file_index) + ":" + to_string(on_read++);
      
      // Alternative method that keeps number of digits connstant
      //char string_buffer[256];
      //sprintf(string_buffer, "%03u:%010u", file_index, on_read++);
      //on_sequence.m_name = string_buffer;
      
      num_reads++;
      num_bases+= on_sequence.m_sequence.length();
      
      // convert base qualities
      output_fastq_file.write_sequence(on_sequence);
      
      // check to see if we've reached the limit
      if (read_file_base_limit) {
        current_read_file_bases += on_sequence.m_sequence.length();
        if (current_read_file_bases > read_file_base_limit)
          break;
      }
      
    }
    input_fastq_file.close();
    output_fastq_file.close();
    
    converted_fastq_name = convert_file_name;
    
    // quality scores are in SANGER at this point
    min_quality_score = min_quality_score - format_to_chr_offset["SANGER"];
    max_quality_score = max_quality_score - format_to_chr_offset["SANGER"];

    //cerr << "min_quality_score "     << (int)min_quality_score  << endl;
    //cerr << "max_quality_score "     << (int)max_quality_score  << endl;

    if (filter_reads) {
      cerr << "    Filtered reads: " << N_filtered_reads << " (≥50% N) " << homopolymer_filtered_reads << " (≥90% one base)" << endl;
    }
    cerr << "    Analyzed reads: " << num_reads << " bases: "<< num_bases << endl;

    
    double avg_read_length = static_cast<double>(num_bases) / static_cast<double>(num_reads);
    
    Summary::AnalyzeFastq retval(
                                 max_read_length, 
                                 avg_read_length,
                                 original_num_reads,
                                 homopolymer_filtered_reads,
                                 N_filtered_reads,
                                 num_reads, 
                                 min_quality_score, 
                                 max_quality_score, 
                                 original_num_bases,
                                 num_bases, 
                                 quality_format, 
                                 "SANGER", 
                                 converted_fastq_name
                                 );
    return retval;
  }
  
  // converts a sequence file
  void convert_fastq(const string &from_file_name, const string &to_file_name, const string &from_format, const string &to_format, bool _reverse_complement)
  {
    cFastqFile input_fastq_file, output_fastq_file;
    input_fastq_file.open(from_file_name.c_str(), fstream::in);
    output_fastq_file.open(to_file_name.c_str(), fstream::out);

    cFastqQualityConverter fqc(from_format, to_format);

    cFastqSequence on_sequence;
    while (input_fastq_file.read_sequence(on_sequence, fqc)) 
    {
      fqc.convert_sequence(on_sequence);
      if (_reverse_complement) 
        on_sequence = reverse_complement(on_sequence);
      output_fastq_file.write_sequence(on_sequence);
    }
    
    input_fastq_file.close();
    output_fastq_file.close();
  }
  
  void fastq_sequence_trim_end_on_base_quality(cFastqSequence& seq, const uint32_t base_quality)
  {
    for (uint32_t i=0; i<seq.m_qualities.size(); i++) {
      if (static_cast<uint8_t>(seq.m_qualities[i]-33) < base_quality) {
        seq.m_sequence.resize(i);
        seq.m_qualities.resize(i);
        break; 
      }
    }
  }
  
  bool cFastqSequence::identical(cFastqSequence& seq)
  {
    return ( (this->m_sequence == seq.m_sequence) && (this->m_qualities == seq.m_qualities) );
  }


  // constructor
  cFastqQualityConverter::cFastqQualityConverter(const string &_from_quality_format, const string &_to_quality_format)
  {
    // Set up maps between formats
    map<string,uint8_t> format_to_chr_offset;
    format_to_chr_offset["SANGER"] = 33;
    format_to_chr_offset["SOLEXA"] = 64;
    format_to_chr_offset["ILLUMINA_1.3+"] = 64;
    
    map<string,string> format_to_quality_type;
    format_to_quality_type["SANGER"] = "PHRED";
    format_to_quality_type["SOLEXA"] = "SOLEXA";
    format_to_quality_type["ILLUMINA_1.3+"] = "PHRED";
    
    from_quality_format = _from_quality_format;
    to_quality_format = _to_quality_format;
    
    // check what we asked for is valid...
    ASSERT(format_to_chr_offset.count(from_quality_format), 
           "Unknown FASTQ quality score format: " + from_quality_format + "\nValid choices are 'SANGER', 'SOLEXA', 'ILLUMINA_1.3+', 'NUMERICAL'");
    ASSERT(format_to_chr_offset.count(to_quality_format), 
           "Unknown FASTQ quality score format: " + to_quality_format + "\nValid choices are 'SANGER', 'SOLEXA', 'ILLUMINA_1.3+', 'NUMERICAL'");

    
    from_quality_type = format_to_quality_type[from_quality_format];
    to_quality_type = format_to_quality_type[to_quality_format];

    from_chr_offset = format_to_chr_offset[from_quality_format];
    to_chr_offset = format_to_chr_offset[to_quality_format];

    
    this->resize(256);
    for (uint16_t i = 0; i<=255; i++) {
      (*this)[i] = 0;
    }
    
    for (uint16_t from_chr = 0; from_chr<=255; from_chr++) {

      int32_t from_quality = from_chr - from_chr_offset;
      
      // Calculate the probability of error
      double probability_of_error;
      
      if (from_quality_type == "SOLEXA") {
        probability_of_error = 1 / (1+pow(10,(double)from_quality/10));
      } else if (from_quality_type == "PHRED") {
        probability_of_error = pow(10,-(double)from_quality/10);
      } else {
        cerr << "Unknown base quality score type: " << from_quality_type << endl;
        exit(-1);
      }
      
      //Convert back to quality score
      int32_t to_quality;
            
      if (to_quality_type == "SOLEXA") {
        to_quality = static_cast<uint32_t>(round(10 * log((1-probability_of_error)/probability_of_error) / log(10)));
      } else if (to_quality_type == "PHRED") {
        to_quality = static_cast<uint32_t>(round(-10 * log(probability_of_error) / log(10)));
      } else {
        cerr << "Unknown base quality score type: " << to_quality_type << endl;
        exit(-1);
      }
            
      int16_t to_chr = to_quality + to_chr_offset;
      
      // May be out of range
      if ((to_chr < 0) || (to_chr > 255)) continue;

      (*this)[(uint8_t)from_chr] = (uint8_t)to_chr;
      
      // Debug
      //cerr << from_chr << " => " << to_chr << endl;
    }     
    
  }

  void cFastqQualityConverter::convert_sequence(cFastqSequence &seq) {
    
    for(uint32_t i=0; i < seq.m_qualities.size(); i++)
    {
      seq.m_qualities[i] = (*this)[seq.m_qualities[i]];
    }
  }
  
  string cFastqQualityConverter::predict_fastq_file_format(const string& file_name, uint64_t& original_num_reads, uint64_t& original_num_bases, uint32_t& max_read_length, uint8_t& min_quality_score, uint8_t& max_quality_score) 
  {
    
  // Set up maps between formats
  map<string,uint8_t> format_to_chr_offset;
  format_to_chr_offset["SANGER"] = 33;
  format_to_chr_offset["SOLEXA"] = 64;
  format_to_chr_offset["ILLUMINA_1.3+"] = 64;
    
  cFastqFile input_fastq_file(file_name.c_str(), fstream::in);
  input_fastq_file.m_check_for_repeated_read_names = true;
  assert(input_fastq_file.is_open());
  
  cFastqSequence on_sequence;
  cFastqQualityConverter prelim_fqc("ILLUMINA_1.3+", "SANGER");
  
  while (input_fastq_file.read_sequence(on_sequence, prelim_fqc)) {
    
    //increment read number
    original_num_reads++;
    
    //check sequence length
    if( on_sequence.m_sequence.size() > max_read_length ) max_read_length = on_sequence.m_sequence.size();
      
      //add current sequence length to number of bases
      original_num_bases += on_sequence.m_sequence.size();
      
      //iterate through sequence grabbing the associated scores
    for (uint32_t i=0; i<on_sequence.m_qualities.size(); i++) {
      int this_score(uint8_t(on_sequence.m_qualities[i]));
      if( this_score > max_quality_score ) max_quality_score = this_score;
        if( this_score < min_quality_score ) min_quality_score = this_score;
    }
  }
  input_fastq_file.close();
  
  // Default is SANGER
  string quality_format = "SANGER";
  
  // Typical range: (-5, 40) + 64
  if (min_quality_score >= format_to_chr_offset["SOLEXA"] - 5) {
    quality_format = "SOLEXA";
  } 
  // Typical range:  (0, 40) + 64
  if (min_quality_score >= format_to_chr_offset["ILLUMINA_1.3+"]) {
    quality_format = "ILLUMINA_1.3+";
  }
    
    return quality_format;
  }
  
  //constructor
  cFastqFile::cFastqFile() :
    fstream(), m_current_line(0), m_file_name(""), m_needs_conversion(false),
    m_check_for_repeated_read_names(false), m_last_read_name(""), m_repeated_read_name_count(0)
  {
  }
 
  
  cFastqFile::cFastqFile(const string &file_name, std::ios_base::openmode mode) :
    fstream(file_name.c_str(), mode), m_current_line(0), m_file_name(file_name), m_needs_conversion(false),
    m_check_for_repeated_read_names(false), m_last_read_name(""), m_repeated_read_name_count(0)
  { 
    ASSERT(!(*this).fail(), "Could not open file: " +  file_name);
  }

  // read one sequence record from the file
  bool cFastqFile::read_sequence(cFastqSequence &sequence, cFastqQualityConverter& fqc) {
    
    // We're done, no error
    if (this->eof())
     return false; 

    
    uint32_t count = 0;
    string line;
        
    memset(sequence.m_base_counts, 0, sizeof(sequence.m_base_counts));
    
    // get the next four lines
    while (count < 4) {
      std::getline(*this, line);
      m_current_line++;
      
      // Didn't get a first line, then we ended correctly
      if (this->eof()) {
        if (count == 0) {
          return false;
        } else {
          uint32_t last_valid_line = static_cast<uint32_t>(floor((m_current_line-1)/4.0) * 4);
          fprintf(stderr, "Incomplete FASTQ sequence record found at end of file.\nFile %s\nLine: %d\n", m_file_name.c_str(), m_current_line-1);
          fprintf(stderr, "You may be able to repair this damage and salvage the reads before this point with the command:\n");
          fprintf(stderr, "  head -n %u %s > new.fastq\n", last_valid_line, m_file_name.c_str());
          fprintf(stderr, "Then use \"new.fastq\" as input.\n");
          exit(-1);
        }
      }
      
      switch (count) {
        case 0:
          if( line[0] != '@' ) {
            fprintf(stderr, "FASTQ sequence record does not begin with @NAME line.\nFile %s\nLine: %d\n", m_file_name.c_str(), m_current_line);
            exit(-1);
          }
          sequence.m_name = line.substr(1,string::npos);
          if (m_current_line == 1) {
            if (sequence.m_name.find("/") != string::npos) m_needs_conversion = true;
          }
          
          // Delete any sequence name information after the first space...
          // Necessary for scrubbing SRA FASTQs, for example.
          { // block to keep inside this switch case
            size_t space_pos = sequence.m_name.find(" ");
            if (space_pos != string::npos) 
            {
              sequence.m_name.erase(space_pos);
              m_needs_conversion = true;
            }
          }
          
          // some SRA files have identical read names, we don't like this...
          if (m_check_for_repeated_read_names)
          {
            string original_read_name = sequence.m_name;
            if (m_last_read_name == sequence.m_name)
            {
              m_repeated_read_name_count++;
              sequence.m_name += "r" + to_string(m_repeated_read_name_count);
              m_needs_conversion = true;
            }
            else
            {
              m_repeated_read_name_count = 0;
            }
            m_last_read_name = original_read_name;
          }
          
          break;
          
        case 1:
          sequence.m_sequence = line;
          
          // check for extraneous DOS ending
          if (m_current_line == 1) {
            if( sequence.m_sequence[sequence.m_sequence.size()-1] == '\r') {
              sequence.m_sequence.resize(sequence.m_sequence.size()-1);
              m_needs_conversion = true;
            }
          }
          
          for (uint32_t i=0; i<sequence.m_sequence.size(); i++) {
            
            // convert to uppercase and require
            // reformatting if this was necessary
            switch (sequence.m_sequence[i]) {
                
              case 'A':
              case 'T':
              case 'C':
              case 'G':
                break;
                
              case 'N':
                m_needs_conversion = true;
                break;
                
              case 'a':
                sequence.m_sequence.replace(i,1,1,'A');
                m_needs_conversion = true;
                break;
                
              case 't':
                sequence.m_sequence.replace(i,1,1,'T');
                m_needs_conversion = true;
                break;
                
              case 'c':
                sequence.m_sequence.replace(i,1,1,'C');
                m_needs_conversion = true;
                break;
                
              case 'g':
                sequence.m_sequence.replace(i,1,1,'G');
                m_needs_conversion = true;
                break;

              case 'n':
                sequence.m_sequence.replace(i,1,1,'N');
                m_needs_conversion = true;
                break;
              
              // all other characters converted to 'N'
              default :
                sequence.m_sequence.replace(i,1,1,'N');
                m_needs_conversion = true;

            }

            // keep a count of the number of each base for detecting homopolymeric reads
            sequence.m_base_counts[basechar2index(sequence.m_sequence[i])]++;
            
            if(sequence.m_sequence[i] != 'A' && 
               sequence.m_sequence[i] != 'T' && 
               sequence.m_sequence[i] != 'G' && 
               sequence.m_sequence[i] != 'C' && 
               sequence.m_sequence[i] != 'N') {
              
              fprintf(stderr, "FASTQ sequence character not allowed %c.\nSequence: %s\nFile %s\nLine: %d\n", 
                      sequence.m_sequence[i], sequence.m_sequence.c_str(), m_file_name.c_str(), m_current_line);
              exit(-1);
            }
          }
          
          break;
        case 2:
          
          //Only need to see if the first character is a +
          if( line[0] != '+' ) {
            fprintf(stderr, "FASTQ sequence record does not contain +NAME line.\nFile %s\nLine: %d\n", m_file_name.c_str(), m_current_line);
            exit(-1);
          }
          // Could optionally check to see if the name after the + was either absent or identical to the earlier name
          sequence.m_name_plus = line.substr(1,string::npos);

          break;
        case 3:
          
          if (sequence.m_sequence.size() == line.size()) {
            sequence.m_qualities = line;
          } else if ((line.find_first_of(" ") != string::npos) && (line.find_first_not_of(" -0123456789\t") == string::npos)) {
            
            sequence.m_numerical_qualities = true;
            m_needs_conversion = true;
            vector<string> numerical_qualities(split(line, " "));
            if( sequence.m_sequence.size() != numerical_qualities.size() ) {
              fprintf(stderr, "FASTQ sequence record has different SEQUENCE and numerical QUALITY lengths.\nFile %s\nLine: %d\n", m_file_name.c_str(), m_current_line);
              exit(-1);
            }
            
            // convert the qualities to characters with the Illumina offset (which keeps things from being negative)
            sequence.m_qualities = "";
            for(vector<string>::iterator it = numerical_qualities.begin(); it != numerical_qualities.end(); it++)
            {
              // use of uint16_t is on purpose to force proper conversion @JEB
              sequence.m_qualities += static_cast<char>(from_string<int16_t>(*it)) + fqc.from_chr_offset;
            }
          } else {
            ERROR("FASTQ QUALITY line length does not match SEQUENCE length.\nFile: " + m_file_name + " Line: " + to_string(m_current_line) + "\nSequence:     " + sequence.m_sequence + "\nQuality Line: " + line);
          }

          break;
      }
      
      count++;
  }
    
    return true;
  }

  void cFastqFile::write_sequence(const cFastqSequence &sequence) {
    fprintf(*this, "@%s\n", sequence.m_name.c_str()     );
    fprintf(*this, "%s\n" , sequence.m_sequence.c_str() );
    fprintf(*this, "+%s\n", sequence.m_name_plus.c_str());
    fprintf(*this, "%s\n" , sequence.m_qualities.c_str());
  }


  int32_t cSimFastqSequence::SEED_VALUE = time(NULL);
  /*!
    qscore_cumulative_probability_table

    Achieved by:

      Step: Gather frequencies of scores in the following DCAMP read files:
        SRR014475.fastq
        SRR014476.fastq
        SRR014477.fastq
        SRR014478.fastq
        SRR014479.fastq
        SRR014480.fastq
        SRR014481.fastq
        SRR014482.fastq
        SRR014483.fastq
        SRR014484.fastq
        SRR014485.fastq

      NOTE: The scores are in the Sanger format and represent a Phred quality
      score from 0 to 93 using ASCII 33 to 126.
      (http://en.wikipedia.org/wiki/FASTQ_format)

      Step: Normalize each frequency to 1 by dividing each score's frequency by
      the sum of all frequencies that occured.

      Step: Calculate the cumulative probability for each score.

      Step: Multiply each probability by 100,000.

      ***@GRC: Multiplied by 100,000 to achieve precision to the 100,000th
      decimal place. Wasn't sure if C's rand() function would be able to roll
      between 0 and 1 by increment of .00001 so opted for this method.
    */

  map<uint32_t, uint32_t>
  cSimFastqSequence::qscore_cumulative_probability_table =
  make_map<uint32_t, uint32_t>
    (33, 57)
    (34, 96)
    (35, 214)
    (36, 475)
    (37, 985)
    (38, 1501)
    (39, 1759)
    (40, 2017)
    (41, 2275)
    (42, 2532)
    (43, 27931)
    (44, 28185)
    (45, 28439)
    (46, 28693)
    (47, 28946)
    (48, 29201)
    (49, 29457)
    (50, 29714)
    (51, 29971)
    (52, 30230)
    (53, 30491)
    (54, 30753)
    (55, 31016)
    (56, 31280)
    (57, 31545)
    (58, 31809)
    (59, 32074)
    (60, 32337)
    (61, 32600)
    (62, 32861)
    (63, 33121)
    (64, 58268)
    (65, 65171)
    (66, 65422)
    (67, 69288)
    (68, 69533)
    (69, 69775)
    (70, 70013)
    (71, 81668)
    (72, 81899)
    (73, 96356)
    (78, 96412)
    (84, 100000);

  map<char, string> cSimFastqSequence::random_snp_base_options =
  make_map<char, string>
    ('A', "TCG")
    ('T', "ACG")
    ('C', "ATG")
    ('G', "ATC")
    ('N', "ACTG");

  char cSimFastqSequence::random_insertion_base_options[] =
  {'A', 'C', 'T', 'G'};

  char cSimFastqSequence::get_random_quality_score(void)
  {
    uint32_t reserved_offset = qscore_cumulative_probability_table[35];
    
    //Roll between 0 and 99,999.
    uint32_t random_die = rand() % 100000 - reserved_offset;
    random_die += reserved_offset;

    map<uint32_t, uint32_t>::const_iterator it =
        qscore_cumulative_probability_table.begin();

    //Iteterate through until random probability is greater then a cumulative
    //probability in the table.
    while (random_die >= it->second) {
      ++it;
    }

    //Found it! Return as a character.
    return char(it->first);
  }

  char cSimFastqSequence::get_random_error_base(const char not_this_base)
  {
    ASSERT(random_snp_base_options.count(not_this_base), "Error!");

    uint32_t size = random_snp_base_options[not_this_base].size();
    return random_snp_base_options[not_this_base][rand() % size];
  }

  char cSimFastqSequence::get_random_insertion_base(void)
  {
    //Roll from 0 to 3.
    return base_char_list[rand() % 4];
  }

  //Return if this particular base is an error given a quality score.
  bool cSimFastqSequence::is_random_error_base(char ascii_qscore)
  {
    const int32_t qscore = int32_t(ascii_qscore) - 33;

    double p_value = pow(10, -qscore/10.0);

    //We want precision to the 100,000th decimal place.
    double p_value_max = p_value * 100000;

    return ((rand() % 100000) <= p_value_max);
  }

  const uint32_t deletion_probability = 100000;//10E-5
  bool cSimFastqSequence::is_random_deletion_base(void)
  {
    //Roll from 0 to deletion_probability.
    return (1 == (rand() % deletion_probability));
  }

  const uint32_t insertion_probability = 100000;//10E-5
  bool cSimFastqSequence::is_random_insertion_base(void)
  {
    //Roll from 0 to insertion_probability.
    return (1 == (rand() % insertion_probability));
  }

  void cSimFastqSequence::GaussianRNG::box_muller_transform(float* z0, float* z1) {
  static const float PI =
  3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706f;
    //Two random variables in the interval (0, 1] with a precision of .001
    float u1 = static_cast<float>((rand() % 1000 + 1) / 1000.f);
    float u2 = static_cast<float>((rand() % 1000 + 1) / 1000.f);

    *z0 = sqrtf(-2.f * log(u1)) * std::cos(2.f * PI * u2); 
    *z1 = sqrtf(-2.f * log(u1)) * std::sin(2.f * PI * u2); 

    return;
  }

  cSimFastqSequence::GaussianRNG::GaussianRNG(int mean, int stdev) 
    : m_mean(mean)
    , m_stdev(stdev) {

    srand(cSimFastqSequence::SEED_VALUE);
    GaussianRNG::box_muller_transform(&m_z0, &m_z1);
    m_z0 = (m_z0 * m_stdev) + m_mean; 
    m_z1 = (m_z1 * m_stdev) + m_mean; 

    return;
  }

  int32_t cSimFastqSequence::GaussianRNG::sample() {
    int32_t ret_val = static_cast<int32_t>(round(m_z0));
    m_store = m_z1;

    GaussianRNG::box_muller_transform(&m_z0, &m_z1);
    m_z0 = (m_z0 * m_stdev) + m_mean; 
    m_z1 = (m_z1 * m_stdev) + m_mean; 

    return ret_val;
  }


  vector<int32_t> cSimFastqSequence::GaussianRNG::samples(int32_t size) {
    vector<int32_t> ret_val(size, 0);
    for (int32_t i = 0; i < size; ++i) {
      ret_val[i] = this->sample();
    }
    return ret_val;
  }


  cFastqSequence cSimFastqSequence::simulate(const cAnnotatedSequence& ref_sequence,
                                             uint32_t start_1,
                                             uint32_t read_size,
                                             int8_t   strand,
                                             uint32_t  id,
                                             bool     verbose) {
    cFastqSequence ret_val;
    sprintf(ret_val.m_name, "READ-%i", id);
    sprintf(ret_val.m_name_plus, "[strand]:%i\t[start_1]:%u", strand, start_1);
    
    if(verbose && !(id % 10000) && id){cout << "\tREAD: " << id << endl;}

    /*! Algorithm in use:

        1) Grab 2 x read_size segment from a random location in the reference
          sequence. Get the reverse complement of this sequence if it is
          a negative strand.

          ***Note: We treat the sequence circularly such that if we index
          bases from past the end of the sequence we will instead start
          indexing them from the front.

          ***Note: Grab 2x read_size for easier access to the following bases
          when deletions occur and for the impossible chance that each base
          is a deletion.

        2) Iterate through this segment.

        3) At each index/base determine if insertion(INS), deletion(DEL),
          if it's a possible error due to quality score or if the base is
          normal.

           If DEL  : Continue on to next base in segment.
           If INS  : Randomly insert one of the four possible bases.
           If Error: Assign random base other then the one given.
           Normal  : Assign base given in segment.

          Assign the base a random quality score which is determine by the
          qscore_cumulative_probability_table.
    */


    string verbose_errors(read_size, ' ');
    string verbose_deletions (read_size, ' ');
    string verbose_insertions(read_size, ' ');

    //! Algorithm Step 1:
    string ref_segment =
        ref_sequence.get_circular_sequence_1(start_1, 2 * read_size);

    if (strand == -1) {
      ref_segment = reverse_complement(ref_segment);
    }


    //! Algorithm Step 2:
    //Initializations for iterating through bases.
    ret_val.m_sequence.resize(read_size);
    ret_val.m_qualities.resize(read_size);

    //Index for bases in the simulated read.
    size_t index_to_assign = 0;

    //Index for bases in the reference segment.
    size_t index_in_ref_segment = 0;

    //Begin assigning bases to simulated read.
    while (index_to_assign < read_size) {
    //! Algorithm Step 3:

      //! DEL base.
      if (cSimFastqSequence::is_random_deletion_base()) {
       //For verbose.
        verbose_deletions[index_in_ref_segment] =
            ref_segment[index_in_ref_segment];
        //Continue to next index/base in reference segment.
        ++index_in_ref_segment;
        continue;
      }
      //! INS base.
      else if (cSimFastqSequence::is_random_insertion_base()) {
        char base_to_insert =
            cSimFastqSequence::get_random_insertion_base();

        //Assign base.
        ret_val.m_sequence[index_to_assign] = base_to_insert;

        //Assign quality score.
        ret_val.m_qualities[index_to_assign] =
            cSimFastqSequence::get_random_quality_score();

        //For verbose
        verbose_insertions[index_in_ref_segment] = base_to_insert;

        /*Increment index_to_assign but not index_in_ref_segment and
         continue to next iteration.*/
        ++index_to_assign;
        continue;
      }
      //! Determine if error or normal base.
      else {
        char quality_score =
            cSimFastqSequence::get_random_quality_score();

        //! Error base.
        if (cSimFastqSequence::is_random_error_base(quality_score)) {
          /*Since an error occured we want to assign a base different from
           the one given. */
          char not_this_base = ref_segment[index_in_ref_segment];

          //Assign different base.
          char new_base =
              cSimFastqSequence::get_random_error_base(not_this_base);
          ret_val.m_sequence[index_to_assign] = new_base;

          //Assing quality score.
          ret_val.m_qualities[index_to_assign] = quality_score;

          //For verbose.
          verbose_errors[index_to_assign] = new_base;
        }
        //! Normal base.
        else {
          //Assign base.
          ret_val.m_sequence[index_to_assign] = ref_segment[index_in_ref_segment];

          //Assign quality score.
          ret_val.m_qualities[index_to_assign] = quality_score;
        }
        /*Increment both index_to_assign and index_in_ref_segment and
        continue to next iteration. */
        ++index_to_assign;
        ++index_in_ref_segment;
      }
    } //End assigning bases to simulated read.
    if (verbose) {
      if (verbose_deletions.find_first_not_of(' ')  != string::npos ||
          verbose_insertions.find_first_not_of(' ') != string::npos ) {
        printf("\tVerbose output for simulated read    :  %s\n",
               ret_val.m_name.c_str());

        const string &original =
            ref_sequence.get_circular_sequence_1(start_1, 2 * read_size);
        printf("\tReference Segment(2 x Read Size)     :  %s\n",
               original.c_str());

        if (strand == -1) {
          printf("\tSimulated negative strand            :  %s\n",
                 ref_segment.c_str());
        }

        if (verbose_errors.find_first_not_of(' ')     != string::npos) {
          printf("\tSimulated errors                     :  %s\n",
                 verbose_errors.c_str());
        }

        if (verbose_deletions.find_first_not_of(' ')  != string::npos) {
          printf("\tSimulated DEL                        :  %s\n",
                 verbose_deletions.c_str());
        }

        if (verbose_insertions.find_first_not_of(' ') != string::npos) {
          printf("\tSimulated INS                        :  %s\n",
                 verbose_insertions.c_str());
        }

        printf("\tFinal simulated read sequence        :  %s\n",
                 ret_val.m_sequence.c_str());
        printf("\tFinal simulated read quality scores  :  %s\n",
                 ret_val.m_qualities.c_str());
        printf("\n");
      }
    }//End verbose output.

    return ret_val;
  }
  void cSimFastqSequence::simulate_single_ends(const cAnnotatedSequence& sequence,
                                               uint32_t n_reads,
                                               uint32_t read_size, 
                                               string file_name,
                                               bool verbose) 
  {
    cFastqFile out(file_name.c_str(), ios_base::out);

    srand(cSimFastqSequence::SEED_VALUE);
    for (uint32_t i = 0; i < n_reads; ++i) {
      uint32_t start_1 = rand() % sequence.get_sequence_size() + 1;
      int8_t strand    = (rand() % 2) == 0 ? 1 : -1;
      cFastqSequence read = cSimFastqSequence::simulate(sequence,
                                                        start_1,
                                                        read_size,
                                                        strand,
                                                        i + 1,
                                                        verbose); 
      out.write_sequence(read);
    }

    return;
  }

  void cSimFastqSequence::simulate_paired_ends(const cAnnotatedSequence& sequence,
                                               uint32_t n_reads,
                                               uint32_t read_size, 
                                               uint32_t mean,
                                               uint32_t stdev,
                                               string pair_1_file_name,
                                               string pair_2_file_name,
                                               bool verbose)
  {
    cFastqFile pair_1_out(pair_1_file_name.c_str(), ios_base::out);
    cFastqFile pair_2_out(pair_2_file_name.c_str(), ios_base::out);

    cSimFastqSequence::GaussianRNG random_size(mean, stdev);
    srand(cSimFastqSequence::SEED_VALUE);
    for (uint32_t i = 0; i < n_reads; ++i) {

      //Pair 1
      uint32_t start_1 = rand() % sequence.get_sequence_size() + 1;
      int8_t strand    = 1;
      cFastqSequence pair_1 = cSimFastqSequence::simulate(sequence,
                                                          start_1,
                                                          read_size,
                                                          strand,
                                                          i + 1,
                                                          verbose); 

      //Pair 2
      start_1 = (start_1 + random_size.sample() - (read_size * 2)) % sequence.get_sequence_size();
      strand  = -1;
      cFastqSequence pair_2 = cSimFastqSequence::simulate(sequence,
                                                          start_1,
                                                          read_size,
                                                          strand,
                                                          i + 1,
                                                          verbose); 
      if (rand() % 100 < 50) {
        pair_1.m_name += "/1";
        pair_2.m_name += "/2";
        pair_1_out.write_sequence(pair_1);
        pair_2_out.write_sequence(pair_2);
      } else {
        pair_1.m_name += "/2";
        pair_2.m_name += "/1";
        pair_1_out.write_sequence(pair_2);
        pair_2_out.write_sequence(pair_1);
      }

    }


    return;
  }

// Reverse complement and also uppercase
// Convert most characters to 'N'. Might want to give errors on non-printable characters
  char reverse_complement_lookup_table[256] = {
/*  0*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/* 16*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/* 32*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/* 48*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/* 64*/    'N', 'T', 'N', 'G', 'N', 'N', 'N', 'C', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/* 80*/    'N', 'N', 'N', 'N', 'A', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/* 96*/    'N', 'T', 'N', 'G', 'N', 'N', 'N', 'C', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*112*/    'N', 'N', 'N', 'N', 'A', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*128*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*144*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*160*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*176*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*192*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*208*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*224*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N',
/*240*/    'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N'
  };
  
} // breseq namespace

