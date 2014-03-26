#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<assert.h>
#include<stdlib.h>
using namespace std;
#define BUF_LEN 100

int total_threads;
fstream conf_file;
int main( int argc, char **args ) {
  if(argc != 5 ) {
    cout<<"Usage: rundiff data_dir ref_execno total_execno conf_file" << endl;
    return 1;
  }
  conf_file.open(args[4], fstream::in);
  if( !conf_file.is_open()) {
    cout << args[3] <<" conf_file file not found" << endl;
    return 1;
  }
  string str;
  getline(conf_file, str);
  stringstream ss(str);
  ss >> total_threads;
  cout << "total threads " << total_threads << endl;

  int ref_exec; 
  string ref_argv(args[2]);
  stringstream ss1(ref_argv);
  ss1 >> ref_exec;
  cout << "ref excution no " << ref_exec << endl;
 

  int no_exec; 
  string exec_argv(args[3]);
  stringstream ss2(exec_argv);
  ss2 >> no_exec;
  cout << "total excutions " << no_exec << endl;
 // int no_exec2; 
 // string exec2_argv(args[3]);
//  stringstream ss2(exec2_argv);
 // ss2 >> no_exec2;
   
  fstream src_file, dest_file, out_file; 
  char src_file_name[BUF_LEN];
  char dest_file_name[BUF_LEN];
  char out_file_name[BUF_LEN];

  sprintf( out_file_name, "%s/diff.output", args[1]);
  out_file.open( out_file_name, fstream::out );
  
  int *ref_val;
  ref_val = new int[total_threads*total_threads];
 
  sprintf( src_file_name, "%s/counter.report.%d", args[1], ref_exec); 
  src_file.open( src_file_name, fstream::in );
  if( !src_file.is_open() ){
    cout<< src_file_name  << " not found" << endl;
    return 1;
  }
  // populate ref execution values 
  for(int i=0; i<total_threads*total_threads; i++ ){
    string src_str, dest_str;
    getline(src_file, src_str);
    stringstream src_ss(src_str);
    src_ss >> ref_val[i];
  } 
  src_file.close();  

//  compare with other executions

  for( int j=1;j<=no_exec; j++) {
      
    if(j==ref_exec) continue;
    sprintf( dest_file_name, "%s/counter.report.%d", args[1], j);

 
    dest_file.open( dest_file_name, fstream::in);
    if( !dest_file.is_open() ){
      cout<< dest_file_name  << " not found" << endl;
      return 1;
    }   
    double sum_of_diff = 0.0;
    int active_pairs = 0;
    int src_total=0;
    int dest_total=0;
    
    for( int k=0; k< total_threads*total_threads; k++ ){
      string dest_str;
      getline(dest_file, dest_str);
      stringstream dest_ss(dest_str);
      int dest_val;
      dest_ss>>dest_val;
      src_total +=ref_val[k];
      dest_total +=dest_val;
      //if(ref_val[k] == 0 || dest_val == 0) {
      if(dest_val == 0) {

          continue; // considering not active thread pairs are not eligible enough to compare
      }	
      else {
        active_pairs++;
       //   cout << (ref_val[k] - dest_val) << endl;
        double diff = (double) abs(ref_val[k] - dest_val);
        sum_of_diff += diff/dest_val;
      }
    }

     
    //double result = sum_of_diff/(total_threads*total_threads);
    double result = sum_of_diff/active_pairs;
    //assert( result <= 1.0 );
    assert ( result >= 0 );

    //cout << "VAREXEC ( " << ref_exec << ","<< j<< " ) :: " << result << endl; 
    out_file << "VAREXEC ( " << ref_exec << ","<< j<< " ) :: " << result << endl;   	
    dest_file.close();
 
  }  
  out_file.close();
  cout << "Result : " << out_file_name << endl;
  
}
